#include <config.hpp>
#include <glm/gtx/rotate_vector.hpp>
//
#include <lux_shared/common.hpp>
//
#include <entity.hpp>
#include "rasen.hpp"

void RasenEnv::err(U8 code, RasenStack const& s) {
    LUX_UNIMPLEMENTED();
}

void RasenEnv::exec(U16 word, RasenStack const& stack) {
    U8  c = (word & 0xF000) >> 12;
    U8  x = (word & 0x0F00) >> 8;
    U8  y = (word & 0x00F0) >> 4;
    U8  z =  word & 0x000F;
    U8  v =  word & 0x00FF;
    U16 t =  word & 0x0FFF;
    auto& s  = *stack.val;
    auto& sp = *stack.sp;
    auto& m  = ram;
    auto& cr = r[0xf];
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
            case 0xe: r[z]    =  m[r[y]]; break;
            case 0xf: m[r[z]] =  r[y];    break;
            default: err(RN_SC_ERR_ILLEGAL_OP, stack);
        } break;
        case 0x1: switch(x) {
            case 0x0: s[sp++] =  r[y];    break;
            case 0x1: r[y]    =  s[--sp]; break;
            case 0x2: s[sp++] =  v;       break;
            default: err(RN_SC_ERR_ILLEGAL_OP, stack);
        } break;
        case 0x6: {
            if(parent_tag == INTERNAL) {
                int_parent->call(t, stack);
            } else if(parent_tag == EXTERNAL) {
                //ext_parent.call(t, stack);
            }
            break;
        }
        case 0x7: r[x]  = m[v]; break;
        case 0x8: m[v]  = r[x]; break;
        case 0x9: r[x]  = v;    break;
        case 0xa: r[x] += v;    break;
        case 0xb: pc    = t;    break;
        case 0xc: if(    cr == 0) pc = t; break;
        case 0xd: if(    cr != 0) pc = t; break;
        case 0xe: if((I8)cr >  0) pc = t; break;
        case 0xf: if((I8)cr <  0) pc = t; break;
        default: err(RN_SC_ERR_ILLEGAL_OP, stack);
    }
}

void RasenEnv::exec_rom(Slice<U16> const& rom, RasenStack const& s) {
    pc = 0;
    while(pc < rom.len) {
        exec(rom.beg[pc], s);
    }
}

void RasenEnv::call(U16 id, RasenStack const& s) {
    if(id < funcs.len) {
        exec_rom(funcs[id], s);
    } else {
        err(RN_SC_ERR_ILLEGAL_CALL, s);
    }
}

void RasenEnv::call(Str const& str_id, RasenStack const& s) {
    if(funcs_lookup.count(str_id) > 0) {
        auto id = funcs_lookup.at(str_id);
        LUX_ASSERT(id < funcs.len);
        call(id, s);
    } else {
        err(RN_SC_ERR_ILLEGAL_CALL, s);
    }
}
/*
void extcall_err(Rasen* cpu) {
    LUX_LOG("RaseN error: %02x", cpu->pop());
    LUX_LOG("\tPC:    %02x", cpu->pc);
    LUX_LOG("\tSP:    %02x", cpu->sp);
    LUX_LOG("\tM[PC]: %02x", cpu->m[cpu->pc]);
    while(cpu->sp != 0) {
        LUX_LOG("\ts[%02x]: %02x", cpu->sp - 1, cpu->pop());
    }
    for(U8 rp = 0; rp < RN_REG_LEN; ++rp) {
        LUX_LOG("\tr[%02x]: %02x", rp, cpu->r[rp]);
    }

    extcall_halt(cpu);
}

void extcall_halt(Rasen* cpu) {
    cpu->running = false;
    cpu->pc = 0;
    cpu->sp = 0;
}

void extcall_print(Rasen* cpu) {
    U8 val = cpu->pop();
    LUX_LOG("RaseN prints: 0x%x %u", val, val);
}

void extcall_entity_rotate(Rasen* cpu) {
    U16 id = cpu->pop();
    bool right = cpu->pop() != 0;
    if(entity_comps.a_vel.count(id) == 0) {
        cpu->push(RN_ERR_NO_ENTITY);
        extcall_err(cpu);
        return;
    }
    entity_comps.a_vel.at(id).vel += (right ? 1.f : -1.f) * ENTITY_A_VEL;
}

void extcall_entity_move(Rasen* cpu) {
    U16 id = cpu->pop();
    bool forward = cpu->pop() != 0;
    if(entity_comps.vel.count(id) == 0) {
        cpu->push(RN_ERR_NO_ENTITY);
        extcall_err(cpu);
        return;
    }
    Vec2F vec = Vec2F(0.f, (forward ? 1.f : -1.f) * ENTITY_L_VEL);
    if(entity_comps.orientation.count(id) > 0) {
        vec = glm::rotate(vec, entity_comps.orientation.at(id).angle);
    }
    entity_comps.vel.at(id) = vec;
}

void extcall_entity_get_pos(Rasen* cpu) {
    U16 id = cpu->pop();
    if(entity_comps.pos.count(id) == 0) {
        cpu->push(RN_ERR_NO_ENTITY);
        extcall_err(cpu);
        return;
    }
    Vec2<I8> pos =
        glm::floor(glm::clamp(entity_comps.pos.at(id), -128.f, 127.f));
    cpu->push(pos.y);
    cpu->push(pos.x);
}

void extcall_entity_get_rotation(Rasen* cpu) {
    U16 id = cpu->pop();
    if(entity_comps.orientation.count(id) == 0) {
        cpu->push(RN_ERR_NO_ENTITY);
        extcall_err(cpu);
        return;
    }
    F32 angle = entity_comps.orientation.at(id).angle;
    U16 rval = (angle / tau) * (F32)((1 << 15) - 1);
    cpu->push((rval & 0xFF00) >> 8);
    cpu->push( rval & 0x00FF);
}
*/
