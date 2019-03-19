#pragma once

#include <lux_shared/map.hpp>
//
#include <db.hpp>

struct Block {
    BlockId  id;
    BlockLvl lvl;
};

/*struct BlockChange {
    enum TypeFlags : U8 {
        NO_ID   = 0b00, //id does not change
        ID      = 0b01, //we change id
        LVL_REL = 0b00, //relative lvl change (can be 0)
        LVL_ABS = 0b10,
    } type;

    BlockId  id;
    I16      lvl;
};*/

struct ChunkPhysicsMesh;

struct ChunkMesh {
    struct Vert {
        Vec3<U16> pos;  ///12.4 fixed point
        Vec3<U8>  norm;  ///sign.7 fixed point
        BlockId   id;
    };
    DynArr<Vert> verts;
    DynArr<U16>   idxs;

    ChunkPhysicsMesh* physics_mesh;

    ChunkMesh() = default;
    ChunkMesh(ChunkMesh&& that);
};

struct Chunk {
    Arr<Block   , CHK_VOL> blocks;
    IdSet<ChkIdx> updated_blocks; //@TODO change this?
    ChunkMesh* mesh;

    enum MeshState : U8 {
        NOT_BUILT,
        BUILT_EMPTY,
        BUILT_TRIANGLE,
        BUILT_PHYSICS
    } mesh_state = NOT_BUILT;

    ~Chunk();
};

extern F32 day_cycle;
extern VecSet<ChkPos> updated_chunks;

Block get_block(MapPos const& pos);
BlockBp const& get_block_bp(MapPos const& pos);

void map_init();
void map_deinit();
void map_tick();
void guarantee_chunk(ChkPos const& pos);
bool try_guarantee_chunk(ChkPos const& pos);
bool try_guarantee_chunk_mesh(ChkPos const& pos);
void enqueue_missing_chunks_meshes(VecSet<ChkPos> const& requests);
void guarantee_physics_mesh_for_aabb(MapPos const& min, MapPos const& max);
Chunk const& get_chunk(ChkPos const& pos);
Block& write_block(MapPos const& pos);

bool map_cast_ray(MapPos* out_pos, Vec3F* out_dir, Vec3F src, Vec3F dst);
