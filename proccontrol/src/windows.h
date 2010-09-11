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

#if !defined(pc_windows_h_)
#define pc_windows_h_

#include "proccontrol/src/int_process.h"
#include "proccontrol/src/x86_process.h"

class windows_process : public x86_process
{
 public:
   windows_process(Dyninst::PID p, std::string e, std::vector<std::string> a, std::map<int,int> f);
   windows_process(Dyninst::PID pid_, int_process *p);
   virtual ~windows_process();

   virtual bool plat_create();
   virtual bool plat_attach();

   virtual bool plat_contProcess();
   virtual bool plat_forked();
   virtual bool plat_execed();

   virtual bool plat_detach();
   virtual bool plat_terminate(bool &needs_sync);

   virtual bool needIndividualThreadAttach();
   virtual bool getThreadLWPs(std::vector<Dyninst::LWP> &lwps);

   virtual Dyninst::Architecture getTargetArch();
   virtual unsigned getTargetPageSize();
   virtual Dyninst::Address plat_mallocExecMemory(Dyninst::Address min, unsigned size);

   virtual bool plat_individualRegAccess();

   virtual bool plat_createDeallocationSnippet(Dyninst::Address addr, unsigned long size, void* &buffer, 
                                               unsigned long &buffer_size, unsigned long &start_offset);
   virtual bool plat_createAllocationSnippet(Dyninst::Address addr, bool use_addr, unsigned long size, 
                                             void* &buffer, unsigned long &buffer_size, 
                                             unsigned long &start_offset);
   virtual bool plat_collectAllocationResult(int_thread *thr, reg_response::ptr resp);

   virtual SymbolReaderFactory *plat_defaultSymReader();


   virtual bool plat_readMem(int_thread *thr, void *local, 
                             Dyninst::Address remote, size_t size);
   virtual bool plat_writeMem(int_thread *thr, void *local, 
                              Dyninst::Address remote, size_t size);

   virtual bool plat_needsAsyncIO() const;

   virtual bool refresh_libraries(std::set<int_library *> &added_libs,
                                  std::set<int_library *> &rmd_libs,
                                  std::set<response::ptr> &async_responses);
   virtual bool initLibraryMechanism();
   virtual bool plat_isStaticBinary();
private:
	PROCESS_INFORMATION procInfo;
	bool procinfo_set;
};

class windows_thread : public int_thread
{
public:
   virtual bool plat_getAllRegisters(int_registerPool &pool);
   virtual bool plat_getRegister(Dyninst::MachRegister reg, Dyninst::MachRegisterVal &val);
   virtual bool plat_setAllRegisters(int_registerPool &pool);
   virtual bool plat_setRegister(Dyninst::MachRegister reg, Dyninst::MachRegisterVal val);

   virtual bool plat_suspend();
   virtual bool plat_resume();

   virtual bool plat_cont();
   virtual bool plat_stop();

   virtual bool attach();
private:
	HANDLE hThread;

	void *getContextForThread(unsigned &alloc_size);
};

#endif