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

#include "common/h/dthread.h"
#include <Windows.h>
#include <cassert>

DThread::DThread() :
	live(false)
{
}

DThread::~DThread()
{
}

typedef struct {
   DThread::initial_func_t func;
   void *param;
} initial_data;


static DWORD WINAPI thread_init(LPVOID d) 
{
   assert(d);
   initial_data *data = (initial_data *) d;
   DThread::initial_func_t func = data->func;
   void *param = data->param;
   delete data;
   func(param);
   return NULL;
}

bool DThread::spawn(initial_func_t func, void *param)
{
   assert(!live);
   initial_data *data = new initial_data();
   data->func = func;
   data->param = param;

   thrd = CreateThread(NULL, 0, thread_init, data, 0, NULL);
   assert(thrd);
   live = true;
   return true;
}

bool DThread::join()
{
	WaitForSingleObject(thrd, INFINITE);
	CloseHandle(thrd);
	live = false;
	return true;
}

long DThread::id()
{
	return (long) GetCurrentThreadId();
}

Mutex::Mutex(bool)
{
	InitializeCriticalSection(&mutex);
}

Mutex::~Mutex()
{
	DeleteCriticalSection(&mutex);
}

bool Mutex::lock()
{
	EnterCriticalSection(&mutex);
	return true;
}

bool Mutex::unlock()
{
	LeaveCriticalSection(&mutex);
	return true;
}

CondVar::CondVar(Mutex *m)
{
   if (!m) {
      created_mutex = true;
      mutex = new Mutex();
   }
   else {
      created_mutex = false;
      mutex = m;
   }
   InitializeConditionVariable(&cond);
}

CondVar::~CondVar()
{
	if (created_mutex) {
		delete mutex;
	}
	mutex = NULL;
}

bool CondVar::unlock()
{
	mutex->unlock();
	return true;
}

bool CondVar::lock()
{
	mutex->lock();
	return true;
}

bool CondVar::signal()
{
	WakeConditionVariable(&cond);
	return true;
}

bool CondVar::broadcast()
{
	WakeAllConditionVariable(&cond);
	return true;
}

bool CondVar::wait()
{
	BOOL result = SleepConditionVariableCS(&cond, &mutex->mutex, INFINITE);
	assert(result);
	return true;
}