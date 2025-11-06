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
        sockfd = Socket(AF_INET, SOCK_STREAM, 0);
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port   = htons(SERV_PORT);	/* daytime server */
        Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

        // IP -> Host name

        Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));
        str_cli(stdin, sockfd);
        exit(0);
}

void
str_cli(FILE *fp, int sockfd)
{
    char	sendline[MAXLINE], recvline[MAXLINE];

    while (Fgets(sendline, MAXLINE, fp) != NULL) {

        Send(sockfd, sendline, strlen(sendline), 0);

        if (Readline(sockfd, recvline, MAXLINE) == 0)
            err_quit("str_cli: server terminated prematurely");

        Fputs(recvline, stdout);
    }
}

