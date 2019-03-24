#pragma once

#include <lux_shared/common.hpp>
#include <lux_shared/map.hpp>
//
#include <map.hpp>

struct MesherRequest {
    typedef Arr<Block, (CHK_SIZE + 1) * (CHK_SIZE + 1) * (CHK_SIZE + 1)>
        InputData;

    ChkPos pos;
    InputData blocks;
};
//
//@TODO use typedef
typedef VecMap<ChkPos, ChunkMesh> MesherResults;

void mesher_init();
void mesher_deinit();

void mesher_enqueue(DynArr<MesherRequest>&& data);
void mesher_enqueue_wait(DynArr<MesherRequest>&& data);

VecMap<ChkPos, ChunkMesh>& mesher_lock_results();
bool mesher_try_lock_results(MesherResults*& out);
void mesher_unlock_results();
