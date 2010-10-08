#include "ParameterDict.h"
#include "proccontrol_comp.h"
#include "communication.h"

#include <string>
#include <cstdio>
#include <cerrno>
#include <cstring>
#if defined(os_windows_test)
#include <windows.h>
#endif

TEST_DLL_EXPORT ComponentTester *componentTesterFactory()
{
   return (ComponentTester *) new ProcControlComponent();
}

ProcControlMutator::ProcControlMutator()
{
}

ProcControlMutator::~ProcControlMutator()
{
}

test_results_t ProcControlMutator::setup(ParameterDict &param)
{
   comp = (ProcControlComponent *) param["ProcControlComponent"]->getPtr();
   return PASSED;
}

ProcControlComponent::ProcControlComponent() :
   sockfd(0),
   sockname(NULL),
   notification_fd(-1)
   
{
   notification_fd = evNotify()->getFD();
}

static ProcControlComponent *pccomp = NULL;
static Process::cb_ret_t eventCounterFunction(Event::const_ptr ev)
{
   pccomp->eventsRecieved[ev->getEventType()].push_back(ev);
   return Process::cbDefault;
}

bool ProcControlComponent::registerEventCounter(EventType et)
{
   pccomp = this;
   Process::registerEventCallback(et, eventCounterFunction);
   return true;
}

bool ProcControlComponent::checkThread(const Thread &thread)
{
   return true;
}

#define MAX_ARGS 128
Process::ptr ProcControlComponent::launchMutatee(RunGroup *group, ParameterDict &params)
{
   char *logfilename = params["logfilename"]->getString();
   char *humanlogname = params["humanlogname"]->getString();
   bool verboseFormat = (bool) params["verbose"]->getInt();
   char thread_num_str[128];

   const char *args[MAX_ARGS];
   unsigned n=0;
   args[n++] = group->mutatee;
   if (logfilename) {
      args[n++] = "-log";
      args[n++] = logfilename;
   }
   if (humanlogname) {
      args[n++] = "-humanlog";
      args[n++] = humanlogname;
   }
   if (!verboseFormat) {
      args[n++] = "-q";
   }
#if !defined(os_windows)
   args[n++] = "-un_socket";
#else
   args[n++] = "-inet_socket";
#endif
   args[n++] = sockname;

   if (group->threadmode == SingleThreaded) {
      args[n++] = "-st";
   }
   else if (group->threadmode == MultiThreaded) {
      args[n++] = "-mt";
      snprintf(thread_num_str, 128, "%d", DEFAULT_NUM_THREADS);
      args[n++] = thread_num_str;
   }
   if (group->procmode == SingleProcess) {
      args[n++] = "-sp";
   }
   else if (group->procmode == MultiProcess) {
      args[n++] = "-mp";
   }

   bool printed_run = false;
   for (std::vector<TestInfo *>::iterator i = group->tests.begin(); i != group->tests.end(); i++)
   {
      if (shouldRunTest(group, *i)) {
         if (!printed_run) {
            args[n++] = "-run";
            printed_run = true;
         }
         args[n++] = (*i)->name;
      }
   }
   args[n] = NULL;
   assert(n < MAX_ARGS-1);

   Process::ptr proc = Process::ptr();
   if (group->useAttach == CREATE) {
      std::vector<std::string> vargs;
      for (unsigned i=0; i<n; i++) {
         vargs.push_back(std::string(args[i]));
      }
      proc = Process::createProcess(std::string(group->mutatee), vargs);
      if (!proc) {
         logerror("Failed to execute new mutatee\n");
         return Process::ptr();
      }
   }
   else if (group->useAttach == USEATTACH) {
	  Dyninst::PID pid = 0;
#if !defined(os_windows_test)
      pid = fork_mutatee();
      if (!pid) {
         //Child
         execv(group->mutatee, (char * const *)args);
         char buffer[2048];
         snprintf(buffer, 2048, "execv for attach failed on %s: %s\n", 
                  group->mutatee, 
                  strerror(errno));
         logerror(buffer);
         exit(-1);
      }
#else
	  STARTUPINFO stinfo;
	  PROCESS_INFORMATION procinfo;
      memset(&stinfo, 0, sizeof(STARTUPINFO));
      stinfo.cb = sizeof(STARTUPINFO);
	  std::string arg_string;
	  for (unsigned i=0; args[i]; i++) {
		  arg_string += args[i];
		  arg_string += " ";
	  }
      BOOL result = CreateProcess(group->mutatee, const_cast<char *>(arg_string.c_str()),
		                        NULL, NULL, FALSE, 0,
		                        NULL, NULL, &stinfo, &procinfo);	 
	  pid = procinfo.dwProcessId;
#endif
      int sockfd;
      bool result_b = acceptConnections(1, &sockfd);
      if (!result_b) {
         logerror("Unable to accept attach connection\n");
         return Process::ptr();
      }
      
      proc = Process::attachProcess(pid, group->mutatee);
      if (!proc) {
         logerror("Failed to attach to new mutatee\n");
         return Process::ptr();
      }
      process_socks[proc] = sockfd;
   }
   else {
      return Process::ptr();
   }

   assert(proc);
   Dyninst::PID pid = proc->getPid();
   process_pids[pid] = proc;
   procs.push_back(proc);
   return proc;
}

