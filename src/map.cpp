#include <cstring>
#include <cstdlib>
//
#include <lux_shared/common.hpp>
#include <lux_shared/map.hpp>
#include <lux_shared/uninit_obj.hpp>
//
#include <physics.hpp>
#include <db.hpp>
#include <entity.hpp>
#include <chunk_loader.hpp>
#include <chunk_mesher.hpp>
#include "map.hpp"

struct ChunkPhysicsMesh {
    UninitObj<btTriangleIndexVertexArray> trigs_array;
    UninitObj<btBvhTriangleMeshShape>     shape;

    //for some reason we need to store these or bullet will crash the program
    DynArr<Vec3F> verts;
    DynArr<U32>   idxs;

    btRigidBody* body;
};

static VecSet<ChkPos> mesher_requested_chunks;
static VecMap<ChkPos, Chunk> chunks;

F32 day_cycle;
VecSet<ChkPos> updated_chunks;

//needs the out.pos set
static bool prepare_mesher_data(MesherRequest& out);
static bool is_chunk_loaded(ChkPos const& pos) {
    return chunks.count(pos) > 0;
}

static void write_suspended_block(MapPos const& pos, Block block) {
    ChkPos chk_pos = to_chk_pos(pos);
    ChkIdx chk_idx = to_chk_idx(pos);
    if(is_chunk_loaded(chk_pos)) {
        updated_chunks.insert(chk_pos);
        Chunk& chk = chunks.at(chk_pos);
        chk.blocks[chk_idx] = block;
        chk.updated_blocks.insert(chk_idx);
    } else {
        loader_write_suspended_block(block, pos);
    }
}

void map_init() {
    loader_init();
    mesher_init();
}

void map_deinit() {
    mesher_deinit();
    loader_deinit();
}

void guarantee_chunk(ChkPos const& pos) {
    if(!is_chunk_loaded(pos)) {
        ChkPos l_pos = pos;
        loader_enqueue_wait({&l_pos, 1});
        {   auto const& results = loader_lock_results();
            for(auto const& pair : results) {
                auto& chunk = chunks[pair.first];
                for(Uns i = 0; i < CHK_VOL; ++i) {
                    chunk.blocks[i] = pair.second[i];
                }
            }
            loader_unlock_results();
        }
    }
}

bool try_guarantee_chunk(ChkPos const& pos) {
    if(!is_chunk_loaded(pos)) {
        ChkPos l_pos = pos;
        loader_enqueue({&l_pos, 1});
        return false;
    }
    return true;
}

ChunkMesh::ChunkMesh(ChunkMesh&& that) {
    verts = move(that.verts);
    idxs = move(that.idxs);
    physics_mesh = that.physics_mesh;
    that.physics_mesh = nullptr;
}

Chunk::~Chunk() {
    switch(mesh_state) {
        case BUILT_PHYSICS: {
            //@TODO we might want to remove the physics body here
            delete mesh->physics_mesh;
        }
        case BUILT_TRIANGLE: {
            delete mesh;
        }
        case BUILT_EMPTY:
        case NOT_BUILT: break;
    }
}

void enqueue_missing_chunks_meshes(VecSet<ChkPos> const& requests) {
    /*static DynArr<ChkPos> loader_requests;
    loader_requests.clear();
    loader_requests.reserve_at_least(requests.size());

    for(auto const& request : requests) {
        if(mesher_requested_chunks.count(pos) == 0) {
            loader_requests.push(pos);
        }
    }*/
}

bool try_guarantee_chunk_mesh(ChkPos const& pos) {
    if(mesher_requested_chunks.count(pos) > 0) {
        return false;
    }
    Arr<ChkPos, 3 * 3 * 3> positions;
    Slice<ChkPos> slice = {positions, 0};
    if(is_chunk_loaded(pos)) {
        Chunk& chunk = chunks.at(pos);
        if(chunk.mesh_state != Chunk::NOT_BUILT) {
            return true;
        }
    } else {
        slice[slice.len++] = pos;
    }
    for(auto const& off : chebyshev_hollow<ChkCoord>) {
        if(!is_chunk_loaded(pos + off)) {
            slice[slice.len++] = pos + off;
        }
    }
    if(slice.len > 0) {
        loader_enqueue(slice);
        return false;
    }
    DynArr<MesherRequest> mesher_requests(1);
    auto& request = mesher_requests[0];
    request.pos = pos;
    if(not prepare_mesher_data(request)) {
        ///empty mesh detected
        auto& chunk = chunks.at(pos);
        chunk.mesh_state = Chunk::BUILT_EMPTY;
        return true;
    } else {
        mesher_enqueue(move(mesher_requests));
        mesher_requested_chunks.insert(pos);
        return false;
    }
}

