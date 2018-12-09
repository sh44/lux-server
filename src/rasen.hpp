#pragma once

#include <lux_shared/common.hpp>
#include <lux_shared/rasen.hpp>

SizeT constexpr RN_STACK_LEN = 0x100;
SizeT constexpr RN_RAM_LEN   = 0x100;
SizeT constexpr RN_ROM_LEN   = 0x1000;
SizeT constexpr RN_REG_LEN   = 0x10;
SizeT constexpr RN_XCS_LEN   = 0x1000;

struct RasenStack {
    Arr<U8, RN_STACK_LEN>* val;
    U8*                    sp;
};

struct RasenExtEnv {
    typedef void (*ExtFunc)(RasenStack const&);
    HashMap<Str, U16> funcs_lookup;
    DynArr<ExtFunc>      funcs;
};

struct RasenEnv {
    Arr<U8, RN_RAM_LEN> ram;

    HashMap<Str, U16> funcs_lookup;
    DynArr<DynArr<U16>>  funcs;
    enum Tag {
        NONE,
        EXTERNAL,
        INTERNAL,
    } parent_tag = NONE;
    union {
        RasenEnv*    int_parent;
        RasenExtEnv* ext_parent;
    };
    U8  r[16];
    U16 pc;

    void exec(U16 word, RasenStack const& stack);
    void exec_rom(Slice<U16> const& rom, RasenStack const& s);
    void call(U16 id, RasenStack const& s);
    void call(Str const& str_id, RasenStack const& s);
    void err(U8 code, RasenStack const& stack);
};

