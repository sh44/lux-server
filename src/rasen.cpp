#include <config.hpp>
#include <cstring>
//
#include <glm/gtx/rotate_vector.hpp>
//
#include <lux_shared/common.hpp>
//
#include <entity.hpp>
#include "rasen.hpp"

LUX_MAY_FAIL RasenFrame::pop(U8* out) {
    LUX_RETHROW(stack.len > 0);
    LUX_ASSERT(stack.len <= RN_STACK_LEN);
    *out = stack.beg[--stack.len];
    return LUX_OK;
}

LUX_MAY_FAIL RasenFrame::push(U8 val) {
    LUX_RETHROW(stack.len < RN_STACK_LEN);
    stack.beg[stack.len++] = val;
    return LUX_OK;
}

LUX_MAY_FAIL RasenFrame::read_sys_mem(U8* out, U8 addr) {
    LUX_RETHROW(addr < sys_mem.len);
    *out = sys_mem[addr];
    return LUX_OK;
}

LUX_MAY_FAIL RasenFrame::write_sys_mem(U8 val, U8 addr) {
    LUX_RETHROW(addr < sys_mem.len);
    sys_mem[addr] = val;
    return LUX_OK;
}

LUX_MAY_FAIL RasenExtEnv::call(U16 id, RasenFrame* frame) {
    LUX_RETHROW(id < funcs.len);
    return (*funcs[id])(frame);
}

LUX_MAY_FAIL RasenExtEnv::register_func(Str const& str_id, ExtFunc func) {
    if(symbol_table.count(str_id) > 0) {
        funcs[symbol_table[str_id]] = func;
    } else {
        funcs.emplace(func);
        symbol_table[str_id] = funcs.len - 1;
    }
    return LUX_OK;
}

LUX_MAY_FAIL static sys_read_mem(RasenEnv* env, RasenFrame* frame) {
    LUX_RETHROW(env->system);
    U8 addr;
    LUX_RETHROW(frame->pop(&addr));
    U8 val;
    LUX_RETHROW(frame->read_sys_mem(&val, addr));
    return frame->push(val);
}

LUX_MAY_FAIL static sys_write_mem(RasenEnv* env, RasenFrame* frame) {
    LUX_RETHROW(env->system);
    U8 addr;
    LUX_RETHROW(frame->pop(&addr));
    U8 val;
    LUX_RETHROW(frame->pop(&val));
    return frame->write_sys_mem(val, addr);
}

typedef LuxRval (*SysFunc)(RasenEnv*, RasenFrame*);

SizeT constexpr MAX_SYS_FUNCS = 0x100;
static SysFunc constexpr sys_funcs[] = {
    sys_read_mem,
    sys_write_mem,
};

struct {
    HashMap<Str, U8> funcs_lookup;
    LUX_MAY_FAIL call(U8 id, RasenEnv* env, RasenFrame* frame) {
        LUX_RETHROW(id < arr_len(sys_funcs));
        return (*sys_funcs[id])(env, frame);
    }
} static sys_env;

static void register_sys_func(Str const& str) {
    LUX_ASSERT(sys_env.funcs_lookup.count(str) == 0);
    static U8 counter = 0;
    sys_env.funcs_lookup[str] = counter++;
}

void rasen_init() {
    register_sys_func("sys_read_mem"_l);
    register_sys_func("sys_write_mem"_l);
}