static void synchronize_loader_results() {
    auto const& results = loader_lock_results();
    for(auto const& pair : results) {
        auto& chunk = chunks[pair.first];
        for(Uns i = 0; i < CHK_VOL; ++i) {
            chunk.blocks[i] = pair.second[i];
        }
    }
    loader_unlock_results();
}

static void synchronize_loader_block_changes() {
    auto& block_changes = loader_lock_block_changes();
    for(auto it = block_changes.begin(), end = block_changes.end();
             it != end;) {
        if(is_chunk_loaded(it->first)) {
            auto& chunk = chunks.at(it->first);
            updated_chunks.insert(it->first);
            for(auto const& change : it->second) {
                chunk.blocks[change.idx] = change.block;
                chunk.updated_blocks.insert(change.idx);
            }
            it = block_changes.erase(it);
        } else ++it;
    }
    loader_unlock_block_changes();
}

static void synchronize_mesher_results() {
    auto& meshes = mesher_lock_results();
    for(auto&& pair : meshes) {
        auto& chunk = chunks.at(pair.first);
        LUX_ASSERT(chunk.mesh_state == Chunk::NOT_BUILT);
        chunk.mesh = new ChunkMesh(move(pair.second));
        chunk.mesh_state = Chunk::BUILT_TRIANGLE;
        mesher_requested_chunks.erase(pair.first);
    }
    mesher_unlock_results();
}

void guarantee_physics_mesh_for_aabb(MapPos const& min, MapPos const& max) {
    ChkPos const c_min = to_chk_pos(min);
    ChkPos const c_max = to_chk_pos(max);
    ChkPos iter;

    //first we do a quick discard of already built meshes (most of the cases)
    bool needs_generation = false;
    for(iter.z = c_min.z; iter.z <= c_max.z; ++iter.z) {
        for(iter.y = c_min.y; iter.y <= c_max.y; ++iter.y) {
            for(iter.x = c_min.x; iter.x <= c_max.x; ++iter.x) {
                if(!is_chunk_loaded(iter) ||
                  (get_chunk(iter).mesh_state != Chunk::BUILT_PHYSICS &&
                   get_chunk(iter).mesh_state != Chunk::BUILT_EMPTY)) {
                    needs_generation = true;
                }
            }
        }
    }
    if(not needs_generation) return;

    //here we load the required chunks
    ChkPos const cl_min = c_min - (ChkCoord)1;
    ChkPos const cl_max = c_max + (ChkCoord)1;
    static DynArr<ChkPos> loader_requests;
    loader_requests.reserve_at_least(glm::compMul(cl_max - cl_min));
    for(iter.z = cl_min.z; iter.z <= cl_max.z; ++iter.z) {
        for(iter.y = cl_min.y; iter.y <= cl_max.y; ++iter.y) {
            for(iter.x = cl_min.x; iter.x <= cl_max.x; ++iter.x) {
                if(!is_chunk_loaded(iter)) {
                    loader_requests.emplace(iter);
                }
            }
        }
    }
    if(loader_requests.len > 0) {
        loader_enqueue_wait(loader_requests);
        synchronize_loader_results();
    }

    //now we build the meshes
    static DynArr<MesherRequest> mesher_requests;
    mesher_requests.reserve_at_least(glm::compMul(c_max - c_min));
    for(iter.z = c_min.z; iter.z <= c_max.z; ++iter.z) {
        for(iter.y = c_min.y; iter.y <= c_max.y; ++iter.y) {
            for(iter.x = c_min.x; iter.x <= c_max.x; ++iter.x) {
                auto& chunk = chunks.at(iter);
                if(chunk.mesh_state == Chunk::NOT_BUILT) {
                    auto& request = mesher_requests.emplace();
                    request.pos = iter;
                    if(not prepare_mesher_data(request)) {
                        //empty mesh
                        chunk.mesh_state = Chunk::BUILT_EMPTY;
                        mesher_requests.erase(mesher_requests.len - 1);
                    }
                }
            }
        }
    }
    if(mesher_requests.len > 0) {
        mesher_enqueue_wait(move(mesher_requests));
        synchronize_mesher_results();
    }

    //finally we build the physics meshes
    for(iter.z = c_min.z; iter.z <= c_max.z; ++iter.z) {
        for(iter.y = c_min.y; iter.y <= c_max.y; ++iter.y) {
            for(iter.x = c_min.x; iter.x <= c_max.x; ++iter.x) {
                auto& chunk = chunks.at(iter);
                if(chunk.mesh_state == Chunk::BUILT_TRIANGLE) {
                    LUX_LOG("building physics mesh {%zd, %zd, %zd}",
                        iter.x, iter.y, iter.z);
                    auto& mesh = *chunk.mesh;
                    chunk.mesh->physics_mesh = new ChunkPhysicsMesh();
                    auto& p_mesh = *chunk.mesh->physics_mesh;
                    Uns trigs_num = mesh.idxs.len / 3;
                    p_mesh.verts.resize(mesh.verts.len);
                    p_mesh.idxs = mesh.idxs;
                    LUX_ASSERT(p_mesh.idxs.len > 0);
                    for(Uns i = 0; i < p_mesh.verts.len; ++i) {
                        p_mesh.verts[i] =
                            fixed_to_float<4, U16, 3>(mesh.verts[i].pos);
                    }
                    p_mesh.trigs_array.init(
                        trigs_num, (I32*)p_mesh.idxs.beg , sizeof(I32) * 3,
                        p_mesh.verts.len, (F32*)p_mesh.verts.beg, sizeof(Vec3F));

                    p_mesh.shape.init(
                        &*p_mesh.trigs_array, true, btVector3{0, 0, 0},
                        btVector3{CHK_SIZE, CHK_SIZE, CHK_SIZE});

                    p_mesh.body = physics_create_mesh(to_map_pos(iter, 0),
                        &*p_mesh.shape);
                    chunk.mesh_state = Chunk::BUILT_PHYSICS;
                }
            }
        }
    }
}

