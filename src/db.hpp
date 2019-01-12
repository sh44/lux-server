#pragma once

#include <lux_shared/common.hpp>
#include <lux_shared/map.hpp>
//
#include <entity.hpp>

struct BlockBp {
    BlockBp(Str str) :
        str_id(str) { }
    StrBuff str_id;
};

void db_init();
BlockBp const& db_block_bp(BlockId id);
BlockBp const& db_block_bp(Str const& str_id);
BlockId const& db_block_id(Str const& str_id);
