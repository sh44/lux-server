#pragma once

namespace entity { struct Type; }

namespace data
{

struct Database;

struct Config
{
    Database     const *db;
    entity::Type const *player_type;
};

}
