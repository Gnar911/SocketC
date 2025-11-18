#include <stdio.h>
#include "../unp.h"
#include "../thread/unpthread.h"

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
        // The numeric address can change but the name can remain the same
        // /etc/hosts = hard-code hostnames to IPs before DNS server

        Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));
        str_cli(stdin, sockfd);
        exit(0);
}


// Thread version
void	*copyto(void *);

static int	sockfd;		/* global for both threads to access */
static FILE	*fp;

void
str_cli(FILE *fp_arg, int sockfd_arg)
{
    char		recvline[MAXLINE];
    pthread_t	tid;

    sockfd = sockfd_arg;	/* copy arguments to externals */
    fp = fp_arg;

    // The kernel will assign the tid for us
    Pthread_create(&tid, NULL, copyto, NULL);

    while (Readline(sockfd, recvline, MAXLINE) > 0)
        Fputs(recvline, stdout);
}

void *
copyto(void *arg)
{
    char	sendline[MAXLINE];

    while (Fgets(sendline, MAXLINE, fp) != NULL)
        Writen(sockfd, sendline, strlen(sendline));

    Shutdown(sockfd, SHUT_WR);	/* EOF on stdin, send FIN */

    return(NULL);
        /* 4return (i.e., thread terminates) when EOF on stdin */
}

// Blocking version
//void
//str_cli(FILE *fp, int sockfd)
//{
//    char	sendline[MAXLINE], recvline[MAXLINE];

//    // Fgets always returns NULL if user does not type anything yet -> so we put it in while loop to call it
//    // continously until it return the full buffer data that user typed in
//    // It internally calls the read to read the data from buffer of a specified file descriptor e.g. stdin or socket into user-space buffer.
//    // FILE *fp = stdin;
//    // int fd = fileno(fp);      // returns 0
//    // read(fd, buf, size);
//    //
//    // FILE *fp;
//    // fp = fopen("input.txt", "r");
//    // Fgets(line, sizeof(line), fp)
//    while (Fgets(sendline, MAXLINE, fp) != NULL) {
//        Send(sockfd, sendline, strlen(sendline), 0);

//        if (Readline(sockfd, recvline, MAXLINE) == 0)
//            err_quit("str_cli: server terminated prematurely");

//        Fputs(recvline, stdout);
//    }
//}


// Multiplexing version to handle error/close event from server while client process typing
// select, pselect, poll
//void str_cli(FILE *fp, int sockfd)
//{
//int maxfdp1;
//fd_set rset;
//char sendline[MAXLINE], recvline[MAXLINE];
//FD_ZERO(&rset);
//for ( ; ; ) {
//    // Multiplexing 2 file descriptors, socket and stdin I/O
//    // By using the file descriptor set fd_set data type
//    FD_SET(fileno(fp), &rset);
//    FD_SET(sockfd, &rset);
//    maxfdp1 = max(fileno(fp), sockfd) + 1;

//    // This is multiplexing I/O model, it returns if one of the file descriptor (stdin or socket descriptor) are ready, has the data to read
//    Select(maxfdp1, &rset, NULL, NULL, NULL);
//    // Select returned, one of those file desctiptor is ready (maybe socket send the FIN or maybe user done typing)
//    //10112025
//    // So the I/O we gonna read here is in condition of nonblocking anymore, the buffer has data to read
//    // Although it blocks small amount of time to copy the data from kernel to our recvline
//     if (FD_ISSET(sockfd, &rset)) { /* socket is readable */
//        if (Readline(sockfd, recvline, MAXLINE) == 0) err_quit("str_cli: server terminated prematurely");
//        Fputs(recvline, stdout);
//    }

//    // The Fgets is sure to be non blocking but write can still block if the buffer is full
//    if (FD_ISSET(fileno(fp), &rset)) { /* input is readable */
//        if (Fgets(sendline, MAXLINE, fp) == NULL) return; /* all done */
//            Writen(sockfd, sendline, strlen(sendline));
//        }
//    }
//}

// Non-blocking I/O Model
//void
//str_cli(FILE *fp, int sockfd)
//{
//    int			maxfdp1, val, stdineof;
//    ssize_t		n, nwritten;
//    fd_set		rset, wset;
//    char		to[MAXLINE], fr[MAXLINE];
//    char		*toiptr, *tooptr, *friptr, *froptr;

//    // First we tell the kernel that these file descriptors force it to return immediately instead with an error (EWOULDBLOCK / EAGAIN)
//    //read() returns -1 and sets errno = EWOULDBLOCK if no data is available.
//    //write() returns -1 and sets errno = EWOULDBLOCK if the socket buffer is full.
//    val = Fcntl(sockfd, F_GETFL, 0);
//    Fcntl(sockfd, F_SETFL, val | O_NONBLOCK);

//    val = Fcntl(STDIN_FILENO, F_GETFL, 0);
//    Fcntl(STDIN_FILENO, F_SETFL, val | O_NONBLOCK);

//    val = Fcntl(STDOUT_FILENO, F_GETFL, 0);
//    Fcntl(STDOUT_FILENO, F_SETFL, val | O_NONBLOCK);

//    toiptr = tooptr = to;	/* initialize buffer pointers */
//    friptr = froptr = fr;
//    stdineof = 0;

//    maxfdp1 = max(max(STDIN_FILENO, STDOUT_FILENO), sockfd) + 1;