bool ProcControlComponent::launchMutatees(RunGroup *group, ParameterDict &param)
{
   bool error = false;
   bool result = setupServerSocket();
   if (!result) {
      logerror("Failed to setup server side socket\n");
      return FAILED;
   }
   
   num_processes = 0;
   if (group->procmode == MultiProcess)
      num_processes = NUM_PARALLEL_PROCS;
   else
      num_processes = 1;
   
   for (unsigned i=0; i<num_processes; i++) {
      Process::ptr proc = launchMutatee(group, param);
      if (proc == NULL) {
         error = true;
         continue;
      }
   }

   EventType thread_create(EventType::None, EventType::ThreadCreate);
   registerEventCounter(thread_create);

   num_threads = group->threadmode == MultiThreaded ? DEFAULT_NUM_THREADS : 0;
   if (group->useAttach == CREATE)
   {
      int num_procs = 0;
      for (std::vector<Process::ptr>::iterator j = procs.begin(); j != procs.end(); j++) {
         bool result = (*j)->continueProc();
         num_procs++;
         if (!result) {
            error = true;
            continue;
         }
      }
   
      while (eventsRecieved[thread_create].size() < num_procs*num_threads) {
         bool result = Process::handleEvents(true);
         if (!result) {
            logerror("Failed to handle events during thread create\n");
            error = true;
         }
      }

      result = acceptConnections(num_procs, NULL);
      if (!result) {
         logerror("Failed to accept connections from new mutatees\n");
         error = true;
      }

      if (group->state == STOPPED) {
         std::map<Process::ptr, int>::iterator i;
         for (i = process_socks.begin(); i != process_socks.end(); i++) {
            bool result = i->first->stopProc();
            if (!result) {
               logerror("Failed to stop process\n");
               error = true;
            }
         }
      }
   }
   else if (group->useAttach == USEATTACH)
   {
      for (std::vector<Process::ptr>::iterator j = procs.begin(); j != procs.end(); j++) {
         Process::ptr proc = *j;
         if (proc->threads().size() != num_threads+1) {
            logerror("Process has incorrect number of threads");
            error = true;
         }
      }
      if (eventsRecieved[thread_create].size()) {
         logerror("Recieved unexpected thread creation events on process\n");
         error = true;
      }

      if (group->state == RUNNING) {
         std::map<Process::ptr, int>::iterator i;
         for (i = process_socks.begin(); i != process_socks.end(); i++) {
            bool result = i->first->continueProc();
            if (!result) {
               logerror("Failed to continue process");
               error = true;
            }
         }
      }
   }

   result = cleanSocket();
   if (!result) {
      logerror("Failed to clean up socket\n");
      error = true;
   }

   handshake shake;
   shake.code = HANDSHAKE_CODE;
   result = send_broadcast((unsigned char *) &shake, sizeof(handshake));
   if (!result) {
      logerror("Failed to send handshake message to processes\n");
      error = true;
   }

   return !error;
}

test_results_t ProcControlComponent::program_setup(ParameterDict &params)
{
   return PASSED;
}

test_results_t ProcControlComponent::program_teardown(ParameterDict &params)
{
   return PASSED;
}

test_results_t ProcControlComponent::group_setup(RunGroup *group, ParameterDict &params)
{
   process_socks.clear();
   process_pids.clear();
   procs.clear();
   eventsRecieved.clear();
   sockfd = 0;
   sockname = NULL;
   curgroup_self_cleaning = false;

   me.setPtr(this);
   params["ProcControlComponent"] = &me;

   bool result = launchMutatees(group, params);
   if (!result) {
      logerror("Failed to launch mutatees\n");
      return FAILED;
   }

   return PASSED;
}

