#pragma once

#include <lux_shared/common.hpp>
#include <lux_shared/rasen.hpp>

struct Rasen;

SizeT constexpr RN_STACK_LEN = 0x100;
SizeT constexpr RN_RAM_LEN   = 0x100;
SizeT constexpr RN_ROM_LEN   = 0x1000;
SizeT constexpr RN_REG_LEN   = 0x10;
SizeT constexpr RN_XCS_LEN   = 0x1000;

typedef void Extcall(Rasen*);

Extcall extcall_err;
Extcall extcall_halt;
Extcall extcall_print;
Extcall extcall_entity_rotate;
Extcall extcall_entity_move;
Extcall extcall_entity_get_pos;
Extcall extcall_entity_get_rotation;

Arr<Extcall*, RN_XCS_LEN> constexpr extcalls = {
    &extcall_halt,
    &extcall_err,
    &extcall_print,
    &extcall_entity_rotate,
    &extcall_entity_move,
    &extcall_entity_get_pos,
    &extcall_entity_get_rotation,
    nullptr,
};

struct Rasen {
    Arr<U8 , RN_STACK_LEN> s = {0x00};
    Arr<U8 , RN_RAM_LEN>   m = {0x00};
    Arr<U16, RN_ROM_LEN>   o = {0x0000};
    union {
        Arr<U8, RN_REG_LEN> r;
        struct {
            Arr<U8, RN_REG_LEN - 2> pad0;
            U8 sp = 0;
            U8 cr = 0;
        };
    };
    U16 pc = 0x000;
    void run();
    void tick();
    U16 fetch();
    void exec(U16 word);
    void err(U8 code);
    void push(U8 val);
    U8   pop();

    bool running = false;
};