LUX_MAY_FAIL RasenEnv::exec(U16 word, RasenFrame* frame) {
    U8  c = (word & 0xF000) >> 12;
    U8  x = (word & 0x0F00) >> 8;
    U8  y = (word & 0x00F0) >> 4;
    U8  z =  word & 0x000F;
    U8  v =  word & 0x00FF;
    U16 t =  word & 0x0FFF;
    U8& cr = r[0xf];
    pc++;
    switch(c) {
        case 0x0: switch(x) {
            case 0x2: r[z]    =  r[y];    break;
            case 0x3: r[z]   &=  r[y];    break;
            case 0x4: r[z]    = ~r[y];    break;
            case 0x5: r[z]   |=  r[y];    break;
            case 0x6: r[z]   ^=  r[y];    break;
            case 0x7: r[z]  >>=  r[y];    break;
            case 0x8: r[z]  <<=  r[y];    break;
            case 0x9: r[z]   +=  r[y];    break;
            case 0xa: r[z]   -=  r[y];    break;
            case 0xb: r[z]   *=  r[y];    break;
            case 0xc: r[z]   /=  r[y];    break;
            case 0xd: r[z]   %=  r[y];    break;
            case 0xe: LUX_RETHROW(read_mem(&r[z], r[y])); break;
            case 0xf: LUX_RETHROW(write_mem(r[z], r[y])); break;
            default: return LUX_FAIL;
        } break;
        case 0x1: switch(x) {
            case 0x0: LUX_RETHROW(frame->push(r[y])); break;
            case 0x1: LUX_RETHROW(frame->pop(&r[y])); break;
            case 0x2: LUX_RETHROW(frame->push(   v)); break;
            default: return LUX_FAIL;
        } break;
        case 0x6: LUX_RETHROW(call(t, frame));     break;
        case 0x7: LUX_RETHROW(read_mem(&r[x], v)); break;
        case 0x8: LUX_RETHROW(write_mem(v, r[x])); break;
        case 0x9: r[x]  = v;    break;
        case 0xa: r[x] += v;    break;
        case 0xb: pc    = t;    break;
        case 0xc: if(    cr == 0) pc = t; break;
        case 0xd: if(    cr != 0) pc = t; break;
        case 0xe: if((I8)cr >  0) pc = t; break;
        case 0xf: if((I8)cr <  0) pc = t; break;
        default: return LUX_FAIL;
    }
    return LUX_OK;
}

LUX_MAY_FAIL RasenEnv::exec_rom(Slice<U16> const& rom, RasenFrame* frame) {
    pc = 0;
    LUX_ASSERT(rom.len < 0x10000);
    while(pc < rom.len) {
        LUX_RETHROW(exec(rom.beg[pc], frame));
    }
    return LUX_OK;
}

LUX_MAY_FAIL RasenEnv::call(U16 id, RasenFrame* frame) {
    ///last 0x100 bytes are reserved for sys calls
    if((id & 0xF00) >> 8 == 0xF) {
        return sys_env.call(id & 0xFF, this, frame);
    }
    LUX_RETHROW(id < available_funcs.len,
        "undefined function #0x%03x", id);
    auto& func = available_funcs[id];
    if(func.tag == FuncId::LOCAL) {
        LUX_ASSERT(func.func_id < local_funcs.len);
        return exec_rom(local_funcs[func.func_id], frame);
    }
    if(func.tag == FuncId::INTERNAL_PARENT) {
        LUX_ASSERT(func.parent_id < parents.len);
        RasenEnv* parent = parents[func.parent_id];
        LUX_ASSERT(func.func_id < parent->local_funcs.len);
        return parent->call(func.func_id, frame);
    }
    if(func.tag == FuncId::EXTERNAL_PARENT) {
        LUX_ASSERT(func.parent_id < ext_parents.len);
        RasenExtEnv* parent = ext_parents[func.parent_id];
        LUX_ASSERT(func.func_id < parent->funcs.len);
        return parent->call(func.func_id, frame);
    }
    LUX_UNREACHABLE();
}

LUX_MAY_FAIL RasenEnv::call(Str const& str_id, RasenFrame* frame) {
    LUX_RETHROW(symbol_table.count(str_id) > 0);
    auto id = symbol_table.at(str_id);
    LUX_ASSERT(id < available_funcs.len);
    return call(id, frame);
}

LUX_MAY_FAIL RasenEnv::register_func(Str const& str_id, Str const& assembly) {
    if(symbol_table.count(str_id) > 0) {
        U16 symbol_id = symbol_table.at(str_id);
        auto& func = available_funcs[symbol_id];
        if(func.tag == FuncId::LOCAL) {
            local_funcs[func.func_id].clear();
            return rasm(&local_funcs[func.func_id], assembly);
        }
        LUX_LOG_WARN("overwriting non-local function \"%.*s\"",
            (int)str_id.len, str_id.beg);
        available_funcs.erase(symbol_id);
        symbol_table.erase(str_id);
    }
    //@TODO magic number (also other cases)
    //@TODO generalize? we repeat those assertions 3+ times
    LUX_RETHROW(available_funcs.len + 1 <= 0x1000 - MAX_SYS_FUNCS,
        "exceeded maximum functions number");
    LUX_ASSERT(local_funcs.len + 1 <= 0x1000 - MAX_SYS_FUNCS);
    local_funcs.emplace();
    LUX_RETHROW(rasm(&local_funcs.last(), assembly));
    available_funcs.push({FuncId::LOCAL, (U16)(local_funcs.len - 1), 0});
    symbol_table[str_id] = available_funcs.len - 1;
    return LUX_OK;
}

