#pragma once

#include <lux_shared/common.hpp>
#include <lux_shared/map.hpp>
//
#include <map.hpp>

struct MesherRequest {
    typedef Arr<Block, (CHK_SIZE + 3) * (CHK_SIZE + 3) * (CHK_SIZE + 3)>
        InputData;

    ChkPos pos;
    InputData blocks;
};

void mesher_init();
void mesher_deinit();

void mesher_enqueue(DynArr<MesherRequest>&& data);
void mesher_enqueue_wait(DynArr<MesherRequest>&& data);

VecMap<ChkPos, ChunkMesh>& mesher_lock_results();
void mesher_unlock_results();
