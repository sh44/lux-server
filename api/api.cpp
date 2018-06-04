#include <alias/string.hpp>
#include <alias/hash_map.hpp>
#include <util/log.hpp>
#include <world/entity/type.hpp>
#include "api.hpp"

extern "C"
{

void hash_map_insert(ApiHashMap *hash_map, ApiCString key, void *val)
{
    (*(HashMap<String, void *> *)hash_map)[key] = val;
}

void log_msg(ApiLogLevel level, ApiCString msg)
{
    util::log("LUA", (util::Level)level, msg);
}

}