LUX_MAY_FAIL RasenEnv::add_internal_parent(RasenEnv& parent) {
    LUX_RETHROW(parents.len < 0x1000,
        "exceeded maximum internal parents number");
    parents.push(&parent);
    U16 parent_id = parents.len - 1;
    for(auto& id : parent.symbol_table) {
        auto& func = parent.available_funcs[id.second];
        if(func.tag == FuncId::LOCAL) {
            LUX_RETHROW(symbol_table.count(id.first) == 0,
                "multiple definitions of function \"%.*s\"",
                (int)id.first.len, id.first.beg);
            LUX_RETHROW(available_funcs.len + 1 <= 0x1000 - MAX_SYS_FUNCS,
                "exceeded maximum functions number");
            LUX_ASSERT(local_funcs.len + 1 <= 0x1000 - MAX_SYS_FUNCS);
            available_funcs.push(
                {FuncId::INTERNAL_PARENT, id.second, parent_id});
            symbol_table[id.first] = available_funcs.len - 1;
        }
    }
    return LUX_OK;
}

LUX_MAY_FAIL RasenEnv::add_external_parent(RasenExtEnv& parent) {
    LUX_RETHROW(ext_parents.len + 1 <= 0x1000,
        "exceeded maximum external parents number");
    ext_parents.push(&parent);
    U16 parent_id = ext_parents.len - 1;
    LUX_RETHROW(available_funcs.len + parent.funcs.len <= 0x1000 - MAX_SYS_FUNCS,
        "exceeded maximum functions number");
    LUX_ASSERT(local_funcs.len + parent.funcs.len <= 0x1000 - MAX_SYS_FUNCS);
    for(auto& id : parent.symbol_table) {
        available_funcs.push(
            {FuncId::EXTERNAL_PARENT, id.second, parent_id});
        symbol_table[id.first] = available_funcs.len - 1;
    }
    return LUX_OK;
}

LUX_MAY_FAIL RasenEnv::read_mem(U8* out, U8 addr) {
    LUX_RETHROW(addr < RN_RAM_LEN);
    *out = ram[addr];
    return LUX_OK;
}

LUX_MAY_FAIL RasenEnv::write_mem(U8 val, U8 addr) {
    LUX_RETHROW(addr < RN_RAM_LEN);
    ram[addr] = val;
    return LUX_OK;
}

struct OpcodeDef {
    Str str;
    enum class Arg1 { X, Y, V, T, } arg1;
    enum class Arg2 { NONE, Z, V, } arg2;
    U16 val;
};

struct MacroDef {
    Str str;
    enum class Args { ZERO, ONE, TWO } args;
    Str val;
};

