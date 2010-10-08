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

   DWORD id_int;
   thrd = CreateThread(NULL, 0, thread_init, data, 0, &id_int);
   id_ = (long) id_int;
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
	return id_;
}

long DThread::self()
{
	return (long) GetCurrentThreadId();
}

Mutex::Mutex(bool)
{
	mutex = CreateMutex(NULL, FALSE, NULL);
}

Mutex::~Mutex()
{
	CloseHandle(mutex);
}

bool Mutex::lock()
{
	int result = WaitForSingleObject(mutex, INFINITE);
	return result == WAIT_OBJECT_0;
}

bool Mutex::unlock()
{
	BOOL result = ReleaseMutex(mutex);
	return (result == TRUE);
}

/**
 * This condition variable code is based on the code described at: 
 *  http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
 * The code comes from the ACE project, which was licensed under the following:
 *
 * ACE(TM), TAO(TM), CIAO(TM), and CoSMIC(TM) (henceforth referred to as 
 * "DOC software") are copyrighted by Douglas C. Schmidt and his research 
 * group at Washington University, University of California, Irvine, and 
 * Vanderbilt University, Copyright (c) 1993-2009, all rights reserved. 
 * Since DOC software is open-source, freely available software, you are 
 * free to use, modify, copy, and distribute--perpetually and irrevocably--the
 * DOC software source code andobject code produced from the source, as well
 * as copy and distribute modified versions of this software. You must, 
 * however, include this copyright statement along with any code built using 
 * DOC software that you release. No copyright statement needs to be provided 
 * if you just ship binary executables of your software products.
 *
 * You can use DOC software in commercial and/or binary software releases and 
 * are under no obligation to redistribute any of your source code that is 
 * built using DOC software. Note, however, that you may not misappropriate the
 * DOC software code, such as copyrighting it yourself or claiming authorship 
 * of the DOC software code, in a way that will prevent DOC software from being 
 * distributed freely using an open-source development model. You needn't 
 * inform anyone that you're using DOC software in your software, though we 
 * encourage you to let us know so we can promote your project in the DOC software
 * success stories.
 * 
 * The ACE, TAO, CIAO, and CoSMIC web sites are maintained by the DOC Group at
 * the Institute for Software Integrated Systems (ISIS) and the Center for 
 * Distributed Object Computing of Washington University, St. Louis for the 
 * development of open-source software as part of the open-source software
 * community.  Submissions are provided by the submitter ``as is'' with no 
 * warranties whatsoever, including any warranty of merchantability, 
 * noninfringement of third party intellectual property, or fitness for any 
 * particular purpose. In no event shall the submitter be liable for any 
 * direct, indirect, special, exemplary, punitive, or consequential damages, 
 * including without limitation, lost profits, even if advised of the 
 * possibility of such damages. Likewise, DOC software is provided as is with
 * no warranties of any kind, including the warranties of design, 
 * merchantability, and fitness for a particular purpose, noninfringement, or
 * arising from a course of dealing, usage or trade practice. Washington 
 * University, UC Irvine, Vanderbilt University, their employees, and students
 * shall have no liability with respect to the infringement of copyrights, trade 
 * secrets or any patents by DOC software or any part thereof. Moreover, in no 
 * event will Washington University, UC Irvine, or Vanderbilt University, their
 * employees, or students be liable for any lost revenue or profits or other 
 * special, indirect and consequential damages.
 * 
 * DOC software is provided with no support and without any obligation on the
 * part of Washington University, UC Irvine, Vanderbilt University, their 
 * employees, or students to assist in its use, correction, modification, or 
 * enhancement. A number of companies around the world provide commercial 
 * support for DOC software, however. DOC software is Y2K-compliant, as long
 *  as the underlying OS platform is Y2K-compliant. Likewise, DOC software is 
 * compliant with the new US daylight savings rule passed by Congress as "The 
 * Energy Policy Act of 2005," which established new daylight savings times (DST) 
 * rules for the United States that expand DST as of March 2007. Since DOC 
 * software obtains time/date and calendaring information from operating systems 
 * users will not be affected by the new DST rules as long as they upgrade their 
 * operating systems accordingly.
 *
 * The names ACE(TM), TAO(TM), CIAO(TM), CoSMIC(TM), Washington University, UC 
 * Irvine, and Vanderbilt University, may not be used to endorse or promote 
 * products or services derived from this source without express written 
 * permission from Washington University, UC Irvine, or Vanderbilt University.
 * This license grants no permission to call products or services derived from 
 * this source ACE(TM), TAO(TM), CIAO(TM), or CoSMIC(TM), nor does 
 * it grant permission for the name Washington University, UC Irvine, or 
 * Vanderbilt University to appear in their names. 
 **/
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

  waiters_count_ = 0;
  was_broadcast_ = 0;
  sema_ = CreateSemaphore(NULL,       // no security
                          0,          // initially 0
                          0x7fffffff, // max count
                          NULL);      // unnamed 
  InitializeCriticalSection (&waiters_count_lock_);
  waiters_done_ = CreateEvent(NULL,  // no security
                              FALSE, // auto-reset
                              FALSE, // non-signaled initially
                              NULL); // unnamed
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
    EnterCriticalSection (&waiters_count_lock_);
    int have_waiters = waiters_count_ > 0;
    LeaveCriticalSection (&waiters_count_lock_);

    // If there aren't any waiters, then this is a no-op.  
    if (have_waiters)
      ReleaseSemaphore (sema_, 1, 0);
	return true;
}