Process::cb_ret_t on_exit(Event::const_ptr ev)
{
   return Process::cbDefault;
}

test_results_t ProcControlComponent::group_teardown(RunGroup *group, ParameterDict &params)
{
   bool error = false;
   bool hasRunningProcs;

   if (curgroup_self_cleaning)
      return PASSED;

   Process::registerEventCallback(EventType(EventType::Exit), on_exit);
   do {
      hasRunningProcs = false;
      for (std::vector<Process::ptr>::iterator i = procs.begin(); i != procs.end(); i++) {
         Process::ptr p = *i;
         if (!p->isTerminated()) {
            bool result = block_for_events();
            if (!result) {
               logerror("Process failed to handle events\n");
               return FAILED;
            }
            if (!p->isTerminated()) {
               hasRunningProcs = true;
            }
         }
      }
   } while(hasRunningProcs);

   for (std::vector<Process::ptr>::iterator i = procs.begin(); i != procs.end(); i++) {
      Process::ptr p = *i;
      if (!p->isTerminated()) {
         logerror("Process did not terminate\n");
         error = true;
         continue;
      }
      if (p->isCrashed()) {
         logerror("Process terminated on crash\n");
         error = true;
         continue;
      }
      if (!p->isExited()) {
         logerror("Process did not report as exited\n");
         error = true;
         continue;
      }
      if (p->getExitCode() != 0) {
         logerror("Process has unexpected error code\n");
         error = true;
         continue;
      }
   }
   procs.clear();

   return error ? FAILED : PASSED;
}

test_results_t ProcControlComponent::test_setup(TestInfo *test, ParameterDict &parms)
{
   return PASSED;
}

test_results_t ProcControlComponent::test_teardown(TestInfo *test, ParameterDict &parms)
{
   return PASSED;
}

std::string ProcControlComponent::getLastErrorMsg()
{
   return std::string("");
}

ProcControlComponent::~ProcControlComponent()
{
}

#if !defined(os_windows_test)
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#else
#include <WinSock.h>
#endif

#if !defined(os_windows_test)

bool ProcControlComponent::setupServerSocket()
{
   int fd = socket(AF_UNIX, SOCK_STREAM, 0);
   if (fd == -1) {
      char error_str[1024];
      snprintf(error_str, 1024, "Unable to create socket: %s\n", strerror(errno));
      logerror(error_str);
      return false;
   }
   struct sockaddr_un addr;
   memset(&addr, 0, sizeof(struct sockaddr_un));
   addr.sun_family = AF_UNIX;
   snprintf(addr.sun_path, sizeof(addr.sun_path)-1, "/tmp/pct%d", getpid());
   
   int result = bind(fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un));
   if (result != 0){
      char error_str[1024];
      snprintf(error_str, 1024, "Unable to bind socket: %s\n", strerror(errno));
      logerror(error_str);
   }

   result = listen(fd, 512);
   if (result == -1) {
      char error_str[1024];
      snprintf(error_str, 1024, "Unable to listen on socket: %s\n", strerror(errno));
      logerror(error_str);
      return false;
   }

   sockfd = fd;   
   sockname = strdup(addr.sun_path);

   return true;
}

bool ProcControlComponent::cleanSocket()
{
   if (!sockname)
      return false;

   int result = unlink(sockname);
   if (result == -1) {
      logerror("Could not clean socket\n");
      return false;
   }
   free(sockname);
   sockname = NULL;
   result = close(sockfd);
   if (result == -1) {
      logerror("Could not close socket\n");
      return false;
   }
   return true;
}

#else

