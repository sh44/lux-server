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

void add_dbg_point(NetSsTick::DbgInf::Shape::Point const& val,
    Vec4F col = {1.f, 0.f, 1.f, 0.5f}, bool border = true);
void add_dbg_line(NetSsTick::DbgInf::Shape::Line const& val,
    Vec4F col = {1.f, 0.f, 1.f, 0.5f}, bool border = true);
void add_dbg_arrow(NetSsTick::DbgInf::Shape::Arrow const& val,
    Vec4F col = {1.f, 0.f, 1.f, 0.5f}, bool border = true);
void add_dbg_cross(NetSsTick::DbgInf::Shape::Cross const& val,
    Vec4F col = {1.f, 0.f, 1.f, 0.5f}, bool border = true);
void add_dbg_sphere(NetSsTick::DbgInf::Shape::Sphere const& val,
    Vec4F col = {1.f, 0.f, 1.f, 0.5f}, bool border = true);
void add_dbg_triangle(NetSsTick::DbgInf::Shape::Triangle const& val,
    Vec4F col = {1.f, 0.f, 1.f, 0.5f}, bool border = true);
void add_dbg_rect(NetSsTick::DbgInf::Shape::Rect const& val,
    Vec4F col = {1.f, 0.f, 1.f, 0.5f}, bool border = true);

extern NetCsSgnl cs_sgnl;
extern NetCsTick cs_tick;
extern NetSsInit ss_init;
extern NetSsSgnl ss_sgnl;
extern NetSsTick ss_tick;
