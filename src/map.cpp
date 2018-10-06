#include <cstdlib>
//
#include <lux_shared/common.hpp>
#include <lux_shared/util/packer.hpp>
#include <lux_shared/vec_hash.hpp>
#include <lux_shared/map.hpp>
//
#include <db.hpp>
#include "map.hpp"

static VecMap<ChkPos, Chunk> chunks;

constexpr Uns LIGHTNING_BITS_PER_COLOR = 4;
static_assert(LIGHTNING_BITS_PER_COLOR * 3 <= sizeof(LightLvl) * 8);
constexpr Uns LIGHTNING_RANGE          = std::exp2(LIGHTNING_BITS_PER_COLOR);
constexpr Uns RAY_NUM  = 4 * (2 * LIGHTNING_RANGE - 2);
constexpr Uns RAY_AREA = std::pow(2 * LIGHTNING_RANGE - 1, 2);
static Arr<Vec2F, RAY_NUM> ray_step_map;
//@CONSIDER Arr2 for 2d access
static Arr<F32, RAY_AREA>  ray_mul_map;

void map_init() {
    LUX_LOG("generating ray map");
    Uns mul_idx = 0;
    for(Int y = -(Int)(LIGHTNING_RANGE - 1); y < (Int)LIGHTNING_RANGE; ++y) {
        for(Int x = -(Int)(LIGHTNING_RANGE - 1); x < (Int)LIGHTNING_RANGE; ++x) {
            LUX_ASSERT(mul_idx < RAY_AREA);
            ray_mul_map[mul_idx] =
                std::max(0.f, 1.f -
                         glm::length(Vec2F(x, y)) / (F32)(LIGHTNING_RANGE - 1));
            LUX_ASSERT(ray_mul_map[mul_idx] >= 0.f &&
                       ray_mul_map[mul_idx] <= 1.f);
            ++mul_idx;
        }
    }
    for(Uns i = 0; i < RAY_NUM; ++i) {
        Int constexpr SIDE = LIGHTNING_RANGE - 1;
        Vec2I src  = {0, 0};
        Vec2I dst;
        Int off = i % (2 * (LIGHTNING_RANGE - 1));
        if(i < 1 * (2 * LIGHTNING_RANGE - 2)) {
            dst = Vec2I(off - SIDE,     - SIDE);
        } else if(i < 2 * (2 * LIGHTNING_RANGE - 2)) {
            dst = Vec2I(      SIDE, off - SIDE);
        } else if(i < 3 * (2 * LIGHTNING_RANGE - 2)) {
            dst = Vec2I(SIDE - off,       SIDE);
        } else if(i < 4 * (2 * LIGHTNING_RANGE - 2)) {
            dst = Vec2I(    - SIDE, SIDE - off);
        } else LUX_UNREACHABLE();
        Vec2I delta = dst - src;
        ray_step_map[i] = (Vec2F)delta / (F32)std::max(std::abs(delta.x),
                                                       std::abs(delta.y));
    }
}

static void update_chunk_lightning(ChkPos const &pos, Chunk& chunk);
static bool is_chunk_loaded(ChkPos const& pos) {
    return chunks.count(pos) > 0;
}

static Chunk& load_chunk(ChkPos const& pos) {
    LUX_ASSERT(!is_chunk_loaded(pos));
    LUX_LOG("loading chunk");
    LUX_LOG("    pos: {%zd, %zd, %zd}", pos.x, pos.y, pos.z);
    ///@RESEARCH to do a better way to no-copy default construct
    Chunk& chunk = chunks[pos];
    for(Uns i = 0; i < CHK_VOL; ++i) {
        VoxelId voxel_id = db_voxel_id("void");
        MapPos map_pos = to_map_pos(pos, i);
        Uns pos_hash = std::hash<MapPos>()(map_pos / 2l);
        if((map_pos.x % 16 == 0 || map_pos.y % 16 == 0) && pos_hash % 3 != 0) {
            voxel_id = db_voxel_id("stone_wall");
        } else {
            if(pos_hash % 13 == 0) {
                voxel_id = db_voxel_id("stone_wall");
            } else {
                voxel_id = db_voxel_id("stone_floor");
            }
        }
        /*if((map_pos.x % 16 != 0 && map_pos.y % 16 != 0) && pos_hash % 30 == 0) {
            U16 col = std::rand() % 0x1000;
            add_light_node(to_map_pos(pos, i),
                {col & 0x00F, (col & 0x0F0) >> 4, (col & 0xF00) >> 8});
        }*/
        chunk.voxels[i] = voxel_id;
        chunk.light_lvls[i] = 0x0000;
    }
    return chunk;
}

void guarantee_chunk(ChkPos const& pos) {
    if(!is_chunk_loaded(pos)) {
        load_chunk(pos);
    }
}

Chunk const& get_chunk(ChkPos const& pos) {
    LUX_ASSERT(is_chunk_loaded(pos));
    ///wish there was a non-bound-checking way to do it
    return chunks.at(pos);
}

VoxelId get_voxel(MapPos const& pos) {
    return get_chunk(to_chk_pos(pos)).voxels[to_chk_idx(pos)];
}