Chunk const& get_chunk(ChkPos const& pos) {
    LUX_ASSERT(is_chunk_loaded(pos));
    ///wish there was a non-bound-checking way to do it
    return chunks.at(pos);
}

Block& write_block(MapPos const& pos) {
    ChkPos chk_pos = to_chk_pos(pos);
    LUX_ASSERT(is_chunk_loaded(chk_pos));
    updated_chunks.emplace(chk_pos);
    Chunk& chunk = chunks.at(chk_pos);
    ChkIdx chk_idx = to_chk_idx(pos);
    chunk.updated_blocks.insert(chk_idx);
    return chunk.blocks[chk_idx];
}

Block get_block(MapPos const& pos) {
    return get_chunk(to_chk_pos(pos)).blocks[to_chk_idx(pos)];
}

BlockBp const& get_block_bp(MapPos const& pos) {
    return db_block_bp(get_block(pos).id);
}

void map_tick() {
    synchronize_loader_results();
    synchronize_loader_block_changes();
    synchronize_mesher_results();

    static Uns tick_num = 0;
    physics_tick(1.f / 64.f); //TODO tick time
    for(auto const& pos : updated_chunks) {
        /*if(physics_meshes.count(pos) > 0) {
            U8 updated_sides = 0b000000;
            LUX_ASSERT(is_chunk_loaded(pos));
            Chunk const& chunk = get_chunk(pos);
            for(auto const& idx : chunk.updated_blocks) {
                IdxPos idx_pos = to_idx_pos(idx);
                for(Uns a = 0; a < 3; ++a) {
                    if(idx_pos[a] == 0) {
                        updated_sides |= (0b000001 << (a * 2));
                    } else if(idx_pos[a] == CHK_SIZE - 1) {
                        updated_sides |= (0b000010 << (a * 2));
                    }
                }
            }
            auto update_mesh = [&](ChkPos const& r_pos) {
                ChkPos off_pos = r_pos + pos;
                if(physics_meshes.count(off_pos) > 0) {
                    build_physics_mesh(off_pos);
                }
            };
            update_chunks_around(update_mesh, updated_sides);
        }*/
        LUX_UNIMPLEMENTED();
    }
    Uns constexpr ticks_per_day = 1 << 12;
    day_cycle = 1.f;
    /*day_cycle = std::sin(tau *
        (((F32)(tick_num % ticks_per_day) / (F32)ticks_per_day) + 0.25f));*/
    tick_num++;
}