static OpcodeDef constexpr opcodes[] = {
    {"copy"_l , OpcodeDef::Arg1::Y, OpcodeDef::Arg2::Z   , 0x0200},
    {"and"_l  , OpcodeDef::Arg1::Y, OpcodeDef::Arg2::Z   , 0x0300},
    {"not"_l  , OpcodeDef::Arg1::Y, OpcodeDef::Arg2::Z   , 0x0400},
    {"or"_l   , OpcodeDef::Arg1::Y, OpcodeDef::Arg2::Z   , 0x0500},
    {"xor"_l  , OpcodeDef::Arg1::Y, OpcodeDef::Arg2::Z   , 0x0600},
    {"rshf"_l , OpcodeDef::Arg1::Y, OpcodeDef::Arg2::Z   , 0x0700},
    {"lshf"_l , OpcodeDef::Arg1::Y, OpcodeDef::Arg2::Z   , 0x0800},
    {"add"_l  , OpcodeDef::Arg1::Y, OpcodeDef::Arg2::Z   , 0x0900},
    {"sub"_l  , OpcodeDef::Arg1::Y, OpcodeDef::Arg2::Z   , 0x0a00},
    {"mul"_l  , OpcodeDef::Arg1::Y, OpcodeDef::Arg2::Z   , 0x0b00},
    {"div"_l  , OpcodeDef::Arg1::Y, OpcodeDef::Arg2::Z   , 0x0c00},
    {"mod"_l  , OpcodeDef::Arg1::Y, OpcodeDef::Arg2::Z   , 0x0d00},
    {"load"_l , OpcodeDef::Arg1::Y, OpcodeDef::Arg2::Z   , 0x0e00},
    {"save"_l , OpcodeDef::Arg1::Y, OpcodeDef::Arg2::Z   , 0x0f00},
    {"push"_l , OpcodeDef::Arg1::Y, OpcodeDef::Arg2::NONE, 0x1000},
    {"pop"_l  , OpcodeDef::Arg1::Y, OpcodeDef::Arg2::NONE, 0x1100},
    {"pushv"_l, OpcodeDef::Arg1::V, OpcodeDef::Arg2::NONE, 0x1200},
    {"xcall"_l, OpcodeDef::Arg1::T, OpcodeDef::Arg2::NONE, 0x6000},
    {"loadv"_l, OpcodeDef::Arg1::X, OpcodeDef::Arg2::V   , 0x7000},
    {"savev"_l, OpcodeDef::Arg1::X, OpcodeDef::Arg2::V   , 0x8000},
    {"const"_l, OpcodeDef::Arg1::X, OpcodeDef::Arg2::V   , 0x9000},
    {"addv"_l , OpcodeDef::Arg1::X, OpcodeDef::Arg2::V   , 0xa000},
    {"jmp"_l  , OpcodeDef::Arg1::T, OpcodeDef::Arg2::NONE, 0xb000},
    {"jez"_l  , OpcodeDef::Arg1::T, OpcodeDef::Arg2::NONE, 0xc000},
    {"jnz"_l  , OpcodeDef::Arg1::T, OpcodeDef::Arg2::NONE, 0xd000},
    {"jgz"_l  , OpcodeDef::Arg1::T, OpcodeDef::Arg2::NONE, 0xe000},
    {"jlz"_l  , OpcodeDef::Arg1::T, OpcodeDef::Arg2::NONE, 0xf000},
};

static MacroDef constexpr macros[] = {
    //{"subv"_l, MacroDef::Args::TWO , "addv $0 (#ff $1 #1 - -)"_l},
    //{"incr"_l, MacroDef::Args::ONE , "addv $0 #1"_l},
    //{"decr"_l, MacroDef::Args::ONE , "subv $0 #1"_l},
    {"cmp"_l , MacroDef::Args::TWO , "copy $1 CR\n"_l
                                     "sub  $0 CR"_l},
    {"jeq"_l , MacroDef::Args::ONE , "jez  $0"_l},
    {"jnq"_l , MacroDef::Args::ONE , "jnz  $0"_l},
    {"jgq"_l , MacroDef::Args::ONE , "jlz  $0"_l},
    {"jlq"_l , MacroDef::Args::ONE , "jgz  $0"_l},
    {"nop"_l , MacroDef::Args::ZERO, "and  r0 r0"_l},
};

struct LabelRef {
    Str ident;
    U16 pos;
};

struct LabelDef {
    StrBuff ident;
    Uns pos;
};

static DynArr<U16>*     bytecode;
static DynArr<LabelRef> label_refs;
static DynArr<LabelDef> label_defs;

static void error_expected(Str const& x, Str const& got) {
    LUX_LOG_ERR("expected %.*s, got \"%.*s\"",
                (int)x.len, x.beg, (int)got.len, got.beg);
}

LUX_MAY_FAIL static try_parse_digit(U8* out, char ch) {
    if(ch >= '0' && ch <= '9') {
        *out = ch - '0';
    } else if(ch >= 'a' && ch <= 'f') {
        *out = (ch - 'a') + 10;
    } else {
        return LUX_FAIL;
    }
    return LUX_OK;
}

LUX_MAY_FAIL static parse_digit(U8* out, char ch) {
    LUX_RETHROW(try_parse_digit(out, ch),
        "expected hexadecimal digit, got \"%c\"", ch);
    return LUX_OK;
}

LUX_MAY_FAIL static parse_number(U8* out, Str str) {
    U8  digit = 0;
    Uns i     = 0;
    *out      = 0;
    while(i < str.len && try_parse_digit(&digit, str[i]) == LUX_OK) {
        *out *= 0x10;
        *out += digit;
        i++;
    }
    return LUX_OK;
}

