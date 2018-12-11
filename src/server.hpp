#pragma once

#include <cstdint>
//
#include <lux_shared/common.hpp>
#include <lux_shared/map.hpp>

typedef std::uintptr_t ClientId;

bool is_client_connected(ClientId id);
void server_init(U16 server_port, F64 tick_rate);
void server_deinit();
void server_tick();

void kick_client(ClientId id, Str reason);

void server_broadcast(Str str);
bool server_is_running();
void server_quit();
ClientId get_client_id(Str name);
void server_make_admin(ClientId id);

extern NetCsSgnl cs_sgnl;
extern NetCsTick cs_tick;
extern NetSsInit ss_init;
extern NetSsSgnl ss_sgnl;
extern NetSsTick ss_tick;