bool map_cast_ray(MapPos* out_pos, Vec3F* out_dir, Vec3F src, Vec3F dst) {
    Vec3F ray = dst - src;
    Vec3F a = abs(ray);
    int max_i;
    if(a.x > a.y) {
        if(a.x > a.z) max_i = 0;
        else          max_i = 2;
    } else {
        if(a.y > a.z) max_i = 1;
        else          max_i = 2;
    }
    F32 max = a[max_i];
    Vec3F dt = ray / max;
    Vec3F fr = src - trunc(src);
    F32    o = fr[max_i];

    Vec3F it = src + o * dt;

    for(Uns i = 0; i < max; ++i)
    {
        MapPos map_pos = floor(it);
        ChkPos chk_pos = to_chk_pos(map_pos);
        if(!is_chunk_loaded(chk_pos)) return false;
        if(get_chunk(chk_pos).blocks[to_chk_idx(map_pos)].lvl >= 0x8) {
            *out_pos = map_pos;
            Vec3F norm(0.f);
            norm[max_i] = sign(ray[max_i]);
            *out_dir = norm;
            return true;
        }

        it += dt;
    }
    return false;
}

static bool prepare_mesher_data(MesherRequest& out) {
    ChkPos const& pos = out.pos;
    bool has_any_faces = false;
    int face_check_sign;
    auto get_block_l = [&](Vec3I pos) -> Block& {
        pos += (Int)1;
        Int idx = pos.x + (pos.y + pos.z * (CHK_SIZE + 3)) * (CHK_SIZE + 3);
        LUX_ASSERT(idx < (Int)arr_len(out.blocks) && idx >= 0);
        return out.blocks[idx];
    };
    //@TODO abstract this code?
    {   Chunk const& chk = get_chunk(pos + ChkPos(-1, 0, 0));
        for(Uns z = 0; z < CHK_SIZE; ++z) {
            for(Uns y = 0; y < CHK_SIZE; ++y) {
                auto const& block = chk.blocks[to_chk_idx(IdxPos{CHK_SIZE - 1, y, z})];
                get_block_l({-1, y, z}) = block;
            }
        }
    }
    {   Chunk const& chk = get_chunk(pos + ChkPos(0, -1, 0));
        for(Uns z = 0; z < CHK_SIZE; ++z) {
            for(Uns x = 0; x < CHK_SIZE; ++x) {
                auto const& block = chk.blocks[to_chk_idx(IdxPos{x, CHK_SIZE - 1, z})];
                get_block_l({x, -1, z}) = block;
            }
        }
    }
    {   Chunk const& chk = get_chunk(pos + ChkPos(0, 0, -1));
        for(Uns y = 0; y < CHK_SIZE; ++y) {
            for(Uns x = 0; x < CHK_SIZE; ++x) {
                auto const& block = chk.blocks[to_chk_idx(IdxPos{x, y, CHK_SIZE - 1})];
                get_block_l({x, y, -1}) = block;
            }
        }
    }
    {   Chunk const& chk = get_chunk(pos + ChkPos(1, 0, 0));
        {   auto const& block =
                chk.blocks[to_chk_idx(IdxPos{0, 0, 0})];
            get_block_l({CHK_SIZE, 0, 0}) = block;
            face_check_sign = block.lvl >= 8;
        }
        for(Uns y = 1; y < CHK_SIZE; ++y) {
            {   auto const& block = chk.blocks[to_chk_idx(IdxPos{0, y, 0})];
                get_block_l({CHK_SIZE, y, 0}) = block;
                has_any_faces |= ((block.lvl >= 8) != face_check_sign);
            }
            {   auto const& block = chk.blocks[to_chk_idx(IdxPos{1, y, 0})];
                get_block_l({CHK_SIZE + 1, y, 0}) = block;
            }
        }
        for(Uns z = 1; z < CHK_SIZE; ++z) {
            for(Uns y = 0; y < CHK_SIZE; ++y) {
                {   auto const& block = chk.blocks[to_chk_idx(IdxPos{0, y, z})];
                    get_block_l({CHK_SIZE, y, z}) = block;
                    has_any_faces |= ((block.lvl >= 8) != face_check_sign);
                }
                {   auto const& block = chk.blocks[to_chk_idx(IdxPos{1, y, z})];
                    get_block_l({CHK_SIZE + 1, y, z}) = block;
                }
            }
        }
    }
    {   Chunk const& chk = get_chunk(pos + ChkPos(0, 1, 0));
        for(Uns z = 0; z < CHK_SIZE; ++z) {
            for(Uns x = 0; x < CHK_SIZE; ++x) {
                auto const& block = chk.blocks[to_chk_idx(IdxPos{x, 0, z})];
                get_block_l({x, CHK_SIZE, z}) = block;
                has_any_faces |= ((block.lvl >= 8) != face_check_sign);
            }
            for(Uns x = 0; x < CHK_SIZE; ++x) {
                auto const& block = chk.blocks[to_chk_idx(IdxPos{x, 1, z})];
                get_block_l({x, CHK_SIZE + 1, z}) = block;
            }
        }
    }
    {   Chunk const& chk = get_chunk(pos + ChkPos(0, 0, 1));
        for(Uns y = 0; y < CHK_SIZE; ++y) {
            for(Uns x = 0; x < CHK_SIZE; ++x) {
                auto const& block = chk.blocks[to_chk_idx(IdxPos{x, y, 0})];
                get_block_l({x, y, CHK_SIZE}) = block;
                has_any_faces |= ((block.lvl >= 8) != face_check_sign);
            }
        }
        for(Uns y = 0; y < CHK_SIZE; ++y) {
            for(Uns x = 0; x < CHK_SIZE; ++x) {
                auto const& block = chk.blocks[to_chk_idx(IdxPos{x, y, 1})];
                get_block_l({x, y, CHK_SIZE + 1}) = block;
            }
        }
    }
    {   Chunk const& chk = get_chunk(pos + ChkPos(0, 1, 1));
        for(Uns x = 0; x < CHK_SIZE; ++x) {
            auto const& block = chk.blocks[to_chk_idx(IdxPos{x, 0, 0})];
            get_block_l({x, CHK_SIZE, CHK_SIZE}) = block;
            has_any_faces |= ((block.lvl >= 8) != face_check_sign);
        }
        for(Uns x = 0; x < CHK_SIZE; ++x) {
            auto const& block = chk.blocks[to_chk_idx(IdxPos{x, 1, 0})];
            get_block_l({x, CHK_SIZE + 1, CHK_SIZE}) = block;
        }
        for(Uns x = 0; x < CHK_SIZE; ++x) {
            auto const& block = chk.blocks[to_chk_idx(IdxPos{x, 0, 1})];
            get_block_l({x, CHK_SIZE, CHK_SIZE + 1}) = block;
        }
    }
    {   Chunk const& chk = get_chunk(pos + ChkPos(1, 0, 1));
        for(Uns y = 0; y < CHK_SIZE; ++y) {
            auto const& block = chk.blocks[to_chk_idx(IdxPos{0, y, 0})];
            get_block_l({CHK_SIZE, y, CHK_SIZE}) = block;
            has_any_faces |= ((block.lvl >= 8) != face_check_sign);
        }
        for(Uns y = 0; y < CHK_SIZE; ++y) {
            auto const& block = chk.blocks[to_chk_idx(IdxPos{1, y, 0})];
            get_block_l({CHK_SIZE + 1, y, CHK_SIZE}) = block;
        }
        for(Uns y = 0; y < CHK_SIZE; ++y) {
            auto const& block = chk.blocks[to_chk_idx(IdxPos{0, y, 1})];
            get_block_l({CHK_SIZE, y, CHK_SIZE + 1}) = block;
        }
    }
    {   Chunk const& chk = get_chunk(pos + ChkPos(1, 1, 0));
        for(Uns z = 0; z < CHK_SIZE; ++z) {
            auto const& block = chk.blocks[to_chk_idx(IdxPos{0, 0, z})];
            get_block_l({CHK_SIZE, CHK_SIZE, z}) = block;
            has_any_faces |= ((block.lvl >= 8) != face_check_sign);
        }
        for(Uns z = 0; z < CHK_SIZE; ++z) {
            auto const& block = chk.blocks[to_chk_idx(IdxPos{1, 0, z})];
            get_block_l({CHK_SIZE + 1, CHK_SIZE, z}) = block;
        }
        for(Uns z = 0; z < CHK_SIZE; ++z) {
            auto const& block = chk.blocks[to_chk_idx(IdxPos{0, 1, z})];
            get_block_l({CHK_SIZE, CHK_SIZE + 1, z}) = block;
        }
    }
    {   Chunk const& chk = get_chunk(pos + ChkPos(1, 1, 1));
        {   auto const& block = chk.blocks[to_chk_idx(IdxPos{0, 0, 0})];
            get_block_l({CHK_SIZE, CHK_SIZE, CHK_SIZE}) = block;
            has_any_faces |= ((block.lvl >= 8) != face_check_sign);
        }
        {   auto const& block = chk.blocks[to_chk_idx(IdxPos{1, 0, 0})];
            get_block_l({CHK_SIZE + 1, CHK_SIZE, CHK_SIZE}) = block;
            has_any_faces |= ((block.lvl >= 8) != face_check_sign);
        }
        {   auto const& block = chk.blocks[to_chk_idx(IdxPos{0, 1, 0})];
            get_block_l({CHK_SIZE, CHK_SIZE + 1, CHK_SIZE}) = block;
            has_any_faces |= ((block.lvl >= 8) != face_check_sign);
        }
        {   auto const& block = chk.blocks[to_chk_idx(IdxPos{0, 0, 1})];
            get_block_l({CHK_SIZE, CHK_SIZE, CHK_SIZE + 1}) = block;
            has_any_faces |= ((block.lvl >= 8) != face_check_sign);
        }
    }
    {   Chunk const& chk = get_chunk(pos + ChkPos(-1, 1, 0));
        for(Uns z = 0; z < CHK_SIZE; ++z) {
            auto const& block = chk.blocks[to_chk_idx(IdxPos{CHK_SIZE - 1, 0, z})];
            get_block_l({-1, CHK_SIZE, z}) = block;
        }
    }
    {   Chunk const& chk = get_chunk(pos + ChkPos(-1, 0, 1));
        for(Uns y = 0; y < CHK_SIZE; ++y) {
            auto const& block = chk.blocks[to_chk_idx(IdxPos{CHK_SIZE - 1, y, 0})];
            get_block_l({-1, y, CHK_SIZE}) = block;
        }
    }
    {   Chunk const& chk = get_chunk(pos + ChkPos(-1, 1, 1));
        auto const& block = chk.blocks[to_chk_idx(IdxPos{CHK_SIZE - 1, 0, 0})];
        get_block_l({-1, CHK_SIZE, CHK_SIZE}) = block;
    }
    {   Chunk const& chk = get_chunk(pos + ChkPos(1, -1, 0));
        for(Uns z = 0; z < CHK_SIZE; ++z) {
            auto const& block = chk.blocks[to_chk_idx(IdxPos{0, CHK_SIZE - 1, z})];
            get_block_l({CHK_SIZE, -1, z}) = block;
        }
    }
    {   Chunk const& chk = get_chunk(pos + ChkPos(0, -1, 1));
        for(Uns x = 0; x < CHK_SIZE; ++x) {
            auto const& block = chk.blocks[to_chk_idx(IdxPos{x, CHK_SIZE - 1, 0})];
            get_block_l({x, -1, CHK_SIZE}) = block;
        }
    }
    {   Chunk const& chk = get_chunk(pos + ChkPos(1, -1, 1));
        auto const& block = chk.blocks[to_chk_idx(IdxPos{0, CHK_SIZE - 1, 0})];
        get_block_l({CHK_SIZE, -1, CHK_SIZE}) = block;
    }
    {   Chunk const& chk = get_chunk(pos + ChkPos(0, 1, -1));
        for(Uns x = 0; x < CHK_SIZE; ++x) {
            auto const& block = chk.blocks[to_chk_idx(IdxPos{x, 0, CHK_SIZE - 1})];
            get_block_l({x, CHK_SIZE, -1}) = block;
        }
    }
    {   Chunk const& chk = get_chunk(pos + ChkPos(1, 0, -1));
        for(Uns y = 0; y < CHK_SIZE; ++y) {
            auto const& block = chk.blocks[to_chk_idx(IdxPos{0, y, CHK_SIZE - 1})];
            get_block_l({CHK_SIZE, y, -1}) = block;
        }
    }
    {   Chunk const& chk = get_chunk(pos + ChkPos(1, 1, -1));
        auto const& block = chk.blocks[to_chk_idx(IdxPos{0, 0, CHK_SIZE - 1})];
        get_block_l({CHK_SIZE, CHK_SIZE, -1}) = block;
    }
    Chunk const& chk = get_chunk(pos);
    Uns idx = 0;
    for(Uns z = 0; z < CHK_SIZE; ++z) {
        for(Uns y = 0; y < CHK_SIZE; ++y) {
            for(Uns x = 0; x < CHK_SIZE; ++x) {
                auto const& block = chk.blocks[idx];
                get_block_l({x, y, z}) = block;
                has_any_faces |= ((block.lvl >= 8) != face_check_sign);
                idx++;
            }
        }
    }
    return has_any_faces;
}