bool CondVar::broadcast()
{
   // This is needed to ensure that <waiters_count_> and <was_broadcast_> are
   // consistent relative to each other.
   EnterCriticalSection (&waiters_count_lock_);
   int have_waiters = 0;

   if (waiters_count_ > 0) {
	  // We are broadcasting, even if there is just one waiter...
      // Record that we are broadcasting, which helps optimize
      // <pthread_cond_wait> for the non-broadcast case.
      was_broadcast_ = 1;
      have_waiters = 1;
   }

   if (have_waiters) {
	  // Wake up all the waiters atomically.
      ReleaseSemaphore (sema_, waiters_count_, 0);

      LeaveCriticalSection (&waiters_count_lock_);

      // Wait for all the awakened threads to acquire the counting
      // semaphore. 
      WaitForSingleObject (waiters_done_, INFINITE);
      // This assignment is okay, even without the <waiters_count_lock_> held 
      // because no other waiter threads can wake up to access it.
      was_broadcast_ = 0;
    }
    else
      LeaveCriticalSection (&waiters_count_lock_);

    return true;
}

bool CondVar::wait()
{
    // Avoid race conditions.
    EnterCriticalSection (&waiters_count_lock_);
    waiters_count_++;
    LeaveCriticalSection (&waiters_count_lock_);

    // This call atomically releases the mutex and waits on the
    // semaphore until <signal> or <broadcast>
	// are called by another thread.
	SignalObjectAndWait (mutex->mutex, sema_, INFINITE, FALSE);

	// Reacquire lock to avoid race conditions.
    EnterCriticalSection (&waiters_count_lock_);

    // We're no longer waiting...
    waiters_count_--;

    // Check to see if we're the last waiter after <pthread_cond_broadcast>.
    int last_waiter = was_broadcast_ && waiters_count_ == 0;

    LeaveCriticalSection (&waiters_count_lock_);

    // If we're the last waiter thread during this particular broadcast
    // then let all the other threads proceed.
    if (last_waiter)
      // This call atomically signals the <waiters_done_> event and waits until
      // it can acquire the <mutex>.  This is required to ensure fairness. 
      SignalObjectAndWait (waiters_done_, mutex->mutex, INFINITE, FALSE);
    else
      // Always regain the external mutex since that's the guarantee we
      // give to our callers. 
      WaitForSingleObject (mutex->mutex, INFINITE);
	return true;
}
