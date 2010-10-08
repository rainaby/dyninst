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
#include "pcontrol_mutatee_tools.h"
#include "communication.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#if defined(os_windows_test)
#include <windows.h>
#if defined(__cplusplus)
extern "C" {
#endif
__declspec(dllimport) int _getpid(void);
#if defined(__cplusplus)
}
#endif

#endif

thread_t threads[MAX_POSSIBLE_THREADS];
int thread_results[MAX_POSSIBLE_THREADS];
int num_threads;
int sockfd;

typedef struct {
   thread_t thread_id;
   int thread_index;
   int (*func)(int, void*);
   void *data;
} datagram;

static testlock_t thread_startup_lock;

void *ThreadTrampoline(void *d)
{
   int (*func)(int, void*);
   datagram *datag;
   thread_t thread_id;
   void *data;
   int func_result;
      
   datag = (datagram *) d;
   func = datag->func;
   data = datag->data;
   free(datag);

   testLock(&thread_startup_lock);
   testUnlock(&thread_startup_lock);

   func_result = func(datag->thread_index, data);
   
   return (void *) (long) func_result;
}

int MultiThreadFinish() {
   int i=0;
   void *result;
   for (i = 0; i < num_threads; i++) 
   {
      result = joinThread(threads[i]);
      thread_results[i] = (int) ((long) result);
   }
   return 0;
}

int MultiThreadInit(int (*init_func)(int, void*), void *thread_data)
{
   int i, j;
   int is_mt = 0;
   num_threads = 0;
   for (i = 0; i < gargc; i++) {
      if ((strcmp(gargv[i], "-mt") == 0) && init_func) {
         is_mt = 1;
         num_threads = atoi(gargv[i+1]);
         break;
      }
   }

   if (is_mt) {
      initLock(&thread_startup_lock);
      testLock(&thread_startup_lock);
      for (j = 0; j < num_threads; j++) {
         datagram *data = (datagram *) malloc(sizeof(datagram));
         data->thread_index = j;
         data->func = init_func;
         data->data = thread_data;
         threads[j] = spawnNewThread((void *) ThreadTrampoline, (void *) data);
		 data->thread_id = threads[j];
      }
   }
   return 0;
}

#if defined(os_windows_test)
static int P_getpid()
{
	return _getpid();
}
#else
static int P_getpid()
{
	return getpid();
}
#endif

int handshakeWithServer()
{
   int result;
   handshake shake;
   send_pid spid;
   spid.code = SEND_PID_CODE;
   spid.pid = P_getpid();

   result = send_message((unsigned char *) &spid, sizeof(send_pid));
   if (result == -1) {
      fprintf(stderr, "Could not send PID\n");
      return -1;
   }

   result = recv_message((unsigned char *) &shake, sizeof(handshake));
   if (result != 0) {
      fprintf(stderr, "Error recieving message\n");
      return -1;
   }
   if (shake.code != HANDSHAKE_CODE) {
      fprintf(stderr, "Recieved unexpected message\n");
      return -1;
   }

   return 0;
}

int releaseThreads()
{
   if (num_threads == 0) {
      return 0;
   }

   testUnlock(&thread_startup_lock);
   return 0;
}

int initProcControlTest(int (*init_func)(int, void*), void *thread_data)
{
   int result = 0;

   if (init_func) {
      result = MultiThreadInit(init_func, thread_data);
   }
   if (result != 0) {
      fprintf(stderr, "Error initializing threads\n");
      return -1;
   }

   result = initMutatorConnection();
   if (result != 0) {
      fprintf(stderr, "Error initializing connection to mutator\n");
      return -1;
   }
   
   result = handshakeWithServer();
   if (result != 0) {
      fprintf(stderr, "Could not handshake with server\n");
      return -1;
   }

   result = releaseThreads();
   if (result != 0) {
      fprintf(stderr, "Could not release threads\n");
      return -1;
   }

   return 0;
}