bool ProcControlComponent::setupServerSocket()
{
   WSADATA data;
   bool wsaok = false;

   // request WinSock 2.0
   if (WSAStartup(MAKEWORD(2,0), &data) == 0) {
      // verify that the version that was provided is one we can use
      if (LOBYTE(data.wVersion) != 2 || HIBYTE(data.wVersion) != 0)
      {
		 fprintf(stderr, "low = %d, high = %d\n", (int) LOBYTE(data.wVersion), (int) HIBYTE(data.wVersion));
		 logerror("Version error setting up socket\n");
         return false;
      }
   }

   //  set up socket to accept connections from mutatees (on demand)
   sockfd = socket(PF_INET, SOCK_STREAM, 0);

   if (sockfd == INVALID_SOCKET || sockfd == SOCKET_ERROR) {
	  logerror("new socket failed, sock = %d, lasterror = %d\n",
			   (unsigned int) sockfd, WSAGetLastError());
      return false;
   }

   struct sockaddr_in saddr;
   struct in_addr *inadr;
   struct hostent *hostptr;

   hostptr = gethostbyname("localhost");
   inadr = (struct in_addr *) ((void*) hostptr->h_addr_list[0]);
   memset((void*) &saddr, 0, sizeof(saddr));
   saddr.sin_family = PF_INET;
   saddr.sin_port = htons(0); // ask system to assign
   saddr.sin_addr = *inadr;
  
   int result = bind(sockfd, (struct sockaddr *) &saddr, sizeof(saddr));
   if (result == -1) { 
     logerror("bind socket failed\n");
	 return false;
   }

   //  get the port number that was assigned to us
   int length = sizeof(saddr);
   result = getsockname(sockfd, (struct sockaddr *) &saddr, &length);
   if (result == -1) 
   {
     logerror("getsockname failed\n");
	 return false;
   }
   int port = ntohs(saddr.sin_port);

  // set socket to listen for connections  
  // (we will explicitly accept in the main event loop)
  result = listen(sockfd, 32);
  if (result == -1) {
	  logerror("listen failed\n");
	  return false;
  }

  char sockname_local[32];
  snprintf(sockname_local, 32, "%d", port);
  sockname = strdup(sockname_local);

  return true;
}

bool ProcControlComponent::cleanSocket()
{
   sockname = NULL;
   int result = closesocket(sockfd);
   if (result == -1) {
      logerror("Could not close socket\n");
      return false;
   }
   sockfd = 0;
   return true;
}

#endif

bool ProcControlComponent::acceptConnections(int num, int *attach_sock)
{
   std::vector<int> socks;
   assert(num == 1 || !attach_sock);  //If attach_sock, then num == 1

   while (socks.size() < num) {
      fd_set readset; FD_ZERO(&readset);
      fd_set writeset; FD_ZERO(&writeset);
      fd_set exceptset; FD_ZERO(&exceptset);

      FD_SET(sockfd, &readset);
      FD_SET(notification_fd, &readset);
      int nfds = (sockfd > notification_fd ? sockfd : notification_fd)+1;
      
      struct timeval timeout;
      timeout.tv_sec = 30;
      timeout.tv_usec = 0;
      int result = select(nfds, &readset, &writeset, &exceptset, &timeout);
      if (result == 0) {
         logerror("Timeout while waiting for socket connect");
         return false;
      }
      if (result == -1) {
         perror("Error in select");
         return false;
      }

      if (FD_ISSET(sockfd, &readset))
      {
#if !defined(os_windows_test)
         struct sockaddr_un addr;
         socklen_t addr_size = sizeof(struct sockaddr_un);
#else
		 struct sockaddr_in addr;
		 int addr_size = sizeof(struct sockaddr_in);
#endif
         int newsock = accept(sockfd, (struct sockaddr *) &addr, &addr_size);
         if (newsock == -1) {
            char error_str[1024];
            snprintf(error_str, 1024, "Unable to accept socket: %s\n", strerror(errno));
            logerror(error_str);
            return false;
         }
         socks.push_back(newsock);
      }
      if (FD_ISSET(notification_fd, &readset)) {
         bool result = Process::handleEvents(true);
         if (!result) {
            logerror("Failed to handle process events\n");
            return false;
         }
      }
   }

   for (unsigned i=0; i<num; i++) {
      send_pid msg;
      bool result = recv_message((unsigned char *) &msg, sizeof(send_pid), socks[i]);
      if (!result) {
         logerror("Could not receive handshake pid\n");
         return false;
      }
      if (msg.code != SEND_PID_CODE)
      {
         logerror("Received bad code in handshake message\n");
         return false;
      }
      std::map<Dyninst::PID, Process::ptr>::iterator j = process_pids.find(msg.pid);
      if (j == process_pids.end()) {
         if (attach_sock) {
            *attach_sock = socks[i];
            return true;
         }
         logerror("Recieved unexpected PID in handshake message\n");
         return false;
      }
      process_socks[j->second] = socks[i];
   }

   return true;
}

