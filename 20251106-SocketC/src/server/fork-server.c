#include  "../unp.h"
#include  <time.h>

// This is an example of TCP/TP protocol concurrent server for text read/write.
// For each client, fork spawns a child, and the child handles the new client.
// The child closes the listening socket and the parent closes the connected socket.
// Child call str_echo to handle client.
// IPC is a technique for passing information between the parent and the child after fork
// Because the child inherits a copy of the parent’s memory and descriptors, any information that existed before the fork is automatically available
//    int value = 42;
//    pid_t pid = fork();
//    if (pid == 0)
//        printf("child got value = %d\n", value);

//    int fd = open("data.txt", O_RDONLY);
//    pid_t pid = fork();
//    if (pid == 0)
//        read(fd, buf, n);  // works — same open file

// clone() is used to implement pthread_create
// Using these system calls, the caller can control whether or not the two
// processes share the virtual address space, the table of file descriptors, and the table of signal handlers.
//
//
// So when developing a server, that means it intends to run it like a deamon process on the background, no screen, no UI, no terminal.
// For that reason we prefer using syslog over printf

// More info about DNS refer to Chapter 14 of TCPv1 and [Albitz and Liu 2001]
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

        int                             listenfd, connfd;
        socklen_t                       len; //int
        struct sockaddr_in              servaddr, cliaddr;
        char				buff[MAXLINE];
        time_t				ticks;
        pid_t                           childpid;

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

        //
        // 20251411
        // Signals provide a way of handling asynchronous events
        // Each signal has a default action or run a custom handler if user set
        // SIGSEGV              Segmentation fault          Terminal + core dump
        // SIGCONT              Continue process            Continue process
        // SIGCHLD              Child exited                Ignored by default
        //
        // The kill function sends a signal to a process or a group of processes.
        // The pause function suspends the calling process until a signal is caught.
        // The alarm function allows us to set a timer that will expire at a specified time in the future.
        // When the timer expires, the SIGALRM signal is generated.
        //
        // The raise function allows a process to send a signal to itself.
        // Kernel interrupts the system call (accept(), read(), sleep()) to run the signal handler function
        // When the handler finishes, the kernel has two choices:
        // Restart          |	Automatically restart the system call after the handler returns
        // Don’t restart    |	Return from the system call immediately with -1 and set errno = EINTR (“interrupted system call”)
        // So we register the child process termination SIGCHLD handler to the kernel
        Signal(SIGCHLD, sig_chld);

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
                connfd = Accept(listenfd, (SA *) &cliaddr, &len);

                // Convert it to humanreadble ip address string from sockaddr_in data type
                char client_ip[INET_ADDRSTRLEN];  // 16 bytes for IPv4 dotted string
                inet_ntop(AF_INET, &cliaddr.sin_addr, client_ip, sizeof(client_ip));

                syslog(LOG_INFO, "Client address: %s\n", client_ip);
                syslog(LOG_INFO, "Accepted connection from %s:%d (child pid %d)\n",
                       client_ip, ntohs(cliaddr.sin_port), getpid());

                // 20251411
                // fork() creates a new process by duplicating the entire execution context of the parent.
                // Child resumes the same instruction point by duplicating the parent stack, registers including program counter and stack pointer
                if ((childpid = Fork()) == 0) { /* child process */
                    Close(listenfd); /* close listening socket */
                    str_echo(connfd); /* process the request */
                    exit (0);
                }
                    Close(connfd); /* parent closes connected socket */
                }
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