void static skip_spaces(Str* out, Str in) {
    Uns i = 0;
    while(i < in.len && (in[i] == ' ' || in[i] == '\t')) i++;
    *out += i;
}

LUX_MAY_FAIL static _parse_expr(Deque<U8>* stack, Str str) {
    if(str.len == 0) return LUX_OK;
    skip_spaces(&str, str);
    if(str.len >= 2 && str[0] == '#') {
        stack->emplace_back();
        return parse_number(&stack->back(), str + 1);
    }
    LUX_UNIMPLEMENTED();
    /*if(str.len >= 2 && str[0] == '(' && str[str.len - 1] == ')') {
        str += 1;
        str.len -= 1;
        return _parse_expr(&stack, str);
    }
    if(str.len >= 1)
        if(str[0] == '-') {
            if(stack->len < 2) {
                LUX_LOG_ERR("expected 2 stack elements, got %zu", stack->len);
                return LUX_FAIL;
            }
            U8 v1 = stack->back();
            stack->pop_back();
            U8 v2 = stack->back();
            stack->pop_back();
            stack->emplace(v2 - v1);
            return LUX_OK;
        }
        if(str[0] == '+') {
            if(stack->len < 2) {
                LUX_LOG_ERR("expected 2 stack elements, got %zu", stack->len);
                return LUX_FAIL;
            }
            U8 v1 = stack->back();
            stack->pop_back();
            U8 v2 = stack->back();
            stack->pop_back();
            stack->emplace(v2 + v1);
            return LUX_OK;
        }
    }*/
    LUX_LOG_ERR("unrecognized expression");
    return LUX_FAIL;
}

LUX_MAY_FAIL static parse_expr(U8* out, Str str) {
    static Deque<U8> stack;
    stack.clear();
    LUX_RETHROW(_parse_expr(&stack, str));
    if(stack.size() == 0) {
        LUX_LOG_ERR("expression stack empty, could not evaluate expression");
        return LUX_FAIL;
    }
    if(stack.size() > 1) {
        LUX_LOG_ERR("excess value(s) in expression stack");
        return LUX_FAIL;
    }
    *out = stack.back();
    return LUX_OK;
}

LUX_MAY_FAIL static assemble_arg(U8* out, Str str, U16 mask) {
    if(str.len == 0) {
        error_expected("argument"_l, str);
        return LUX_FAIL;
    }
    if(str[0] == 'r') {
        if(str.len != 2) {
            error_expected("register argument"_l, str);
            return LUX_FAIL;
        }
        LUX_RETHROW(parse_digit(out, str[1]));
    } else if(str[0] == '@') {
        if(mask != 0xfff) {
            LUX_LOG_ERR("invalid argument mask for label");
            return LUX_FAIL;
        }
        str.beg += 1;
        str.len -= 1;
        label_refs.push({str, (U16)(bytecode->len - 1)});
        *out = 0;
    } else if(str[0] == '#') {
        U8 expr;
        LUX_RETHROW(parse_expr(&expr, str),
                    "failed to parse expression");
        *out = expr;
    } else if(str[0] == '(' && str.len >= 3 && str[str.len - 1] == ')') {
        U8 expr;
        LUX_RETHROW(parse_expr(&expr, str),
                    "failed to parse expression");
        *out = expr;
    } else if(str[0] == 'C') {
        if(str.len != 2 || str[1] != 'R') {
            error_expected("comparison register"_l, str);
            return LUX_FAIL;
        }
        *out = 0xf;
    } else {
        error_expected("argument"_l, str);
        return LUX_FAIL;
    }
    return LUX_OK;
}

LUX_MAY_FAIL static parse_opcode(OpcodeDef const& def, Arr<Str, 2> const& args) {
    bytecode->emplace(def.val);
    auto& out = bytecode->last();
    out = def.val;
    U16 arg1 = 0;
    U16 arg1_mask;
    U16 arg1_shift;
    switch(def.arg1) {
        case OpcodeDef::Arg1::X: arg1_shift = 8; arg1_mask = 0xf;   break;
        case OpcodeDef::Arg1::Y: arg1_shift = 4; arg1_mask = 0xf;   break;
        case OpcodeDef::Arg1::V: arg1_shift = 0; arg1_mask = 0xff;  break;
        case OpcodeDef::Arg1::T: arg1_shift = 0; arg1_mask = 0xfff; break;
        default: LUX_UNREACHABLE();
    }
    LUX_RETHROW(assemble_arg((U8*)&arg1, args[0], arg1_mask),
                "failed to parse first argument");
    out |= arg1 << arg1_shift;
    if(def.arg2 != OpcodeDef::Arg2::NONE) {
        U16 arg2 = 0;
        U16 arg2_mask;
        switch(def.arg2) {
            case OpcodeDef::Arg2::Z: arg2_mask = 0xf;  break;
            case OpcodeDef::Arg2::V: arg2_mask = 0xff; break;
            default: LUX_UNREACHABLE();
        }
        LUX_RETHROW(assemble_arg((U8*)&arg2, args[1], arg2_mask),
                    "failed to parse second argument");
        out |= arg2;
    }
    return LUX_OK;
}

