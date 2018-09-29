#pragma once

#include <lux_shared/common.hpp>
#include <lux_shared/map.hpp>

void server_init(U16 server_port, F64 tick_rate);
void server_deinit();
void server_tick(DynArr<ChkPos> const& light_updated_chunks);

void kick_client(char const* name, char const* reason);

void server_broadcast(char const* str);
bool server_is_running();
void server_quit();
LUX_MAY_FAIL server_make_admin(char const* name);
