#include <cstring>
#include <cstdlib>
//
#include <lux_shared/common.hpp>
#include <lux_shared/util/packer.hpp>
#include <lux_shared/vec_hash.hpp>
#include <lux_shared/map.hpp>
//
#include <db.hpp>
#include <entity.hpp>
#include "map.hpp"

static VecMap<ChkPos, Chunk> chunks;

constexpr Uns LIGHTNING_BITS_PER_COLOR = 5;
static_assert(LIGHTNING_BITS_PER_COLOR * 3 <= sizeof(LightLvl) * 8);
constexpr Uns LIGHTNING_RANGE          = std::exp2(LIGHTNING_BITS_PER_COLOR);
constexpr Uns RAY_NUM  = 4 * (2 * LIGHTNING_RANGE - 2);
constexpr Uns RAY_AREA = std::pow(2 * LIGHTNING_RANGE - 1, 2);
static Arr<Vec2F, RAY_NUM> ray_step_map;
//@CONSIDER Arr2 for 2d access
static Arr<F32, RAY_AREA>  ray_mul_map;


static void update_chunk_lightning(ChkPos const &pos, Chunk& chunk);
static bool is_chunk_loaded(ChkPos const& pos) {
    return chunks.count(pos) > 0;
}

static Chunk& load_chunk(ChkPos const& pos) {
    LUX_ASSERT(!is_chunk_loaded(pos));
    LUX_LOG("loading chunk");
    LUX_LOG("    pos: {%zd, %zd}", pos.x, pos.y);
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
                if(rand() % 30 == 0 && entities.size() < 20) {
                    auto id = create_player();
                    entity_comps.pos[id] = map_pos;
                    if(rand() % 10 != 0) {
                        entity_comps.item[id] = {1.f};
                        constexpr char default_name[] = "donger man";
                        entity_comps.name[id].resize(sizeof(default_name) - 1);
                        std::memcpy(entity_comps.name[id].data(),
                                    default_name, sizeof(default_name) - 1);
                    } else {
                        entity_comps.sphere[id] = {2.f};
                        constexpr char default_name[] = "Big Bob";
                        entity_comps.name[id].resize(sizeof(default_name) - 1);
                        std::memcpy(entity_comps.name[id].data(),
                                    default_name, sizeof(default_name) - 1);
                    }
                }
            }
        }
        /*if((map_pos.x % 16 != 0 && map_pos.y % 16 != 0) && pos_hash % 30 == 0) {
            U16 col = std::rand() % 0x1000;
            add_light_node(to_map_pos(pos, i),
                {col & 0x00F, (col & 0x0F0) >> 4, (col & 0xF00) >> 8});
        }*/
        chunk.voxels[i] = voxel_id;
        chunk.light_lvls[i] = 0xFFF0;
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
};

VecMap<ChkPos, HashTable<ChkIdx, LightSource>>                   light_sources;
VecMap<ChkPos, HashSet<ChkIdx, util::Identity<ChkIdx>>> awaiting_light_sources;
VecSet<ChkPos>                                          awaiting_light_updates;

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


void add_light_source(MapPos const& pos, Vec3F col) {
    ChkPos chk_pos = to_chk_pos(pos);
    ChkIdx chk_idx = to_chk_idx(pos);
    //@TODO
    LUX_ASSERT(light_sources.count(chk_pos) == 0 ||
               light_sources.at(chk_pos).count(chk_idx) == 0);
    light_sources[chk_pos][chk_idx] = {(Vec3<U8>)(col * Vec3F(255.f))};
    awaiting_light_sources[chk_pos].insert(chk_idx);
    awaiting_light_updates.insert(chk_pos);
}

void del_light_source(MapPos const& pos) {
    //@TODO
    LUX_UNIMPLEMENTED();
}

static void apply_light_source(MapPos const& src_pos, LightSource& light) {
    LUX_UNIMPLEMENTED();
}

static void update_chunk_lightning(ChkPos const &pos, Chunk& chunk) {
    if(awaiting_light_sources.count(pos) > 0) {
        for(auto& chk_idx : awaiting_light_sources.at(pos)) {
            auto& source = light_sources.at(pos).at(chk_idx);
            apply_light_source(to_map_pos(pos, chk_idx), source);
        }
        awaiting_light_sources.at(pos).clear();
    }
}