LUX_MAY_FAIL static parse_opcodes(Str const& in, Arr<Str, 2> const& args) {
    for(Uns j = 0; j < arr_len(opcodes); j++) {
        auto const& opcode = opcodes[j];
        if(in == opcode.str) {
            LUX_RETHROW(parse_opcode(opcode, args),
                        "failed to parse opcode \"%.*s %.*s %.*s\"",
                        (int)in.len, in.beg,
                        (int)args[0].len, args[0].beg,
                        (int)args[1].len, args[1].beg);
            return LUX_OK;
        }
    }
    return LUX_FAIL;
}

LUX_MAY_FAIL static rasm_str(Str const& in);

LUX_MAY_FAIL static parse_macro(MacroDef const& def, Arr<Str, 2> const& args) {
    MacroDef::Args args_num =
        args[0].len == 0 ? MacroDef::Args::ZERO :
        args[1].len == 0 ? MacroDef::Args::ONE  : MacroDef::Args::TWO;
    if(def.args != args_num) {
        //@TODO more info?
        LUX_LOG_ERR("unexpected number of arguments");
        return LUX_FAIL;
    }
    struct Subst {
        U16 pos;
        Uns arg;
    };
    DynArr<Subst> substs;
    Uns buff_len = def.val.len;
    Arr<Arr<char, 2>, 2> s_args;
    for(Uns j = 0; j < 2; j++) {
        s_args[j][0] = '$';
        s_args[j][1] = (char)(j + '0');
    }
    for(Uns i = 0; i < def.val.len; i++) {
        for(Uns j = 0; j < 2; j++) {
            ///not possible for a s_arg to exist
            SizeT s_len = arr_len(s_args[j]);
            if(def.val.len - i < s_len) break;
            if(Str(s_args[j], s_len) == Str(def.val.beg + i, s_len)) {
                buff_len -= s_len;
                buff_len += args[j].len;
                substs.push({(U16)(substs.len == 0 ? i :
                                   i - substs.last().pos - s_len), j});
            }
        }
    }
    DynArr<char> buff(buff_len);
    Uns b = 0;
    Uns s = 0;
    for(Uns i = 0; i < substs.len; i++) {
        //@TODO rewrite using slices
        std::memcpy(buff.beg + b, def.val.beg + s, substs[i].pos);
        b += substs[i].pos;
        s += substs[i].pos;
        LUX_ASSERT(substs[i].arg < arr_len(args));
        auto const& arg = args[substs[i].arg];
        std::memcpy(buff.beg + b, arg.beg, arg.len);
        b += arg.len;
        s += arr_len(s_args[substs[i].arg]);
    }
    std::memcpy(buff.beg + b, def.val.beg + s, def.val.len - s);
    return rasm_str((Str)buff);
}

LUX_MAY_FAIL static parse_macros(Str const& in, Arr<Str, 2> const& args) {
    for(Uns j = 0; j < arr_len(macros); j++) {
        auto const& macro = macros[j];
        if(in == macro.str) {
            LUX_RETHROW(parse_macro(macro, args));
            return LUX_OK;
        }
    }
    return LUX_FAIL;
}

LUX_MAY_FAIL static parse_arg(Str* remaining, Str* arg, Str in) {
    *remaining = in;
    *arg = in;
    Uns i = 0;
    bool parens = false;
    while(i < in.len && (in[i] != ' ' || parens)) {
             if(in[i] == '(') parens = true;
        else if(in[i] == ')') parens = false;
        i++;
    }
    if(parens) {
        LUX_LOG_ERR("unterminated parens");
        return LUX_FAIL;
    }
    arg->len = i;
    *remaining += arg->len;
    return LUX_OK;
}

