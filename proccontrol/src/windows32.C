#include <map>
#include "dynutil/h/dyn_regs.h"

#if defined(_AMD86_)
/**
 * Can't seem to get WinNT.h to give me a context for another architecture--this is copied out of WinNT.h and renamed
 **/
#define SIZE_OF_80387_REGISTERS32      80
#define MAXIMUM_SUPPORTED_EXTENSION32  512
typedef struct _FLOATING_SAVE_AREA32 {
    DWORD   ControlWord;
    DWORD   StatusWord;
    DWORD   TagWord;
    DWORD   ErrorOffset;
    DWORD   ErrorSelector;
    DWORD   DataOffset;
    DWORD   DataSelector;
    BYTE    RegisterArea[SIZE_OF_80387_REGISTERS32];
    DWORD   Cr0NpxState;
} FLOATING_SAVE_AREA32
typedef struct _CONTEXT32 {
    DWORD ContextFlags;
    DWORD   Dr0;
    DWORD   Dr1;
    DWORD   Dr2;
    DWORD   Dr3;
    DWORD   Dr6;
    DWORD   Dr7;
    FLOATING_SAVE_AREA32 FloatSave;
    DWORD   SegGs;
    DWORD   SegFs;
    DWORD   SegEs;
    DWORD   SegDs;
    DWORD   Edi;
    DWORD   Esi;
    DWORD   Ebx;
    DWORD   Edx;
    DWORD   Ecx;
    DWORD   Eax;
    DWORD   Ebp;
    DWORD   Eip;
    DWORD   SegCs;
    DWORD   EFlags;
    DWORD   Esp;
    DWORD   SegSs;
    BYTE    ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION32];
} CONTEXT32;

#define CONTEXT_TYPE CONTEXT32

#else
#include <WinNT.h>
#define CONTEXT_TYPE CONTEXT
#endif

#define SET_CONTEXT_OFFSET(DynReg, Field) m[DynReg] = std::pair<int, int>((int) ((unsigned long) &(context_ptr->Field)), (int) sizeof(context_ptr->Field))

unsigned int getContextSize32() {
	return (unsigned int) sizeof(CONTEXT_TYPE);
}

void getContextOffsets32(std::map<Dyninst::MachRegister, std::pair<int, int> > &m)
{
	CONTEXT_TYPE *context_ptr = (CONTEXT_TYPE *) 0x0;
	SET_CONTEXT_OFFSET(Dyninst::x86::eax, Eax);
	SET_CONTEXT_OFFSET(Dyninst::x86::ecx, Ecx);
	SET_CONTEXT_OFFSET(Dyninst::x86::edx, Edx);
	SET_CONTEXT_OFFSET(Dyninst::x86::edx, Ebx);
	SET_CONTEXT_OFFSET(Dyninst::x86::ebx, Esp);
	SET_CONTEXT_OFFSET(Dyninst::x86::ebp, Ebp);
	SET_CONTEXT_OFFSET(Dyninst::x86::esi, Esi);
	SET_CONTEXT_OFFSET(Dyninst::x86::edi, Edi);
	SET_CONTEXT_OFFSET(Dyninst::x86::eip, Eip);
	SET_CONTEXT_OFFSET(Dyninst::x86::dr0, Dr0);
	SET_CONTEXT_OFFSET(Dyninst::x86::dr1, Dr1);
	SET_CONTEXT_OFFSET(Dyninst::x86::dr2, Dr2);
	SET_CONTEXT_OFFSET(Dyninst::x86::dr3, Dr3);
	SET_CONTEXT_OFFSET(Dyninst::x86::dr6, Dr6);
	SET_CONTEXT_OFFSET(Dyninst::x86::dr7, Dr7);
	SET_CONTEXT_OFFSET(Dyninst::x86::ds, SegDs);
	SET_CONTEXT_OFFSET(Dyninst::x86::es, SegEs);
	SET_CONTEXT_OFFSET(Dyninst::x86::fs, SegFs);
	SET_CONTEXT_OFFSET(Dyninst::x86::gs, SegGs);
	SET_CONTEXT_OFFSET(Dyninst::x86::cs, SegCs);
	SET_CONTEXT_OFFSET(Dyninst::x86::ss, SegSs);
	SET_CONTEXT_OFFSET(Dyninst::x86::flags, EFlags);
}
