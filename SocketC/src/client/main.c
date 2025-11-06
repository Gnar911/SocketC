#include <stdio.h>
#include "../unp.h"
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

        int					sockfd, n;
        char				recvline[MAXLINE + 1];
        if (argc != 2)
                err_quit("usage: a.out <IPaddress>");


        struct sockaddr_in	servaddr;
        if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                err_sys("socket error");


        // Most existing IPv4 code uses inet_addr and inet_ntoa,
        // but two new functions, inet_pton and inet_ntop, handle both
        // IPv4 and IPv6.
        bzero(&servaddr, sizeof(servaddr));
        //IPv4
        servaddr.sin_family = AF_INET;
        servaddr.sin_port   = htons(13);	/* daytime server */
        if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
                err_quit("inet_pton error for %s", argv[1]);
        //IPv6
        // struct sockaddr_in6 servaddr6;
        // if ( (sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
        // servaddr6.sin6_family = AF_INET6;
        // servaddr6.sin6_port   = htons(13);	/* daytime server */
        // if (inet_pton(AF_INET6, argv[1], &servaddr6.sin6_addr) <= 0)
        //         err_quit("inet_pton error for %s", argv[1]);




        // IP -> Host name



          // This causes the client TCP to send a "synchronize" (SYN) segment
          // it just contains an IP header, a TCP header, and possible TCP options

          // sockaddr (SA) here is the generic pointer type.
          // Any function call must cast the pointer to generic sockaddr. (struct sockaddr_in serv;)
          // If that memory doesn’t actually contain a valid sa_family, the kernel misreads random bytes — undefined results.
          // Since the pointer inside connect function does not know about what actual data type you pass in, so it need the size of you data type
          // The client does not need to specify the Ip address and the portm the kernel will do that
          if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
              // This is an abstract error handling function
              err_sys("connect error");

        //Normally, a single segment containing all 26 bytes of data
        //is returned, but with larger data sizes, we cannot assume that the server's reply will be
        // returned by a single read.
        // we always need to code
        // the read in a loop and terminate the loop when either read returns 0
        //“TCP does not provide message boundaries — it only transports bytes reliably.”
        //So your application layer must define where messages start and end.

        // read, write is only used for file descriptors in UNIX. In Window -> objects
        //while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
        while ( (n = recv(sockfd, recvline, MAXLINE, 0)) > 0) {
                recvline[n] = 0;	/* null terminate */
                if (fputs(recvline, stdout) == EOF)
                        err_sys("fputs error");
        }
        //if (n < 0)
        if (ISSOCKETERROR(n))
                err_sys("read error");

        exit(0);
}
