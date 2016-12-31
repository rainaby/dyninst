/*
 * See the dyninst/COPYRIGHT file for copyright information.
 *
 * We provide the Paradyn Tools (below described as "Paradyn")
 * on an AS IS basis, and do not warrant its validity or performance.
 * We reserve the right to update, modify, or discontinue this
 * software at any time.  We shall have no obligation to supply such
 * updates or modifications or any other form of support to you.
 *
 * By your use of Paradyn, you understand and agree that we (or any
 * other person or entity with proprietary rights in Paradyn) are
 * under no obligation to provide either maintenance services,
 * update services, notices of latent defects, or correction of
 * defects for Paradyn.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "RegisterConversion.h"
#include "registerSpace.h"

#include <map>
#include <boost/assign/list_of.hpp>
#include "Register.h"
#include "dyn_regs.h"
#include "registerSpace.h"

using namespace Dyninst;
using namespace Dyninst::InstructionAPI;
using namespace std;
using namespace boost::assign;

//#warning "This file is not verified yet!"
multimap<Register, MachRegister> regToMachReg64 = map_list_of
  (registerSpace::r0,  		archPTX::x0)
  (registerSpace::r1,  		archPTX::x1)
  (registerSpace::r2,  		archPTX::x2)
  (registerSpace::r3,  		archPTX::x3)
  (registerSpace::r4,  		archPTX::x4)
  (registerSpace::r5,  		archPTX::x5)
  (registerSpace::r6,  		archPTX::x6)
  (registerSpace::r7,  		archPTX::x7)
  (registerSpace::r8,  		archPTX::x8)
  (registerSpace::r9,  		archPTX::x9)
  (registerSpace::r10, 		archPTX::x10)
  (registerSpace::r11, 		archPTX::x11)
  (registerSpace::r12, 		archPTX::x12)
  (registerSpace::r13, 		archPTX::x13)
  (registerSpace::r14, 		archPTX::x14)
  (registerSpace::r15, 		archPTX::x15)
  (registerSpace::r16, 		archPTX::x16)
  (registerSpace::r17, 		archPTX::x17)
  (registerSpace::r18, 		archPTX::x18)
  (registerSpace::r19, 		archPTX::x19)
  (registerSpace::r20, 		archPTX::x20)
  (registerSpace::r21, 		archPTX::x21)
  (registerSpace::r22, 		archPTX::x22)
  (registerSpace::r23, 		archPTX::x23)
  (registerSpace::r24, 		archPTX::x24)
  (registerSpace::r25, 		archPTX::x25)
  (registerSpace::r26, 		archPTX::x26)
  (registerSpace::r27, 		archPTX::x27)
  (registerSpace::r28, 		archPTX::x28)
  (registerSpace::r29, 		archPTX::x29)
  (registerSpace::r30, 		archPTX::x30)
  (registerSpace::fpr0,  	archPTX::q0)
  (registerSpace::fpr1,  	archPTX::q1)
  (registerSpace::fpr2,  	archPTX::q2)
  (registerSpace::fpr3,  	archPTX::q3)
  (registerSpace::fpr4,  	archPTX::q4)
  (registerSpace::fpr5,  	archPTX::q5)
  (registerSpace::fpr6,  	archPTX::q6)
  (registerSpace::fpr7,  	archPTX::q7)
  (registerSpace::fpr8,  	archPTX::q8)
  (registerSpace::fpr9,  	archPTX::q9)
  (registerSpace::fpr10, 	archPTX::q10)
  (registerSpace::fpr11, 	archPTX::q11)
  (registerSpace::fpr12, 	archPTX::q12)
  (registerSpace::fpr13, 	archPTX::q13)
  (registerSpace::fpr14, 	archPTX::q14)
  (registerSpace::fpr15, 	archPTX::q15)
  (registerSpace::fpr16, 	archPTX::q16)
  (registerSpace::fpr17, 	archPTX::q17)
  (registerSpace::fpr18, 	archPTX::q18)
  (registerSpace::fpr19, 	archPTX::q19)
  (registerSpace::fpr20, 	archPTX::q20)
  (registerSpace::fpr21, 	archPTX::q21)
  (registerSpace::fpr22, 	archPTX::q22)
  (registerSpace::fpr23, 	archPTX::q23)
  (registerSpace::fpr24, 	archPTX::q24)
  (registerSpace::fpr25, 	archPTX::q25)
  (registerSpace::fpr26, 	archPTX::q26)
  (registerSpace::fpr27, 	archPTX::q27)
  (registerSpace::fpr28, 	archPTX::q28)
  (registerSpace::fpr29, 	archPTX::q29)
  (registerSpace::fpr30, 	archPTX::q30)
  (registerSpace::fpr31, 	archPTX::q31)
  (registerSpace::lr, 		archPTX::x30)
  (registerSpace::sp, 		archPTX::sp)
  (registerSpace::pc, 		archPTX::pc)
  (registerSpace::pstate, 	archPTX::pstate)
  (registerSpace::fpcr, 	archPTX::fpcr)
  (registerSpace::fpsr, 	archPTX::fpsr)
  ;

map<MachRegister, Register> reverseRegisterMap = map_list_of
  (archPTX::x0,  registerSpace::r0)
  (archPTX::x1,  registerSpace::r1)
  (archPTX::x2,  registerSpace::r2)
  (archPTX::x3,  registerSpace::r3)
  (archPTX::x4,  registerSpace::r4)
  (archPTX::x5,  registerSpace::r5)
  (archPTX::x6,  registerSpace::r6)
  (archPTX::x7,  registerSpace::r7)
  (archPTX::x8,  registerSpace::r8)
  (archPTX::x9,  registerSpace::r9)
  (archPTX::x10, registerSpace::r10)
  (archPTX::x11, registerSpace::r11)
  (archPTX::x12, registerSpace::r12)
  (archPTX::x13, registerSpace::r13)
  (archPTX::x14, registerSpace::r14)
  (archPTX::x15, registerSpace::r15)
  (archPTX::x16, registerSpace::r16)
  (archPTX::x17, registerSpace::r17)
  (archPTX::x18, registerSpace::r18)
  (archPTX::x19, registerSpace::r19)
  (archPTX::x20, registerSpace::r20)
  (archPTX::x21, registerSpace::r21)
  (archPTX::x22, registerSpace::r22)
  (archPTX::x23, registerSpace::r23)
  (archPTX::x24, registerSpace::r24)
  (archPTX::x25, registerSpace::r25)
  (archPTX::x26, registerSpace::r26)
  (archPTX::x27, registerSpace::r27)
  (archPTX::x28, registerSpace::r28)
  (archPTX::x29, registerSpace::r29)
  (archPTX::x30, registerSpace::r30)
  (archPTX::q0,   registerSpace::fpr0)
  (archPTX::q1,   registerSpace::fpr1)
  (archPTX::q2,   registerSpace::fpr2)
  (archPTX::q3,   registerSpace::fpr3)
  (archPTX::q4,   registerSpace::fpr4)
  (archPTX::q5,   registerSpace::fpr5)
  (archPTX::q6,   registerSpace::fpr6)
  (archPTX::q7,   registerSpace::fpr7)
  (archPTX::q8,   registerSpace::fpr8)
  (archPTX::q9,   registerSpace::fpr9)
  (archPTX::q10,  registerSpace::fpr10)
  (archPTX::q11,  registerSpace::fpr11)
  (archPTX::q12,  registerSpace::fpr12)
  (archPTX::q13,  registerSpace::fpr13)
  (archPTX::q14,  registerSpace::fpr14)
  (archPTX::q15,  registerSpace::fpr15)
  (archPTX::q16,  registerSpace::fpr16)
  (archPTX::q17,  registerSpace::fpr17)
  (archPTX::q18,  registerSpace::fpr18)
  (archPTX::q19,  registerSpace::fpr19)
  (archPTX::q20,  registerSpace::fpr20)
  (archPTX::q21,  registerSpace::fpr21)
  (archPTX::q22,  registerSpace::fpr22)
  (archPTX::q23,  registerSpace::fpr23)
  (archPTX::q24,  registerSpace::fpr24)
  (archPTX::q25,  registerSpace::fpr25)
  (archPTX::q26,  registerSpace::fpr26)
  (archPTX::q27,  registerSpace::fpr27)
  (archPTX::q28,  registerSpace::fpr28)
  (archPTX::q29,  registerSpace::fpr29)
  (archPTX::q30,  registerSpace::fpr30)
  (archPTX::q31,  registerSpace::fpr31)
  //(archPTX::x30,   registerSpace::lr)
  (archPTX::sp,   registerSpace::sp)
  (archPTX::pc,   registerSpace::pc)
  (archPTX::pstate,   registerSpace::pstate)
  (archPTX::fpcr,   registerSpace::fpcr)
  (archPTX::fpsr,   registerSpace::fpsr)
  ;

Register convertRegID(MachRegister reg, bool &wasUpcast) {
    wasUpcast = false;

    MachRegister baseReg = MachRegister((reg.getBaseRegister().val() & ~reg.getArchitecture()) | Arch_archPTX);
//    RegisterAST::Ptr debug(new RegisterAST(baseReg));
//    fprintf(stderr, "DEBUG: converting %s", toBeConverted->format().c_str());
//    fprintf(stderr, " to %s\n", debug->format().c_str());
    map<MachRegister, Register>::const_iterator found =
      reverseRegisterMap.find(baseReg);
    if(found == reverseRegisterMap.end()) {
      // Yeah, this happens when we analyze trash code. Oops...
      return registerSpace::ignored;
    }

    return found->second;
}


Register convertRegID(RegisterAST::Ptr toBeConverted, bool& wasUpcast)
{
    return convertRegID(toBeConverted.get(), wasUpcast);
}

Register convertRegID(RegisterAST* toBeConverted, bool& wasUpcast)
{
    if(!toBeConverted) {
        //assert(0);
      return registerSpace::ignored;
    }
    return convertRegID(toBeConverted->getID(), wasUpcast);
}

MachRegister convertRegID(Register r, Dyninst::Architecture arch) {
    if( arch == Arch_archPTX ) {
        switch(r) {
            case registerSpace::r0: 	return archPTX::x0;
            case registerSpace::r1: 	return archPTX::x1;
            case registerSpace::r2: 	return archPTX::x2;
            case registerSpace::r3: 	return archPTX::x3;
            case registerSpace::r4: 	return archPTX::x4;
            case registerSpace::r5: 	return archPTX::x5;
            case registerSpace::r6: 	return archPTX::x6;
            case registerSpace::r7: 	return archPTX::x7;
            case registerSpace::r8: 	return archPTX::x8;
            case registerSpace::r9: 	return archPTX::x9;
            case registerSpace::r10: 	return archPTX::x10;
            case registerSpace::r11: 	return archPTX::x11;
            case registerSpace::r12: 	return archPTX::x12;
            case registerSpace::r13: 	return archPTX::x13;
            case registerSpace::r14: 	return archPTX::x14;
            case registerSpace::r15: 	return archPTX::x15;
            case registerSpace::r16: 	return archPTX::x16;
            case registerSpace::r17: 	return archPTX::x17;
            case registerSpace::r18: 	return archPTX::x18;
            case registerSpace::r19: 	return archPTX::x19;
            case registerSpace::r20: 	return archPTX::x20;
            case registerSpace::r21: 	return archPTX::x21;
            case registerSpace::r22: 	return archPTX::x22;
            case registerSpace::r23: 	return archPTX::x23;
            case registerSpace::r24: 	return archPTX::x24;
            case registerSpace::r25: 	return archPTX::x25;
            case registerSpace::r26: 	return archPTX::x26;
            case registerSpace::r27: 	return archPTX::x27;
            case registerSpace::r28: 	return archPTX::x28;
            case registerSpace::r29: 	return archPTX::x29;
            case registerSpace::r30: 	return archPTX::x30;
            case registerSpace::fpr0:  return archPTX::q0;
            case registerSpace::fpr1:  return archPTX::q1;
            case registerSpace::fpr2:  return archPTX::q2;
            case registerSpace::fpr3:  return archPTX::q3;
            case registerSpace::fpr4:  return archPTX::q4;
            case registerSpace::fpr5:  return archPTX::q5;
            case registerSpace::fpr6:  return archPTX::q6;
            case registerSpace::fpr7:  return archPTX::q7;
            case registerSpace::fpr8:  return archPTX::q8;
            case registerSpace::fpr9:  return archPTX::q9;
            case registerSpace::fpr10: return archPTX::q10;
            case registerSpace::fpr11: return archPTX::q11;
            case registerSpace::fpr12: return archPTX::q12;
            case registerSpace::fpr13: return archPTX::q13;
            case registerSpace::fpr14: return archPTX::q14;
            case registerSpace::fpr15: return archPTX::q15;
            case registerSpace::fpr16: return archPTX::q16;
            case registerSpace::fpr17: return archPTX::q17;
            case registerSpace::fpr18: return archPTX::q18;
            case registerSpace::fpr19: return archPTX::q19;
            case registerSpace::fpr20: return archPTX::q20;
            case registerSpace::fpr21: return archPTX::q21;
            case registerSpace::fpr22: return archPTX::q22;
            case registerSpace::fpr23: return archPTX::q23;
            case registerSpace::fpr24: return archPTX::q24;
            case registerSpace::fpr25: return archPTX::q25;
            case registerSpace::fpr26: return archPTX::q26;
            case registerSpace::fpr27: return archPTX::q27;
            case registerSpace::fpr28: return archPTX::q28;
            case registerSpace::fpr29: return archPTX::q29;
            case registerSpace::fpr30: return archPTX::q30;
            case registerSpace::fpr31: return archPTX::q31;
            case registerSpace::lr: 	return archPTX::x30;
            case registerSpace::sp: 	return archPTX::sp;
            case registerSpace::pc: 	return archPTX::pc;
            case registerSpace::pstate: 	return archPTX::pstate;
            default:
                break;
        }
    }else{
        assert(!"Invalid architecture");
    }

    assert(!"Register not handled");
    return InvalidReg;
}