VoxelType const& get_voxel_type(MapPos const& pos) {
    return db_voxel_type(get_voxel(pos));
}
//@CONSIDER separate file for lightsim
struct LightSource {
    Vec3<U8> col;
    static_assert(sizeof(col.r) * 8 >= LIGHTNING_BITS_PER_COLOR);
    //@CONSIDER optimizing for space
    Arr<bool, RAY_AREA> shadow_map = {0};
};

struct DelayedLightUpdate {
    struct IncomingRay {
        Vec2F pos;
        Uns   id;
        Vec3<U8> col;
    };
    DynArr<IncomingRay> incoming_rays;
};

VecMap<ChkPos, DelayedLightUpdate>             delayed_light_updates;
VecMap<ChkPos, HashTable<ChkIdx, LightSource>> applied_light_sources;
VecMap<ChkPos, HashTable<ChkIdx, LightSource>> awaiting_light_sources;
VecSet<ChkPos>                                 awaiting_light_updates;

void map_tick(DynArr<ChkPos>& light_updated_chunks) {
    LUX_ASSERT(light_updated_chunks.size() == 0);
    for(auto const& update : awaiting_light_updates) {
        if(is_chunk_loaded(update)) {
            light_updated_chunks.emplace_back(update);
        }
    }
    for(auto const& update : light_updated_chunks) {
        update_chunk_lightning(update, chunks.at(update));
        awaiting_light_updates.erase(update);
    }
}


void add_light_source(MapPos const& pos, Vec3<U8> col) {
    ChkPos chk_pos = to_chk_pos(pos);
    ChkIdx chk_idx = to_chk_idx(pos);
    //@TODO
    LUX_ASSERT(applied_light_sources.count(chk_pos) == 0 ||
               applied_light_sources.at(chk_pos).count(chk_idx) == 0);
    LUX_ASSERT(awaiting_light_sources.count(chk_pos) == 0 ||
               awaiting_light_sources.at(chk_pos).count(chk_idx) == 0);
    awaiting_light_sources[chk_pos][chk_idx] = {col};
    awaiting_light_updates.insert(chk_pos);
}

void del_light_source(MapPos const& pos) {
    //@TODO
    LUX_LOG("unimplemented");
    //awaiting_light_updates.insert(to_chk_pos(pos));
}

static void apply_light_source(MapPos const& src_pos, LightSource& light) {
    Arr<bool, RAY_AREA> shadow_map = {false};
    Vec2F h_pos = Vec2F(src_pos);
    for(Uns i = 0; i < RAY_NUM; ++i) {
        Vec2F ray_iter = {0, 0};
        for(;;) {
            ChkPos chk_pos = to_chk_pos(MapPos(glm::round(h_pos + ray_iter), src_pos.z));
            ChkIdx chk_idx = to_chk_idx(MapPos(glm::round(h_pos + ray_iter), src_pos.z));
            Vec2U idx_pos = glm::round(ray_iter) + (F32)(LIGHTNING_RANGE - 1);
            Uns area_idx = idx_pos.x + idx_pos.y * (2 * LIGHTNING_RANGE - 1);
            LUX_ASSERT(area_idx < RAY_AREA);
            if(is_chunk_loaded(chk_pos) &&
               db_voxel_type(chunks.at(chk_pos).voxels[chk_idx]).shape != VoxelType::BLOCK) {
                shadow_map[area_idx] = true;
            } else {
                //delayed_light_updates[chk_pos].incoming_rays.emplace_back({ray_iter + h_pos, i});
                break;
            }
            ray_iter += ray_step_map[i];
            if(ray_mul_map[area_idx] <= 0.f) break;
        }
    }
    for(Uns i = 0; i < RAY_AREA; ++i) {
        printf("%d", shadow_map[i]);
        if(i % (2 * LIGHTNING_RANGE - 1) == (2 * LIGHTNING_RANGE - 1) - 1) {
            printf("\n");
        }
    }
    printf("\n");
    Uns mul_idx = 0;
    Chunk& chunk = chunks.at(to_chk_pos(src_pos));
    Vec2I min_pos = (Vec2I)h_pos - (Int)(LIGHTNING_RANGE - 1);
    Vec2I max_pos = (Vec2I)h_pos + (Int)(LIGHTNING_RANGE - 1);
    MapPos iter = MapPos(min_pos, src_pos.z);
    for(; iter.y <= max_pos.y; ++iter.y) {
        for(; iter.x <= max_pos.x; ++iter.x) {
            LUX_ASSERT(mul_idx < RAY_AREA);
            if(to_chk_pos(iter) == to_chk_pos(src_pos)) {
                Vec3<U8> col(0xF);
                col = (Vec3<U8>)glm::round((Vec3F)col * ray_mul_map[mul_idx]) *
                    Vec3<U8>(shadow_map[mul_idx]);
                chunk.light_lvls[to_chk_idx(iter)] =
                    col.r << 12 | col.g << 8 | col.b << 4;
            } else {
                //@TODO
            }
            ++mul_idx;
        }
        iter.x = min_pos.x;
    }
}

static void update_chunk_lightning(ChkPos const &pos, Chunk& chunk) {
    if(awaiting_light_sources.count(pos) > 0) {
        for(auto& source : awaiting_light_sources.at(pos)) {
            apply_light_source(to_map_pos(pos, source.first), source.second);
        }
        awaiting_light_sources.at(pos).clear();
    }
    if(delayed_light_updates.count(pos) > 0) {
    }
}
