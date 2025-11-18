#include	"../thread/unpthread.h"
#include    "../unp.h"

// 11112025
// This is an example of TCP/IP Transport Layer Protocol using multiple threads and threads pool
// A function is thread-safe when:
// Remove all use of static/global variables
// Leave the static data, but guard access to it with a mutex (loses concurrency)
// One static key, separate copy of the static buffer
//
// In this example, the distribution will be kernel, kernel will schedule which thread in thread pool will handle the client
// Protect the call to accept from all the threads by mutex lock (at processes it is file locking)
//

typedef struct {
  pthread_t		thread_tid;		/* thread ID */

  // For tracking how many clients each thread has served
  long			thread_count;	/* # connections handled */
} Thread;

int				listenfd, nthreads;
socklen_t		addrlen;
Thread	*tptr;		/* array of Thread structures; calloc'ed */
pthread_mutex_t	mlock = PTHREAD_MUTEX_INITIALIZER;

int
main(int argc, char **argv)
{
	int		i;
    // Because it’s inside a function, the declarations’ scope is limited to that function body.
    void	sig_int(int), thread_make(int);

	if (argc == 3)
		listenfd = Tcp_listen(NULL, argv[1], &addrlen);
	else if (argc == 4)
		listenfd = Tcp_listen(argv[1], argv[2], &addrlen);
	else
		err_quit("usage: serv07 [ <host> ] <port#> <#threads>");
	nthreads = atoi(argv[argc-1]);

    // Calloc() (“clear allocate”) is just like malloc(), but with zero-initialization.
    // tptr = malloc(nthreads * sizeof(Thread));
    // memset(tptr, 0, nthreads * sizeof(Thread));
    tptr = Calloc(nthreads, sizeof(Thread));

	for (i = 0; i < nthreads; i++)
		thread_make(i);			/* only main thread returns */

	Signal(SIGINT, sig_int);

    // Suspend the process until a signal arrives.
	for ( ; ; )
		pause();	/* everything done by threads */
}
/* end serv07 */

void
sig_int(int signo)
{
	int		i;
	void	pr_cpu_time(void);

	pr_cpu_time();
	for (i = 0; i < nthreads; i++)
		printf("thread %d, %ld connections\n", i, tptr[i].thread_count);

	exit(0);
}

void
thread_make(int i)
{
    void	*thread_main(void *);

    Pthread_create(&tptr[i].thread_tid, NULL, &thread_main, (void *) i);
    return;		/* main thread returns */
}

void *
thread_main(void *arg)
{
    int				connfd;
    void			web_child(int);
    socklen_t		clilen;
    struct sockaddr	*cliaddr;

    cliaddr = Malloc(addrlen);

    printf("thread %d starting\n", (int) arg);
    for ( ; ; ) {
        clilen = addrlen;
        Pthread_mutex_lock(&mlock);
        connfd = Accept(listenfd, cliaddr, &clilen);
        Pthread_mutex_unlock(&mlock);

        // Thread pools array does not need a lock because it is pre-created with each allocated block of memory
        // So each thread will be assigned to use each block index -> no conflict here
        tptr[(int) arg].thread_count++;

        web_child(connfd);		/* process request */
        Close(connfd);
    }
}

#define	MAXN	16384		/* max # bytes client can request */

void
web_child(int sockfd)
{
    int			ntowrite;
    ssize_t		nread;
    char		line[MAXLINE], result[MAXN];

    for ( ; ; ) {
        if ( (nread = Readline(sockfd, line, MAXLINE)) == 0)
            return;		/* connection closed by other end */

            /* 4line from client specifies #bytes to write back */
        ntowrite = atol(line);
        if ((ntowrite <= 0) || (ntowrite > MAXN))
            err_quit("client request for %d bytes", ntowrite);

        Writen(sockfd, result, ntowrite);
    }
}

#include	<sys/resource.h>

#ifndef	HAVE_GETRUSAGE_PROTO
int		getrusage(int, struct rusage *);
#endif

void
pr_cpu_time(void)
{
    double			user, sys;
    struct rusage	myusage, childusage;

    if (getrusage(RUSAGE_SELF, &myusage) < 0)
        err_sys("getrusage error");
    if (getrusage(RUSAGE_CHILDREN, &childusage) < 0)
        err_sys("getrusage error");

    user = (double) myusage.ru_utime.tv_sec +
                    myusage.ru_utime.tv_usec/1000000.0;
    user += (double) childusage.ru_utime.tv_sec +
                     childusage.ru_utime.tv_usec/1000000.0;
    sys = (double) myusage.ru_stime.tv_sec +
                   myusage.ru_stime.tv_usec/1000000.0;
    sys += (double) childusage.ru_stime.tv_sec +
                    childusage.ru_stime.tv_usec/1000000.0;

    printf("\nuser time = %g, sys time = %g\n", user, sys);
}

