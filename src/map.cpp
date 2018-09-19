#include <lux_shared/common.hpp>
#include <lux_shared/util/packer.hpp>
#include <lux_shared/map.hpp>
//
#include <db.hpp>
#include "map.hpp"

HashMap<ChkPos, Chunk, util::Packer<ChkPos>> chunks;

void map_tick() {
    //@TODO
}

bool is_chunk_loaded(ChkPos const& pos) {
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
        if(map_pos.x % 8 == 0 || map_pos.y % 8 == 0) {
            voxel_id = db_voxel_id("stone_wall");
        } else {
            voxel_id = db_voxel_id("stone_floor");
        }
        chunk.voxels[i] = voxel_id;
        chunk.light_lvls[i] = 0;
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
