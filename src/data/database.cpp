#include "database.hpp"

namespace data
{

world::entity::Type const &Database::entity_type_at(String const &id) const
{
    return *entity_types.at(id);
}

world::tile::Type const &Database::tile_type_at(String const &id) const
{
    return *tile_types.at(id);
}

}
