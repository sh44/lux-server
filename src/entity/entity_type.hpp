#pragma once

#include <lux/alias/scalar.hpp>
#include <lux/alias/string.hpp>
#include <lux/world/entity.hpp>

struct EntityType
{
    EntityType(String const &_str_id, String const &_name) :
        str_id(_str_id), name(_name)
    {

    }

    String str_id;
    String name;
};