int finiProcControlTest(int expected_ret_code)
{
   int i, result;
   int has_error = 0;
   if (num_threads == 0)
      return 0;

   result = MultiThreadFinish();
   if (result != 0) {
      fprintf(stderr, "Thread return values were not collected\n");
      return -1;
   }
   for (i = 0; i < num_threads; i++) {
      if (thread_results[i] != expected_ret_code) {
         fprintf(stderr, "Thread returned unexpected return code %d\n", thread_results[i]);
         has_error = 1;
      }
   }
   return has_error ? -1 : 0;
}

#if !defined(os_windows_test)
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#else
#include <WinSock.h>
#include <windows.h>

int initWindowsSocket()
{
   WSADATA data;

   // request WinSock 2.0
   if (WSAStartup(MAKEWORD(2,0), &data) == 0) {
      // verify that the version that was provided is one we can use
      if (LOBYTE(data.wVersion) != 2 || !HIBYTE(data.wVersion) != 0)
      {
		 logerror("Version error setting up socket\n");
         return -1;
      }
   }
   return 0;
}
#endif

int initMutatorConnection()
{
   int result;
   char *un_socket = NULL;
   char *inet_socket = NULL;
   struct hostent *hostptr;
   struct in_addr *inadr;
   struct sockaddr_in server_addr;
   int i;

   for (i = 0; i < gargc; i++) {
      if (strcmp(gargv[i], "-un_socket") == 0) {
         un_socket = gargv[i+1];
         break;
      }
	  if (strcmp(gargv[i], "-inet_socket") == 0) {
		  inet_socket = gargv[i+1];
		  break;
	  }
   }
   
#if !defined(os_windows_test)
   assert(un_socket);
   sockfd = socket(PF_UNIX, SOCK_STREAM, 0);
   if (sockfd == -1) {
      perror("Failed to create socket");
      return -1;
   }
     
   struct sockaddr_un server_addr;
   memset(&server_addr, 0, sizeof(struct sockaddr_un));
   server_addr.sun_family = PF_UNIX;
   strncpy(server_addr.sun_path, un_socket, sizeof(server_addr.sun_path));  
#else
   assert(inet_socket);
   result = initWindowsSocket();
   if (result == -1) {
	   fprintf(stderr, "Windows init failed\n");
	   return -1;
   }
   
   sockfd = socket(PF_INET, SOCK_STREAM, 0);
   if (sockfd == INVALID_SOCKET || sockfd == SOCKET_ERROR) {
	  fprintf(stderr, "New socket failed\n");
      return -1;
   }

   hostptr = gethostbyname("localhost");
   inadr = (struct in_addr *) ((void*) hostptr->h_addr_list[0]);
  
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = PF_INET;
   server_addr.sin_port = atoi(inet_socket);
   server_addr.sin_addr = *inadr;


   /*
   struct sockaddr_in {
        short   sin_family;
        u_short sin_port;
        struct  in_addr sin_addr;
        char    sin_zero[8];
};
*/   
#endif

   result = connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
   if (result != 0) {
      perror("Failed to connect to server");
      return -1;
   }
   
   return 0;
}

int send_message(unsigned char *msg, size_t msg_size)
{
   int result;
   result = send(sockfd, (char *) msg, msg_size, 0);
   if (result == -1) {
      perror("Mutatee unable to send message");
      return -1;
   }
   return 0;
}

int recv_message(unsigned char *msg, size_t msg_size)
{
   int result = -1;
   while( result != msg_size && result != 0 ) {
#if !defined(os_windows_test)
	   int options = MSG_WAITALL;
#else
	   int options = 0;
#endif
       result = recv(sockfd, (char *) msg, msg_size, options);

       if (result == -1 && errno != EINTR ) {
          perror("Mutatee unable to recieve message");
          return -1;
       }

#if defined(os_freebsd_test)
       /* Sometimes the recv system call is not restarted properly after a
        * signal and an iRPC. TODO a workaround for this bug
        */
       if( result && result != msg_size ) {
           logerror("Received message of unexpected size %d (expected %d)\n",
                   result, msg_size);
       }
#endif
   }
   return 0;
}