LUX_MAY_FAIL static parse_line(Str* remaining, Str str) {
    //@CONSIDER doing it in one run, instead of finding newline first etc.
    *remaining = str;
    ///find line length
    Uns line_end = 0;
    {   Uns i = 0;
        while(i < str.len && str[i] != '\n') i++;
        line_end = i;
        str.len = line_end;
        *remaining += str.len;
    }
    /// skip comments
    {   Uns i = 0;
        while(i < str.len && str[i] != ';') i++;
        str.len = i;
    }
    if(str.len == 0) return LUX_OK;
    ///skip prefix whitespaces
    {   Uns i = 0;
        while(i < str.len && (str[i] == ' ' || str[i] == '\t')) i++;
        str += i;
    }
    Str op = str;
    ///find last non-space char, now we have the opcode
    {   Uns i = 0;
        while(i < str.len && str[i] != ' ') i++;
        op.len = i;
    }
    if(op.len == 0) return LUX_OK;
    if(op[0] == '@') {
        if(op.len <= 1) {
            error_expected("label definition"_l, op);
            return LUX_FAIL;
        }
        op += 1;
        label_defs.push({op, bytecode->len});
        return LUX_OK;
    } else {
        str += op.len;
        skip_spaces(&str, str);
        Str arg1;
        LUX_RETHROW(parse_arg(&str, &arg1, str),
                    "failed to parse first argument");
        skip_spaces(&str, str);
        Str arg2;
        LUX_RETHROW(parse_arg(&str, &arg2, str),
                    "failed to parse second argument");
        return (LuxRval)(parse_opcodes(op, {arg1, arg2}) ||
                         parse_macros( op, {arg1, arg2}));
    }
}

LUX_MAY_FAIL static rasm_str(Str const& in) {
    Str remaining = in;
    Uns line_num  = 1;
    while(remaining.len > 0) {
        ///we skip the newline from previous iteration
        if(remaining.beg[0] == '\n') {
            if(remaining.len > 1) {
                remaining.len -= 1;
                remaining.beg += 1;
            } else break;
        }
        LUX_RETHROW(parse_line(&remaining, remaining),
                    "assembly failed at line %zu", line_num);
        line_num++;
    }
    return LUX_OK;
}

LUX_MAY_FAIL RasenEnv::rasm(DynArr<U16>* out, Str const& in) {
    bytecode = out;
    //@TODO fix the globals
    bytecode->clear();
    label_defs.clear();
    label_refs.clear();
    for(auto const& pair : symbol_table) {
        LUX_LOG("loaded label \"%.*s\":%04x",
                (int)pair.first.len, pair.first.beg, pair.second);
        label_defs.push({pair.first, pair.second});
    }
    for(auto pair : sys_env.funcs_lookup) {
        LUX_LOG("loaded label \"%.*s\":%04x",
                (int)pair.first.len, pair.first.beg, 0xF00 | pair.second);
        label_defs.push({pair.first, (Uns)0xF00 | pair.second});
    }
    LUX_RETHROW(rasm_str(in));
    for(Uns i = 0; i < label_refs.len; i++) {
        bool found = false;
        //@TODO make label_defs a map
        for(Uns j = 0; j < label_defs.len; j++) {
            LUX_LOG("\"%.*s\"", (int)label_defs[i].ident.len, label_defs[i].ident.beg);
            LUX_LOG("\"%.*s\"", (int)label_refs[i].ident.len, label_refs[i].ident.beg);
            if(label_defs[j].ident == label_refs[i].ident) {
                LUX_ASSERT(label_refs[i].pos < bytecode->len);
                (*bytecode)[label_refs[i].pos] |= label_defs[j].pos;
                found = true;
                break;
            }
        }
        if(!found) {
            auto const& ident = label_refs[i].ident;
            LUX_LOG_ERR("undefined label \"%.*s\"", (int)ident.len, ident.beg);
            return LUX_FAIL;
        }
    }
    LUX_LOG("compiled rasen bytecode of length %zu words", bytecode->len);
    LUX_LOG("in:\n%.*s", (int)in.len, in.beg);
    LUX_LOG("out:");
    for(Uns i = 0; i < bytecode->len; ++i) {
        LUX_LOG("%03x: %04x", i, (*bytecode)[i]);
    }
    return LUX_OK;
}

