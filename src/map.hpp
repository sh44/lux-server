#pragma once

#include <lux_shared/map.hpp>
//
#include <db.hpp>

struct Block {
    BlockId  id;
};

struct ChunkPhysicsMesh;

struct ChunkMesh {
    struct Face {
        ChkIdx  idx;
        BlockId id;
        U8      orientation; //first bit is sign, 2 next bits are axis (XYZ)
    };
    DynArr<Face> faces;

    ChunkPhysicsMesh* physics_mesh;

    ChunkMesh() = default;
    ChunkMesh(ChunkMesh&& that);
};

struct Chunk {
    struct Data {
        Arr<Block, CHK_VOL> blocks;
    };
    //@URGENT we need to deallocate this (using lux_dealloc) when unloading
    Data* data;
    IdSet<ChkIdx> updated_blocks; //@TODO change this?
    ChunkMesh* mesh;

    enum MeshState : U8 {
        NOT_BUILT,
        BUILT_EMPTY,
        BUILT_TRIANGLE,
        BUILT_PHYSICS
    } mesh_state = NOT_BUILT;

    Block       &operator[](ChkIdx idx);
    Block const &operator[](ChkIdx idx) const;
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
