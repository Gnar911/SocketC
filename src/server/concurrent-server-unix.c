#include  "../unp.h"
#include  <time.h>

// This is an example of TCP/TP protocol concurrent server for text read/write.
// For each client, fork spawns a child, and the child handles the new client.
// The child closes the listening socket and the parent closes the connected socket.
// Child call str_echo to handle client.
int
main(int argc, char **argv)
{
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
        servaddr.sin_port        = htons(SERV_PORT);	/* daytime server */
        Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

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
                connfd = Accept(listenfd, (SA *) &cliaddr, &len);

                if ( (childpid = Fork()) == 0) { /* child process */
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
        ssize_t n = Recv(sockfd, buf, sizeof(buf), 0); //Recv
        if (n > 0){
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
