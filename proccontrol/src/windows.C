/*
 * Copyright (c) 1996-2009 Barton P. Miller
 * 
 * We provide the Paradyn Parallel Performance Tools (below
 * described as "Paradyn") on an AS IS basis, and do not warrant its
 * validity or performance.  We reserve the right to update, modify,
 * or discontinue this software at any time.  We shall have no
 * obligation to supply such updates or modifications or any other
 * form of support to you.
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

#include "proccontrol/src/windows.h"
#include "common/h/Types.h"
#include <Windows.h>
#include <WinNT.h>

windows_process::windows_process(Dyninst::PID p, std::string e, std::vector<std::string> a, std::map<int,int> f) :
	int_process(p, e, a, f),
	x86_process(p, e, a, f),
	procinfo_set(false)
{
}

windows_process::windows_process(Dyninst::PID pid_, int_process *p) :
	int_process(pid_, p),
	x86_process(pid_, p),
	procinfo_set(false)
{
}

windows_process::~windows_process()
{
}

bool windows_process::plat_create()
{
	std::string arg_str = executable.c_str();
	for (unsigned i=0; i<argv.size(); i++) {
		arg_str += std::string(" ") + argv[i];
	}

    STARTUPINFO stinfo;
    memset(&stinfo, 0, sizeof(STARTUPINFO));
    stinfo.cb = sizeof(STARTUPINFO);

    BOOL result = CreateProcess(executable.c_str(), const_cast<char *>(arg_str.c_str()),
		                        NULL, NULL, FALSE,
		                        CREATE_SUSPENDED | DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS,
		                        NULL, NULL, &stinfo, &procInfo);
	if (result == FALSE) {
	   int err = GetLastError();
	   perr_printf("Error creating new process object %s\n", executable.c_str());
	   setLastError(err_nofile, "Failed to launch new process during create\n");
       return false;
    }

	pid = procInfo.dwProcessId;
	procinfo_set = true;
	return true;   
}

bool windows_process::plat_attach()
{
	BOOL result = DebugActiveProcess(pid);
	if (!result) {
		perr_printf("Failed to attach to new process during attach\n");
		setLastError(err_noproc, "Failed to attach to new process during attach\n");
		return false;
	}

	return true;
}

bool windows_process::plat_contProcess()
{
	return true;
}

bool windows_process::plat_forked()
{
	setLastError(err_internal, "Fork not supported on windows\n");
	perr_printf("Attempted to handle a fork on windows\n");
	return false;
}

bool windows_process::plat_execed()
{
	setLastError(err_internal, "Fork not supported on windows\n");
	perr_printf("Attempted to handle a fork on windows\n");
	return false;
}

 bool windows_process::plat_detach()
 {
	 assert(0); //TODO
	 return false;
 }

 bool windows_process::plat_terminate(bool &needs_sync)
 {
	 assert(0); //TODO
	 return false;
 }

 bool windows_process::needIndividualThreadAttach()
 {
	 return false;
 }

Dyninst::Architecture getTargetArch()
{
	return Dyninst::Arch_x86; //TODO
}

 unsigned windows_process::getTargetPageSize()
 {
	 return 0;
 }

Dyninst::Address windows_process::plat_mallocExecMemory(Dyninst::Address, unsigned)
{
	return 0;
}

 bool windows_process::plat_individualRegAccess()
 {
	 return false;
 }

 bool windows_process::plat_createDeallocationSnippet(Dyninst::Address, unsigned long, void* &, 
                                                      unsigned long &, unsigned long &)
 {
	 //Not needed on Windows--we can allocate and deallocate memory with-out an iRPC
	 return false;
 }

 bool windows_process::plat_createAllocationSnippet(Dyninst::Address, bool, unsigned long, 
                                                            void* &, unsigned long &, unsigned long &)
 {
	 //Not needed on Windows--we can allocate and deallocate memory with-out an iRPC
	 return false;
 }

 bool windows_process::plat_collectAllocationResult(int_thread *thr, reg_response::ptr resp)
 {
	 //Not needed on Windows--we can allocate and deallocate memory with-out an iRPC
	 return false;
 }

SymbolReaderFactory *windows_process::plat_defaultSymReader()
{
	return NULL;
}

 bool windows_process::plat_readMem(int_thread *, void *local, Dyninst::Address remote, size_t size)
 {
	 BOOL result = ReadProcessMemory(procInfo.hProcess, (LPCVOID) remote, local, size, NULL);
	 if (result == FALSE) {
		 perr_printf("Error reading from process %d memory at 0x%lx +%lu\n", pid, (unsigned long) remote, (unsigned long) size);
		 setLastError(err_memaccess, "Failed to read from process\n");
		 return false;
	 }
	 return true;
 }

 bool windows_process::plat_writeMem(int_thread *, void *local, Dyninst::Address remote, size_t size)
 {
	 BOOL result = WriteProcessMemory(procInfo.hProcess, (LPVOID) remote, local, size, NULL);
	 if (result == FALSE) {
		 perr_printf("Error reading from process %d memory at 0x%lx +%lu\n", pid, (unsigned long) remote, (unsigned long) size);
		 setLastError(err_memaccess, "Failed to write to process\n");
		 return false;
	 }
	 return true;
 }

 bool windows_process::plat_needsAsyncIO() const
 {
	 return false;
 }

bool windows_process::refresh_libraries(std::set<int_library *> &,
                                        std::set<int_library *> &,
                                        std::set<response::ptr> &)
{
	return true;
}

bool windows_process::initLibraryMechanism()
{
	return true;
}

 bool windows_process::plat_isStaticBinary()
 {
	return false;
 }

extern void getContextOffsets64(std::map<Dyninst::MachRegister, std::pair<int, int> > &m);
extern void getContextOffsets32(std::map<Dyninst::MachRegister, std::pair<int, int> > &m);
extern unsigned int getContextSize64();
extern unsigned int getContextSize32();

static std::map<Dyninst::MachRegister, std::pair<int, int> >&getRegOffsets(Dyninst::Architecture arch, unsigned int &context_size)
{
	static std::map<Dyninst::MachRegister, std::pair<int, int> > regs32;
	bool regs32_init = false;
	static std::map<Dyninst::MachRegister, std::pair<int, int> > regs64;
	bool regs64_init = false;

	if (arch == Dyninst::Arch_x86) {
		if (!regs32_init) {
			getContextOffsets32(regs32);
			regs32_init = true;
		}
		context_size = getContextSize32();
		return regs32;
	}
	else if (arch == Dyninst::Arch_x86_64) {
		if (!regs64_init) {
			getContextOffsets64(regs64);
			regs64_init = true;
		}
		context_size = getContextSize64();
		return regs64;
	}
	else {
		assert(0);
		return regs32;
	}
}

void *windows_thread::getContextForThread(unsigned &alloc_size)
{
	alloc_size *= 2; //Windows could grow the context, this gives us some space if they do
	void *context_buffer = malloc(alloc_size); 
	assert(context_buffer);
	((int *) context_buffer)[(alloc_size/sizeof(int))-1] = 0x1a2b3c4d;

	BOOL result = GetThreadContext(hThread, (LPCONTEXT) context_buffer);
	assert(((int *) context_buffer)[(alloc_size/sizeof(int))-1] == 0x1a2b3c4d); //Make sure no overflow happened
	if (!result) {
		setLastError(err_internal, "Error getting thread context\n");
		perr_printf("Could not get thread context for %lx\n", getLWP());
		return NULL;
	}
	return context_buffer;
}

bool windows_thread::plat_getAllRegisters(int_registerPool &pool)
{
	pthrd_printf("Plat getting all registers for thread %d/%d\n", llproc()->getPid(), getLWP());
	unsigned int alloc_size = 0;
	std::map<Dyninst::MachRegister, std::pair<int, int> > &offsets = getRegOffsets(llproc()->getTargetArch(), alloc_size);

	void *context_buffer = getContextForThread(alloc_size);
	unsigned char *c_context_buffer = (unsigned char *) context_buffer;
	if (!context_buffer) {
		return false;
	}

	std::map<Dyninst::MachRegister, std::pair<int, int> >::iterator i;
	for (i = offsets.begin(); i != offsets.end(); i++) {
		Dyninst::MachRegister reg = i->first;
		int offset = i->second.first;
		int size = i->second.second;

		Dyninst::MachRegisterVal val;
		/**
		 * I could just memcpy here rather than use casts (since I believe we'll only use
		 * windows on little endian platforms), but I feel better about casting with 
		 * appropriate sizes.
		 **/
		switch (size) {
			case 1:
				val = (Dyninst::MachRegisterVal) *(c_context_buffer+offset);
				break;
			case 2:
				val = (Dyninst::MachRegisterVal) *((uint16_t *) (c_context_buffer+offset));
				break;
			case 4:
				val = (Dyninst::MachRegisterVal) *((uint32_t *) (c_context_buffer+offset));
				break;
			case 8:
				val = (Dyninst::MachRegisterVal) *((uint64_t *) (c_context_buffer+offset));
				break;
			default:
				assert(0);
		}
		pool.regs[i->first] = val;
	}

	pool.reg_buffer = context_buffer;
	pool.reg_buffer_size = alloc_size;
	pool.full = true;

	return true;
}

