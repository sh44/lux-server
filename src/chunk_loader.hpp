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

VecMap<ChkPos, DynArr<BlockChange>>& loader_lock_block_changes();
void loader_unlock_block_changes();
VecMap<ChkPos, Arr<Block, CHK_VOL>> const& loader_lock_results();
void loader_unlock_results();
