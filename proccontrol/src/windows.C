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

#include "proccontrol/h/Handler.h"
#include "proccontrol/src/int_handler.h"
#include "proccontrol/src/windows.h"
#include "proccontrol/src/procpool.h"
#include "common/h/Types.h"
#include <Windows.h>
#include <WinNT.h>

static void printSysError(unsigned errNo);

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
	//Trigger a call to plat_create_gen on the generator.
	return GeneratorWindows::the_generator->spawnProc(this);	
}

bool windows_process::plat_create_gen()
{
	//As called from the generator thread
	std::string arg_str = executable.c_str();
	for (unsigned i=0; i<argv.size(); i++) {
		arg_str += std::string(" ") + argv[i];
	}

    STARTUPINFO stinfo;
    memset(&stinfo, 0, sizeof(STARTUPINFO));
    stinfo.cb = sizeof(STARTUPINFO);
	pthrd_printf("Calling CreateProcess on %s %s\n", executable.c_str(), arg_str.c_str());
    BOOL result = CreateProcess(executable.c_str(), const_cast<char *>(arg_str.c_str()),
		                        NULL, NULL, FALSE,
		                        /*CREATE_SUSPENDED | */DEBUG_PROCESS,
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

Dyninst::Architecture windows_process::getTargetArch()
{
	return Dyninst::Arch_x86; //TODO
}

unsigned windows_process::getTargetPageSize()
{
    return 0;
}

 bool windows_process::getThreadLWPs(std::vector<Dyninst::LWP> &)
 {
	 return true;
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

DEBUG_EVENT *windows_process::getEventCause(Event::ptr ev)
{
	DEBUG_EVENT *ret = NULL;
	event_causes_lock.lock();
	std::map<Event::ptr, DEBUG_EVENT *>::iterator i = event_causes.find(ev);
	if (i != event_causes.end()) {
		ret = i->second;
	}
	event_causes_lock.unlock();
	return ret;
}

void windows_process::addEventCause(Event::ptr ev, const DEBUG_EVENT &dev)
{
	DEBUG_EVENT *new_dev = (DEBUG_EVENT *) malloc(sizeof(DEBUG_EVENT));
	memcpy(new_dev, &dev, sizeof(DEBUG_EVENT));

	event_causes_lock.lock();
	assert(event_causes.find(ev) == event_causes.end());
	event_causes[ev] = new_dev;
	event_causes_lock.unlock();
}

void windows_process::rmEventCause(Event::ptr ev)
{
	DEBUG_EVENT *old_dev = NULL;
	event_causes_lock.lock();
	std::map<Event::ptr, DEBUG_EVENT *>::iterator i = event_causes.find(ev);
	assert(i != event_causes.end());
	old_dev = i->second;
	event_causes_lock.unlock();
	free(old_dev);
}

PROCESS_INFORMATION *windows_process::getProcInfo()
{
	if (!procinfo_set)
		return NULL;
	return &procInfo;
}

int_process *int_process::createProcess(Dyninst::PID p, std::string e)
{
   std::vector<std::string> a;
   std::map<int,int> f;
   windows_process *newproc = new windows_process(p, e, a, f);
   return static_cast<int_process *>(newproc);
}

int_process *int_process::createProcess(std::string e, std::vector<std::string> a, std::map<int,int> f)
{
   windows_process *newproc = new windows_process(0, e, a, f);
   return static_cast<int_process *>(newproc);
}

int_process *int_process::createProcess(Dyninst::PID pid_, int_process *p)
{
   windows_process *newproc = new windows_process(pid_, p);
   assert(newproc);
   return static_cast<int_process *>(newproc);
}

windows_thread::windows_thread(int_process *proc, Dyninst::THR_ID thr_id, Dyninst::LWP lwp_id) :
	int_thread(proc, thr_id, lwp_id)
{
}

windows_thread::~windows_thread()
{
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
				return false;
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

GeneratorWindows *GeneratorWindows::the_generator;
GeneratorWindows::GeneratorWindows() :
	GeneratorMT("Windows Generator"),
	generator_thread_handle(INVALID_HANDLE_VALUE)
{
	assert(!the_generator);
	the_generator = this;
	decoders.insert(new DecoderWindows());
}

GeneratorWindows::~GeneratorWindows()
{
	if (generator_thread_handle != INVALID_HANDLE_VALUE)
		CloseHandle(generator_thread_handle);
	generator_thread_handle = INVALID_HANDLE_VALUE;
	the_generator = NULL;
}

bool GeneratorWindows::initialize()
{
	generator_thread_handle = OpenThread(THREAD_ALL_ACCESS, false, GetCurrentThreadId());
	return true;
}

bool GeneratorWindows::canFastHandle()
{
	return false;
}

ArchEvent *GeneratorWindows::getEvent(bool block)
{
	pthrd_printf("Calling WaitForDebugEvent: %s\n", block ? "Blocking" : "Poll");
	DEBUG_EVENT ev;

	int waittime = block ? INFINITE : 0;
	BOOL result;

retry:
	result = WaitForDebugEvent(&ev, waittime);
	if (result == FALSE) {
		DWORD err = GetLastError();
		if (err == WAIT_TIMEOUT || err == ERROR_SEM_TIMEOUT) {
			if (!block) goto retry;
			pthrd_printf("No debug event available during poll call\n");
			return NULL;
		}
		setLastError(err_internal, "Error calling WaitForDebugEvent\n");
		perr_printf("Error calling WaitForDebugEvent\n");
		printSysError(err);
		return NULL;
	}
	pthrd_printf("Successful return from waitForDebugEvent\n");

	ArchEventWindows *aew = new ArchEventWindows(ev);
	return static_cast<ArchEvent *>(aew);
}

bool GeneratorWindows::isInterrupted()
{
	return interrupt.is_interrupted;
}

void GeneratorWindows::handleInterrupt()
{
	interrupt.interrupt_lock.lock();

	switch (interrupt.interruptOp) {
		case InterruptData::proc_spawn:
			interrupt.ret_value = interrupt.interruptData.proc_spawn_data.proc->plat_create_gen();
			break;
		default:
			assert(0);
	}
	interrupt.is_interrupted = false;

	interrupt.interrupt_lock.broadcast();
	interrupt.interrupt_lock.unlock();
}

bool GeneratorWindows::spawnProc(windows_process *proc)
{
	pthrd_printf("Interrupting generator to trigger a spawnProc\n");
	interrupt.interrupt_lock.lock();

	interrupt.interruptData.proc_spawn_data.proc = proc;
	interrupt.interruptOp = InterruptData::proc_spawn;

	return triggerInterrupt();
}

bool GeneratorWindows::triggerInterrupt()
{
	//interrupt lock should be held
	interrupt.is_interrupted = true;

	//Frees generator if it's in process block.
	ProcPool()->condvar()->lock();
	ProcPool()->condvar()->broadcast();
	ProcPool()->condvar()->unlock();

	//TODO, only if resume needed.
	//ResumeThread(generator_thread_handle);

	while (interrupt.is_interrupted) {
		pthrd_printf("Waiting for interrupt to complete\n");
		interrupt.interrupt_lock.wait();
	}

	bool ret_value = interrupt.ret_value;
	interrupt.interruptOp = InterruptData::no_op;
	interrupt.interrupt_lock.unlock();
	return ret_value;
}

InterruptData::InterruptData() :
	interruptOp(no_op),
	is_interrupted(false),
	ret_value(false)
{
}

InterruptData::~InterruptData()
{
}

ArchEventWindows::ArchEventWindows(const DEBUG_EVENT &e) :
	ArchEvent(),
	ev(e)
{
#define strcase(X) case X: name = #X;
	switch (ev.dwDebugEventCode) {
		strcase(CREATE_PROCESS_DEBUG_EVENT);
		strcase(CREATE_THREAD_DEBUG_EVENT);
		strcase(EXCEPTION_DEBUG_EVENT);
		strcase(EXIT_PROCESS_DEBUG_EVENT);
		strcase(EXIT_THREAD_DEBUG_EVENT);
		strcase(LOAD_DLL_DEBUG_EVENT);
		strcase(OUTPUT_DEBUG_STRING_EVENT);
		strcase(RIP_EVENT);
		strcase(UNLOAD_DLL_DEBUG_EVENT);
		default: name = "unknown";
	}
#undef strcase
}

ArchEventWindows::~ArchEventWindows()
{
}

DEBUG_EVENT &ArchEventWindows::getEvent()
{
	return ev;
}

DecoderWindows::DecoderWindows()
{
}

DecoderWindows::~DecoderWindows()
{
}

unsigned DecoderWindows::getPriority() const 
{
	return default_priority;
}

bool DecoderWindows::decode(ArchEvent *ae, std::vector<Event::ptr> &events)
{
	ArchEventWindows *aew = static_cast<ArchEventWindows *>(ae);
	DEBUG_EVENT &ev = aew->getEvent();
	pthrd_printf("Decoding %s on %d/%d\n", aew->getName().c_str(), ev.dwProcessId, ev.dwThreadId);

	int_process *proc = ProcPool()->findProcByPid(ev.dwProcessId); 
	windows_process *wproc = dynamic_cast<windows_process *>(proc);
	int_thread *thread = NULL;
	windows_thread *wthread = NULL;   
	if (proc) {
	   thread = proc->threadPool()->findThreadByLWP((Dyninst::LWP) ev.dwThreadId);
	   wthread = static_cast<windows_thread *>(thread);
	}
	pthrd_printf("Searched for process/thread, proc: %s, thread %s\n",
		         proc ? "found" : "not-found", thread ? "found" : "not-found");

	Event::ptr new_ev = Event::ptr();
	switch (ev.dwDebugEventCode) {
		case CREATE_PROCESS_DEBUG_EVENT: {
			if (wproc) {
				pthrd_printf("CREATE_PROCESS is a bootstrap event\n");
				new_ev = EventBootstrap::ptr(new EventBootstrap());
				new_ev->setSyncType(Event::sync_process);
				wproc->addEventCause(new_ev, ev);
				break;
			}
			assert(0);  //TODO: New process handling
			break;
		}
		case CREATE_THREAD_DEBUG_EVENT:
		case EXCEPTION_DEBUG_EVENT:
		case EXIT_PROCESS_DEBUG_EVENT:
		case EXIT_THREAD_DEBUG_EVENT:
		case LOAD_DLL_DEBUG_EVENT:
		case OUTPUT_DEBUG_STRING_EVENT:
		case RIP_EVENT:
		case UNLOAD_DLL_DEBUG_EVENT:
			break;
	}
	return true;
}

WindowsBootstrapHandler::WindowsBootstrapHandler() :
	Handler("Windows Bootstrap")
{
}

WindowsBootstrapHandler::~WindowsBootstrapHandler()
{
}

Handler::handler_ret_t WindowsBootstrapHandler::handleEvent(Event::ptr ev)
{
	int_process *proc = ev->getProcess()->llproc();
	windows_process *wproc = dynamic_cast<windows_process *>(proc);
	
	DEBUG_EVENT *dev = wproc->getEventCause(ev);

	PROCESS_INFORMATION *pinfo = wproc->getProcInfo();
	pinfo->hProcess = dev->u.CreateProcessInfo.hProcess;
	pinfo->hThread = dev->u.CreateProcessInfo.hThread;
	pinfo->dwProcessId = dev->dwProcessId;
	pinfo->dwThreadId = dev->dwThreadId;

	wproc->rmEventCause(ev);

	return ret_success;
}

int WindowsBootstrapHandler::getPriority() const
{
	return PrePlatformPriority;
}

void WindowsBootstrapHandler::getEventTypesHandled(std::vector<EventType> &etypes)
{
	etypes.push_back(EventType(EventType::Bootstrap));
}

HandlerPool *plat_createDefaultHandlerPool(HandlerPool *hpool)
{
   static bool initialized = false;
   static WindowsBootstrapHandler *wbootstrap = NULL;
   if (!initialized) {
      wbootstrap = new WindowsBootstrapHandler();
      initialized = true;
   }
   hpool->addHandler(wbootstrap);
   return hpool;
}

Generator *Generator::getDefaultGenerator()
{
   static GeneratorWindows *gen = NULL;
   if (!gen) {
      gen = new GeneratorWindows();
      assert(gen);
      gen->launch();
   }
   return static_cast<Generator *>(gen);
}

int_process::ThreadControlMode int_process::getThreadControlMode() {
    return int_process::IndependentLWPControl;
}

int_thread *int_thread::createThreadPlat(int_process *proc, 
                                         Dyninst::THR_ID thr_id, 
                                         Dyninst::LWP lwp_id,
                                         bool initial_thrd)
{
   windows_process *wproc = dynamic_cast<windows_process *>(proc);
   if (initial_thrd) {
      lwp_id = wproc->getProcInfo()->dwThreadId;
   }
   windows_thread *wthrd = new windows_thread(proc, thr_id, lwp_id);
   assert(wthrd);
   return static_cast<int_thread *>(wthrd);
}

bool ProcessPool::LWPIDsAreUnique()
{
	return true;
}

int_notify *int_notify::plat_createNotify()
{
	return static_cast<int_notify *>(new windows_notify());
}

windows_notify::windows_notify() :
	hevent(INVALID_HANDLE_VALUE)
{
}

windows_notify::~windows_notify()
{
}

bool windows_notify::plat_init()
{
   hevent = CreateEvent(NULL, true, false, NULL);
   assert(hevent != INVALID_HANDLE_VALUE);
   return true;
}

void windows_notify::plat_noteEvent()
{
	BOOL result = SetEvent(hevent);
	assert(result == TRUE);
}

void windows_notify::plat_clearEvent()
{
	BOOL result = ResetEvent(hevent);
	assert(result == TRUE);
}

void *windows_notify::plat_getHandle()
{
	return (void *) hevent;
}

static void printSysError(unsigned errNo) {
    char buf[1000];
    BOOL result = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errNo, 
		  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		  buf, 1000, NULL);
    if (result == FALSE) {
        pthrd_printf("Couldn't print error message\n");
        printSysError(GetLastError());
    }
    pthrd_printf("System error [%d]: %s\n", errNo, buf);
}