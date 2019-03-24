#pragma once

#include <lux_shared/map.hpp>
//
#include <map.hpp>

void loader_init();
void loader_deinit();

void loader_enqueue(Slice<ChkPos> const& chunks);
void loader_enqueue_wait(Slice<ChkPos> const& chunks);
void loader_write_suspended_block(Block const& block, MapPos const& pos);

struct BlockChange {
    ChkIdx  idx;
    Block block;
};

//@TODO use typedef
typedef VecMap<ChkPos, Chunk::Data*>        LoaderResults;
typedef VecMap<ChkPos, DynArr<BlockChange>> LoaderBlockChanges;

bool loader_try_lock_block_changes(VecMap<ChkPos, DynArr<BlockChange>>*& out);
bool loader_try_lock_results(VecMap<ChkPos, Chunk::Data*>*& out);
VecMap<ChkPos, DynArr<BlockChange>>& loader_lock_block_changes();
void loader_unlock_block_changes();
VecMap<ChkPos, Chunk::Data*> const& loader_lock_results();
void loader_unlock_results();
