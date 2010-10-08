#include <map>
#include "dynutil/h/dyn_regs.h"

#include <WinNT.h>

#if defined(_X86_)
/**
 * Can't seem to get WinNT.h to give me a context for another architecture--this is copied out of WinNT.h and renamed
 **/
typedef struct DECLSPEC_ALIGN(16) _XMM_SAVE_AREA_64 {
    WORD   ControlWord;
    WORD   StatusWord;
    BYTE  TagWord;
    BYTE  Reserved1;
    WORD   ErrorOpcode;
    DWORD ErrorOffset;
    WORD   ErrorSelector;
    WORD   Reserved2;
    DWORD DataOffset;
    WORD   DataSelector;
    WORD   Reserved3;
    DWORD MxCsr;
    DWORD MxCsr_Mask;
    M128A FloatRegisters[8];
    M128A XmmRegisters[16];
    BYTE  Reserved4[96];
} XMM_SAVE_AREA_64;
typedef struct DECLSPEC_ALIGN(16) _CONTEXT64 {
    DWORD64 P1Home;
    DWORD64 P2Home;
    DWORD64 P3Home;
    DWORD64 P4Home;
    DWORD64 P5Home;
    DWORD64 P6Home;
    DWORD ContextFlags;
    DWORD MxCsr;
    WORD   SegCs;
    WORD   SegDs;
    WORD   SegEs;
    WORD   SegFs;
    WORD   SegGs;
    WORD   SegSs;
    DWORD EFlags;
    DWORD64 Dr0;
    DWORD64 Dr1;
    DWORD64 Dr2;
    DWORD64 Dr3;
    DWORD64 Dr6;
    DWORD64 Dr7;
    DWORD64 Rax;
    DWORD64 Rcx;
    DWORD64 Rdx;
    DWORD64 Rbx;
    DWORD64 Rsp;
    DWORD64 Rbp;
    DWORD64 Rsi;
    DWORD64 Rdi;
    DWORD64 R8;
    DWORD64 R9;
    DWORD64 R10;
    DWORD64 R11;
    DWORD64 R12;
    DWORD64 R13;
    DWORD64 R14;
    DWORD64 R15;
    DWORD64 Rip;
    union {
        XMM_SAVE_AREA_64 FltSave;
        struct {
            M128A Header[2];
            M128A Legacy[8];
            M128A Xmm0;
            M128A Xmm1;
            M128A Xmm2;
            M128A Xmm3;
            M128A Xmm4;
            M128A Xmm5;
            M128A Xmm6;
            M128A Xmm7;
            M128A Xmm8;
            M128A Xmm9;
            M128A Xmm10;
            M128A Xmm11;
            M128A Xmm12;
            M128A Xmm13;
            M128A Xmm14;
            M128A Xmm15;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    M128A VectorRegister[26];
    DWORD64 VectorControl;
    DWORD64 DebugControl;
    DWORD64 LastBranchToRip;
    DWORD64 LastBranchFromRip;
    DWORD64 LastExceptionToRip;
    DWORD64 LastExceptionFromRip;
} CONTEXT64;

#define CONTEXT_TYPE CONTEXT64
#else
#define CONTEXT_TYPE CONTEXT
#endif


#define SET_CONTEXT_OFFSET(DynReg, Field) m[DynReg] = std::pair<int, int>((int) ((unsigned long) &(context_ptr->Field)), (int) sizeof(context_ptr->Field))

unsigned int getContextSize64() {
	return (unsigned int) sizeof(CONTEXT_TYPE);
}

void getContextOffsets64(std::map<Dyninst::MachRegister, std::pair<int, int> > &m)
{
	CONTEXT_TYPE *context_ptr = (CONTEXT_TYPE *) 0x0;
	SET_CONTEXT_OFFSET(Dyninst::x86_64::rax, Rax);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::rcx, Rcx);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::rdx, Rdx);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::rdx, Rbx);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::rbx, Rsp);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::rbp, Rbp);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::rsi, Rsi);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::rdi, Rdi);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::r8, R8);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::r9, R9);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::r10, R10);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::r11, R11);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::r12, R12);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::r13, R13);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::r14, R14);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::r15, R15);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::rip, Rip);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::dr0, Dr0);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::dr1, Dr1);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::dr2, Dr2);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::dr3, Dr3);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::dr6, Dr6);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::dr7, Dr7);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::ds, SegDs);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::es, SegEs);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::fs, SegFs);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::gs, SegGs);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::cs, SegCs);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::ss, SegSs);
	SET_CONTEXT_OFFSET(Dyninst::x86_64::flags, EFlags);
}
