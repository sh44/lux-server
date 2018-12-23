#pragma once

#include <lux_shared/common.hpp>

SizeT constexpr RN_STACK_LEN = 0x100;
SizeT constexpr RN_RAM_LEN   = 0x100;
SizeT constexpr RN_ROM_LEN   = 0x1000;

struct RasenFrame {
    Slice<U8> stack;
    Slice<U8> sys_mem;

    LUX_MAY_FAIL pop(U8* out);
    LUX_MAY_FAIL push(U8 val);
    LUX_MAY_FAIL read_sys_mem(U8* out, U8 addr);
    LUX_MAY_FAIL write_sys_mem(U8 val, U8 addr);
};

struct RasenExtEnv {
    typedef LuxRval (*ExtFunc)(RasenFrame*);
    HashMap<StrBuff, U16> symbol_table;
    DynArr<ExtFunc>       funcs;
    LUX_MAY_FAIL call(U16 id, RasenFrame* frame);
    LUX_MAY_FAIL register_func(Str const& str_id, ExtFunc func);
};

struct RasenEnv {
    Arr<U8, RN_RAM_LEN> ram;

    struct FuncId {
        enum Tag : U8 {
            LOCAL,
            INTERNAL_PARENT,
            EXTERNAL_PARENT,
        } tag;
        U16 func_id;
        ///the parent_id is ignored if the tag is LOCAL (no parent)
        U16 parent_id;
    };
    ///this maps to an index for 'funcs_lookup'
    HashMap<StrBuff, U16>    symbol_table;
    DynArr<FuncId>           available_funcs;
    DynArr<DynArr<U16>>      local_funcs;
    DynArr<RasenEnv*>        parents;
    DynArr<RasenExtEnv*> ext_parents;
    U8  r[16];
    U16 pc;
    ///means the environment has special privileges to use the system functions
    bool system = false;

    LUX_MAY_FAIL exec(U16 word, RasenFrame* frame);
    LUX_MAY_FAIL exec_rom(Slice<U16> const& rom, RasenFrame* frame);
    LUX_MAY_FAIL call(U16 id, RasenFrame* frame);
    LUX_MAY_FAIL call(Str const& str_id, RasenFrame* frame);
    LUX_MAY_FAIL register_func(Str const& str_id, Str const& assembly);
    LUX_MAY_FAIL add_internal_parent(RasenEnv&    parent);
    LUX_MAY_FAIL add_external_parent(RasenExtEnv& parent);

    LUX_MAY_FAIL read_mem(U8* out, U8 addr);
    LUX_MAY_FAIL write_mem(U8 val, U8 addr);

    LUX_MAY_FAIL rasm(DynArr<U16>* out, Str const& in);
};

void rasen_init();