bool windows_thread::plat_setAllRegisters(int_registerPool &pool)
{
	pthrd_printf("Plat getting all registers for thread %d/%d\n", llproc()->getPid(), getLWP());
	bool alloced_pool = false;
	unsigned int alloc_size = 0;
	std::map<Dyninst::MachRegister, std::pair<int, int> > &offsets = getRegOffsets(llproc()->getTargetArch(), alloc_size);

	void *context_buffer = NULL;
	if (pool.reg_buffer) {
		context_buffer = pool.reg_buffer;
		alloced_pool = false;
	}
	else {
		context_buffer = getContextForThread(alloc_size);
		alloced_pool = true;
	}
	unsigned char *c_context_buffer = (unsigned char *) context_buffer;

	std::map<Dyninst::MachRegister, std::pair<int, int> >::iterator i;
	for (i = offsets.begin(); i != offsets.end(); i++) {
		Dyninst::MachRegister reg = i->first;
		int offset = i->second.first;
		int size = i->second.second;
		Dyninst::MachRegisterVal val = pool.regs[reg];

		switch (size) {
			case 1:
				c_context_buffer[offset] = (unsigned char) val;
				break;
			case 2:
				*((uint16_t *) (c_context_buffer+offset)) = (uint16_t) val;
				break;
			case 4:
				*((uint32_t *) (c_context_buffer+offset)) = (uint32_t) val;
				break;
			case 8:
				*((uint64_t *) (c_context_buffer+offset)) = (uint64_t) val;
				break;
			default:
				assert(0);
		}
	}

	BOOL result = SetThreadContext(hThread, (const CONTEXT *) context_buffer);
	if (result == FALSE) {
		setLastError(err_internal, "Error setting thread context\n");
		perr_printf("Could not set thread context for %lx\n", getLWP());
	}
	if (alloced_pool)
		free(context_buffer);
	return (result == TRUE);
}

bool windows_thread::plat_getRegister(Dyninst::MachRegister, Dyninst::MachRegisterVal &)
{
	return false; //No individual register access on Windows
}

bool windows_thread::plat_setRegister(Dyninst::MachRegister reg, Dyninst::MachRegisterVal val)
{
	return false; //No individual register access on Windows
}

bool windows_thread::plat_suspend()
{
	return true;
}

bool windows_thread::plat_resume()
{
	return true;
}

bool windows_thread::plat_cont()
{
	int count = ResumeThread(hThread);
	if (count == -1) {
		perr_printf("Error plat resuming thread %d\n", getLWP());
		setLastError(err_internal, "Error resuming thread %d\n");
		return false;
	}
	return true;
}

bool windows_thread::plat_stop()
{
	int count = SuspendThread(hThread);
	if (count == -1) {
		perr_printf("Error plat suspending thread %d\n", getLWP());
		setLastError(err_internal, "Error suspending thread %d\n");
		return false;
	}
	return true;
}

bool windows_thread::attach()
{
	return true;
}
