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

#if !defined(DTHREAD_H_)
#define DTHREAD_H_

#include <stdlib.h>
#include "dynutil/h/dyntypes.h"

#if !defined(os_windows)
#define cap_pthreads
#include <pthread.h>
#else
#include <Windows.h>
#endif

class DThread {
#if defined(cap_pthreads)
   pthread_t thrd;
#elif defined(os_windows)
   HANDLE thrd;
   long id_;
#endif
   bool live;   
 public:
   COMMON_EXPORT DThread();
   COMMON_EXPORT ~DThread();
   typedef void (*initial_func_t)(void *);

   COMMON_EXPORT static long self();
   COMMON_EXPORT bool spawn(initial_func_t func, void *param);
   COMMON_EXPORT bool join();
   COMMON_EXPORT long id();
};

class Mutex {
   friend class CondVar;
#if defined(cap_pthreads)
   pthread_mutex_t mutex;
#elif defined(os_windows)
   HANDLE mutex;
#endif
 public:
   COMMON_EXPORT Mutex(bool recursive=false);
   COMMON_EXPORT ~Mutex();

   COMMON_EXPORT bool lock();
   COMMON_EXPORT bool unlock();
};

class CondVar {
#if defined(cap_pthreads)
   pthread_cond_t cond;
#elif defined(os_windows)
   int waiters_count_;
   CRITICAL_SECTION waiters_count_lock_;
   HANDLE sema_;
   HANDLE waiters_done_;
   size_t was_broadcast_;
#endif
   Mutex *mutex;
   bool created_mutex;
 public:
	COMMON_EXPORT CondVar(Mutex *m = NULL);
    COMMON_EXPORT ~CondVar();

    COMMON_EXPORT bool unlock();
    COMMON_EXPORT bool lock();
    COMMON_EXPORT bool signal();
    COMMON_EXPORT bool broadcast();
    COMMON_EXPORT bool wait();
};

#endif
