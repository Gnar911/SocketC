#include  "../unp.h"
#include  <time.h>

//This is an example of TCP/TP protocol
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

        int					listenfd, connfd;
        socklen_t  len; //int
        struct sockaddr_in	servaddr, cliaddr;
        char				buff[MAXLINE];
        time_t				ticks;

        // The socket function is the equivalent of having a telephone to use.
        // Bind is telling other people your telephone number so that they can call you.
        // Listen is turning on the
        listenfd = Socket(AF_INET, SOCK_STREAM, 0);

        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family      = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

        // This is the service port number
        servaddr.sin_port        = htons(13);	/* daytime server */
        Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

        // It specifies the maximum number of client
        // connections that the kernel will queue for this listening descriptor.
        Listen(listenfd, LISTENQ);

        // the server process is put to sleep in the call to accept, waiting for a client
        // connection to arrive and be accepted.
        for ( ; ; ) {
                len = sizeof(cliaddr);
                // This is await funtion wait for the 3 ways handshake established completes -> return
                // If multiple client connections arrive at the same time, the kernel
                // queues them up and return to connfd once at a time
                ///connfd = Accept(listenfd, (SA *) NULL, NULL);

                // Get the client address and port
                connfd = Accept(listenfd, (SA *) &cliaddr, &len);

        // IPv4 dependent, sock_ntop independent
        printf("connection from %s, port %d\n",
        Inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff)),
        ntohs(cliaddr.sin_port));

        ticks = time(NULL);

        // Append to string by snprintf
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));

        //the result is written to the client by write.
        //Write(connfd, buff, strlen(buff));
        Send(connfd, buff, strlen(buff), 0);

        //While it takes three segments to establish a connection, it takes four to terminate a connection
        // Often the client performs the active close, but with
        // some protocols (notably HTTP/1.0), the server performs the active close.
        Close(connfd);
        }
}