//    // We loop through all the non-blocking read/write without waiting for it
//    // So the typing never have to wait for write to be buffer ready
//    for ( ; ; ) {
//        FD_ZERO(&rset);
//        FD_ZERO(&wset);
//        if (stdineof == 0 && toiptr < &to[MAXLINE])
//            FD_SET(STDIN_FILENO, &rset);	/* read from stdin */
//        if (friptr < &fr[MAXLINE])
//            FD_SET(sockfd, &rset);			/* read from socket */
//        if (tooptr != toiptr)
//            FD_SET(sockfd, &wset);			/* data to write to socket */
//        if (froptr != friptr)
//            FD_SET(STDOUT_FILENO, &wset);	/* data to write to stdout */

//        Select(maxfdp1, &rset, &wset, NULL, NULL);
///* end nonb1 */
///* include nonb2 */
//        if (FD_ISSET(STDIN_FILENO, &rset)) {
//            if ( (n = read(STDIN_FILENO, toiptr, &to[MAXLINE] - toiptr)) < 0) {
//                if (errno != EWOULDBLOCK)
//                    err_sys("read error on stdin");

//            } else if (n == 0) {
//#ifdef	VOL2
//                fprintf(stderr, "%s: EOF on stdin\n", gf_time());
//#endif
//                stdineof = 1;			/* all done with stdin */
//                if (tooptr == toiptr)
//                    Shutdown(sockfd, SHUT_WR);/* send FIN */

//            } else {
//#ifdef	VOL2
//                fprintf(stderr, "%s: read %d bytes from stdin\n", gf_time(), n);
//#endif
//                toiptr += n;			/* # just read */
//                FD_SET(sockfd, &wset);	/* try and write to socket below */
//            }
//        }

//        if (FD_ISSET(sockfd, &rset)) {
//            if ( (n = read(sockfd, friptr, &fr[MAXLINE] - friptr)) < 0) {
//                if (errno != EWOULDBLOCK)
//                    err_sys("read error on socket");

//            } else if (n == 0) {
//#ifdef	VOL2
//                fprintf(stderr, "%s: EOF on socket\n", gf_time());
//#endif
//                if (stdineof)
//                    return;		/* normal termination */
//                else
//                    err_quit("str_cli: server terminated prematurely");

//            } else {
//#ifdef	VOL2
//                fprintf(stderr, "%s: read %d bytes from socket\n",
//                                gf_time(), n);
//#endif
//                friptr += n;		/* # just read */
//                FD_SET(STDOUT_FILENO, &wset);	/* try and write below */
//            }
//        }
///* end nonb2 */
///* include nonb3 */
//        if (FD_ISSET(STDOUT_FILENO, &wset) && ( (n = friptr - froptr) > 0)) {
//            if ( (nwritten = write(STDOUT_FILENO, froptr, n)) < 0) {
//                if (errno != EWOULDBLOCK)
//                    err_sys("write error to stdout");

//            } else {
//#ifdef	VOL2
//                fprintf(stderr, "%s: wrote %d bytes to stdout\n",
//                                gf_time(), nwritten);
//#endif
//                froptr += nwritten;		/* # just written */
//                if (froptr == friptr)
//                    froptr = friptr = fr;	/* back to beginning of buffer */
//            }
//        }

//        if (FD_ISSET(sockfd, &wset) && ( (n = toiptr - tooptr) > 0)) {
//            if ( (nwritten = write(sockfd, tooptr, n)) < 0) {
//                if (errno != EWOULDBLOCK)
//                    err_sys("write error to socket");

//            } else {
//#ifdef	VOL2
//                fprintf(stderr, "%s: wrote %d bytes to socket\n",
//                                gf_time(), nwritten);
//#endif
//                tooptr += nwritten;	/* # just written */
//                if (tooptr == toiptr) {
//                    toiptr = tooptr = to;	/* back to beginning of buffer */
//                    if (stdineof)
//                        Shutdown(sockfd, SHUT_WR);	/* send FIN */
//                }
//            }
//        }
//    }
//}
///* end nonb3 */

//// A simpler version of non-blocking I/O Model using fork
//// Each process can block independently on its own stream,
//// so pair handles full-duplex I/O without O_NONBLOCK or select()
//void
//str_cli(FILE *fp, int sockfd)
//{
//    pid_t	pid;
//    char	sendline[MAXLINE], recvline[MAXLINE];

//    if ( (pid = Fork()) == 0) {		/* child: server -> stdout */
//        // Listen to server process
//        while (Readline(sockfd, recvline, MAXLINE) > 0)
//            Fputs(recvline, stdout);

//        // The server sends FIN, Readline return < 0, terminate parent
//        // OS will force terminate parent even user is typing by sending a signal SIGTERM to parent process
//        kill(getppid(), SIGTERM);	/* in case parent still running */
//        exit(0);
//    }

//        /* parent: stdin -> server */
//    // Listen to user event process
//    // So pressing Enter just adds a newline \n, not an EOF or a FIN, so it not return NULL
//    while (Fgets(sendline, MAXLINE, fp) != NULL)
//        Writen(sockfd, sendline, strlen(sendline));

//    /* EOF on stdin, send FIN to the server and wait for the child process receive FIN and shutdown me*/
//    Shutdown(sockfd, SHUT_WR);
//    pause();
//    return;
//}





