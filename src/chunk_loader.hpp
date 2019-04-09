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

typedef VecMap<ChkPos, Chunk::Data*>        LoaderResults;
typedef VecMap<ChkPos, DynArr<BlockChange>> LoaderBlockChanges;

bool loader_try_lock_block_changes(LoaderBlockChanges*& out);
bool loader_try_lock_results(LoaderResults*& out);
LoaderBlockChanges& loader_lock_block_changes();
void loader_unlock_block_changes();
LoaderResults const& loader_lock_results();
void loader_unlock_results();
