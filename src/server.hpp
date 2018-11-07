#pragma once

#include <cstdint>
//
#include <lux_shared/common.hpp>
#include <lux_shared/map.hpp>

typedef std::uintptr_t ClientId;

bool is_client_connected(ClientId id);
void server_init(U16 server_port, F64 tick_rate);
void server_deinit();
void server_tick(VecSet<ChkPos> const& light_updated_chunks);

void kick_client(ClientId id, char const* reason);

void server_broadcast(char const* str);
bool server_is_running();
void server_quit();
ClientId get_client_id(char const* name);
void server_make_admin(ClientId id);