bool ProcControlComponent::recv_message(unsigned char *msg, unsigned msg_size, int sfd)
{
   int result;
   for (;;) {
      int nfds = sfd > notification_fd ? sfd : notification_fd;
      nfds++;
      fd_set readset; FD_ZERO(&readset);
      fd_set writeset; FD_ZERO(&writeset);
      fd_set exceptset; FD_ZERO(&exceptset);
      FD_SET(sfd, &readset);
      FD_SET(notification_fd, &readset);
      struct timeval timeout;
      timeout.tv_sec = 15;
      timeout.tv_usec = 0;
      do {
         result = select(nfds, &readset, &writeset, &exceptset, &timeout);
      } while (result == -1 && errno == EINTR);
      
      if (result == 0) {
         logerror("Timeout while waiting for communication\n");
         return false;
      }
      if (result == -1) {
         char error_str[1024];
         snprintf(error_str, 1024, "Error calling select: %s\n", strerror(errno));
         logerror(error_str);
         return false;
      }
      
      if (FD_ISSET(notification_fd, &readset)) {
         bool result = Process::handleEvents(true);
         if (!result) {
            logerror("Failed to handle process events\n");
            return false;
         }
      }
      if (FD_ISSET(sfd, &readset)) {
         break;
      }
   } 

   int options = 0;
#if !defined(os_windows_test)
   options |= MSG_WAITALL;
#endif
   result = recv(sfd, (char *) msg, msg_size, options);
   if (result == -1) {
      char error_str[1024];
      snprintf(error_str, 1024, "Unable to recieve message: %s\n", strerror(errno));
      logerror(error_str);
      return false;
   }
   return true;
}

bool ProcControlComponent::send_message(unsigned char *msg, unsigned msg_size, int sfd)
{
   int options = 0;
#if !defined(os_windows_test)
   options |= MSG_NOSIGNAL
#endif
   int result = send(sfd, (char *) msg, msg_size, options);
   if (result == -1) {
      char error_str[1024];
      snprintf(error_str, 1024, "Mutator unable to send message: %s\n", strerror(errno));
      logerror(error_str);
      return false;
   }
   return true;
}

bool ProcControlComponent::block_for_events()
{
   int nfds = notification_fd+1;
   fd_set readset; FD_ZERO(&readset);
   fd_set writeset; FD_ZERO(&writeset);
   fd_set exceptset; FD_ZERO(&exceptset);
   FD_SET(notification_fd, &readset);

   struct timeval timeout;
   timeout.tv_sec = 15;
   timeout.tv_usec = 0;
   int result;
   do {
      result = select(nfds, &readset, &writeset, &exceptset, &timeout);
   } while (result == -1 && errno == EINTR);
   
   if (result == 0) {
      logerror("Timeout while waiting for event\n");
      return false;
   }
   if (result == -1) {
      char error_str[1024];
      snprintf(error_str, 1024, "Error calling select: %s\n", strerror(errno));
      logerror(error_str);
      return false;
   }
      
   assert(result == 1 && FD_ISSET(notification_fd, &readset));
   bool bresult = Process::handleEvents(true);
   if (!bresult) {
      logerror("Error waiting for events\n");
      return false;
   }
   return true;
}

bool ProcControlComponent::recv_message(unsigned char *msg, unsigned msg_size, Process::ptr p)
{
  return recv_message(msg, msg_size, process_socks[p]);
}

bool ProcControlComponent::send_message(unsigned char *msg, unsigned msg_size, Process::ptr p)
{
  return send_message(msg, msg_size, process_socks[p]);
}

bool ProcControlComponent::recv_broadcast(unsigned char *msg, unsigned msg_size)
{
   unsigned char *cur_pos = msg;
   for (std::map<Process::ptr, int>::iterator i = process_socks.begin(); i != process_socks.end(); i++) {
      bool result = recv_message(cur_pos, msg_size, i->second);
      if (!result) 
         return false;
      cur_pos += msg_size;
   }
   return true;
}

bool ProcControlComponent::send_broadcast(unsigned char *msg, unsigned msg_size)
{
   unsigned char *cur_pos = msg;
   for (std::map<Process::ptr, int>::iterator i = process_socks.begin(); i != process_socks.end(); i++) {
      bool result = send_message(msg, msg_size, i->second);
      if (!result) 
         return false;
   }
   return true;
}
