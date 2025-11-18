#include  "../unp.h"
#include  <time.h>
#include "../thread/unpthread.h"

// This is an example of TCP/TP transport layer protocol concurrent server for text read/write.
// One thread per client version
// So when developing a server, that means it intends to run it like a deamon process on the background, no screen, no UI, no terminal.
// For that reason we prefer using syslog over printf
// More info about DNS refer to Chapter 14 of TCPv1 and [Albitz and Liu 2001]

//
//
//
//
//
//
// Threads
// A stack is simply a contiguous region of memory
// Each thread has its own stack, they all share the same heap, global variables, file descriptors, etc.
// pthread_create(...);  -> OS allocates a new stack
// Even when 2 threads run on a single CPU, the race condition still happens because the CPU is switching the instruction even before it writes back to the memory
//
// Each thread within a process is identified by a thread ID, whose datatype is pthread_t
// Specify the thread attribute like: its priority, its internal stack size, deamon ? by specify pthread_attr_t variable to overrides the default (Null pointer)
// The thread starts by calling function and then terminates either explicitly (by calling pthread_exit) or implicitly (by letting the function return).
// Return 0 or successful and > 0 for error and do not set errno
// Comparing threads to Unix processes, pthread_create is similar to fork, and pthread_join is similar to waitpid, we must specify the TID of the thread we want to wait for.
// pthread_self is similar to getpid, return tid with datatype pthread_t
// pthread_detach like deamon process, will not know the status of other joinable threads if they have terminated or not
// Threads will be terminated if function return void/ main process returns or any threads exit -> terminated main process and all threads
//
//

static void *doit(void *arg);

int
main(int argc, char **argv)
{
  openlog("ServerConcur", LOG_PID | LOG_PERROR, LOG_USER); // development only
  syslog(LOG_INFO, "Welcome");
  // On Windows, before you can use any Winsock function (socket(), bind(), connect(), etc.),
  // you must first initialize the Winsock library with a call to WSAStartup().
  #if defined(_WIN32)
  WSADATA d;
  if (WSAStartup(MAKEWORD(2, 2), &d)) {
  fprintf(stderr, "Failed to initialize.\n");
  return 1;
  }
  #endif

        int                             listenfd, connfd, *iptr;
        socklen_t                       len; //int
        struct sockaddr_in              servaddr, cliaddr;
        char				buff[MAXLINE];
        time_t				ticks;
        pid_t               childpid;
        pthread_t           tid;


        // The socket function is the equivalent of having a telephone to use.
        // Bind is telling other people your telephone number so that they can call you.
        // Listen is turning on the
        listenfd = Socket(AF_INET, SOCK_STREAM, 0);

        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family      = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

        // This is the service port number
        servaddr.sin_port        = htons(SERV_PORT);
        Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

        Getsockname(listenfd, (struct sockaddr *)&servaddr, &len);
        syslog(LOG_INFO, "Server PID: %d\n", getpid());
        syslog(LOG_INFO, "Listening on %s:%d\n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));

        // It specifies the maximum number of client
        // connections that the kernel will queue for this listening descriptor.
        Listen(listenfd, LISTENQ);

        // the server process is put to sleep in the call to accept, waiting for a client
        // connection to arrive and be accepted.
        for ( ; ; ) {
                // We know the data type of it so we know the size we want to get, then pass that size to the function
                len = sizeof(cliaddr);
                // This is await funtion wait for the 3 ways handshake established completes -> return
                // If multiple client connections arrive at the same time, the kernel
                // queues them up and return to connfd once at a time
                ///connfd = Accept(listenfd, (SA *) NULL, NULL);

                // Get the client address (sockaddr_in data type) and port
                // Because the for loop in main, the connfd is overriden again and again
                connfd = Accept(listenfd, (SA *) &cliaddr, &len);

                // CLIENT INFO
                // Convert it to humanreadble ip address string from sockaddr_in data type
                char client_ip[INET_ADDRSTRLEN];  // 16 bytes for IPv4 dotted string
                inet_ntop(AF_INET, &cliaddr.sin_addr, client_ip, sizeof(client_ip));

                syslog(LOG_INFO, "Client address: %s\n", client_ip);
                syslog(LOG_INFO, "Accepted connection from %s:%d (child pid %d)\n",
                       client_ip, ntohs(cliaddr.sin_port), getpid());


                // CREATE THREADS FOR CLIENT
                // All threads within a process share the descriptors, so if the main thread close connected socket -> terminate connection
                // Using default pthread attrubute
                // connfd is local variable here so we need to stat it into heap or

                // call malloc and allocate space for an integer variable, the connected descriptor.
                iptr = Malloc(sizeof(int));
                *iptr = connfd;
                Pthread_create(&tid, NULL, &doit, iptr);

                // The child gets a copy of the parent’s file descriptor table points to the same open file object in the kernel.
                //                Parent process          Child process
                //                 └─ FD table:            └─ FD table:
                //                     fd=3  ─────┐────────┘fd=3
                //                                │
                //                Kernel open file table:
                //                 └─ [open_file#42] → inode("notes.txt"), offset=0, refcount=2
//                offset = 0 → you’re at the beginning of the file
//                offset = 100 → your next read/write starts at byte #100 in that file
                // write(fd, "AB", 2); so if fd is shared, then the offset will incremented
                // With fork process, since connected socket is copied from parent, so dont need to close it before the fork.
//                if ((childpid = Fork()) == 0) { /* child process */
//                    Close(listenfd); /* close listening socket */
//                    str_echo(connfd); /* process the request */
//                    exit (0);
//                }
                    //Close(connfd); /* parent closes connected socket */
                }
}

static void *
doit(void *arg)
{
    // Detach the thread since there is no reason for the main thread to wait for each thread it creates
    Pthread_detach(pthread_self());
    str_echo((int) arg);	/* same function as before */
    Close((int) arg);		/* done with connected socket */
    return(NULL);
}

void str_echo(int sockfd) {
    char buf[1024];

    while (1) {
        // We want to handle error ourselve for specific system so we dont use the wrap function Recv
        // 11092025
        // So we have recevfrom which is blocking IO which block our process,
        // it only returns when datagram arrives and is copied into our application or an error occurs.
        // Asynchronous I/O model: data is completed and ready to use on your buffer
        // The signal-driven I/O model: I/O is ready -> block all process to call handle the I/O in the process
        // Synchronous I/O: Blocking, Non-blocking, Multiplexing, Signal driven I/O Model. The actual I/O operation (recvfrom) blocks the process.
        ssize_t n = recv(sockfd, buf, sizeof(buf), 0); //Recv

        if (n > 0){
            // We does care about the error of send so we let the wrap function to handle that
            Send(sockfd, buf, n, 0);
        }
        else if (n == 0){
            break;  // client closed connection
        }
        else if (n < 0 && errno == EINTR) {
            continue; // interrupted, retry
        }
        else{
            err_sys("read error");
            break;
        }
    }
}

void
sig_chld(int signo)
{
    pid_t pid;
    int stat;
    // So the loop continues until waitpid() returns 0 → meaning no more zombies remain.
    // That why we use while instead of if 1 time
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        syslog(LOG_INFO, "child %d terminated\n", pid);
    return;
}


// descriptors 0, 1, and 2 are set to standard input,
// output, and error so the first available descriptor for the listening socket is 3.
