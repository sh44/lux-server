#include <functional>
//
#include <lux_shared/common.hpp>
//
#include "db.hpp"

static DynArr<BlockBp> blocks;
//@CONSIDER use regular map?
static HashMap<Str, BlockId> blocks_lookup;

void add_block(BlockBp &&block_type) {
    auto &block = blocks.push(std::move(block_type));
    blocks_lookup[block.str_id] = blocks.len - 1;
    LUX_LOG("added block %zu: \"%.*s\"",
            blocks.len - 1, (int)block.str_id.len, block.str_id.beg);
}

void db_init() {
    add_block({"void"_l});
    add_block({"stone_floor"_l});
    add_block({"stone_wall"_l});
    add_block({"raw_stone"_l});
    add_block({"dirt"_l});
    add_block({"grass"_l});
    add_block({"dark_grass"_l});
    add_block({"tree_trunk"_l});
    add_block({"tree_leaves"_l});
}

BlockBp const& db_block_bp(BlockId id) {
    LUX_ASSERT(id < blocks.len);
    return blocks[id];
}

BlockBp const& db_block_bp(Str const& str_id) {
    return db_block_bp(db_block_id(str_id));
}

BlockId const& db_block_id(Str const& str_id) {
    LUX_ASSERT(blocks_lookup.count(str_id) > 0);
    return blocks_lookup.at(str_id);
}
