#include <alias/string.hpp>
#include <util/log.hpp>
#include <world/entity/type.hpp>
#include "api.hpp"

extern "C"
{

void log_msg(LogLevel level, const char *msg) {util::log("LUA", (util::Level)level, msg);}

void *entity_type_ctor(const char *name)
{
    util::log("LUA", util::DEBUG, "created new entity type: " + String(name));
    return new world::entity::Type{name};
}
void entity_type_dtor(void *ptr) {delete (world::entity::Type *)ptr;}
const char *entity_type_name(void *ptr) {return ((world::entity::Type *)ptr)->name;}

}
