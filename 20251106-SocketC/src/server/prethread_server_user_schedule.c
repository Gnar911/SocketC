#include	"../thread/unpthread.h"
#include "../unp.h"

// 11112025
// This is an example of TCP/IP Transport Layer Protocol using multiple threads and threads pool
// Main thread schedules the to pass the connected socket to one of it thread in thread pool
//
typedef struct {
  pthread_t		thread_tid;		/* thread ID */
  long			thread_count;	/* # connections handled */
} Thread;
extern Thread	*tptr;		/* array of Thread structures; calloc'ed */

#define	MAXNCLI	32
extern int					clifd[MAXNCLI], iget, iput;
extern pthread_mutex_t		clifd_mutex;
extern pthread_cond_t		clifd_cond;

Thread	*tptr;		/* array of Thread structures; calloc'ed */
int					clifd[MAXNCLI], iget, iput;

static int			nthreads;
pthread_mutex_t		clifd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		clifd_cond = PTHREAD_COND_INITIALIZER;

int
main(int argc, char **argv)
{
    int			i, listenfd, connfd;
    void		sig_int(int), thread_make(int);
    socklen_t	addrlen, clilen;
    struct sockaddr	*cliaddr;

    if (argc == 3)
        listenfd = Tcp_listen(NULL, argv[1], &addrlen);
    else if (argc == 4)
        listenfd = Tcp_listen(argv[1], argv[2], &addrlen);
    else
        err_quit("usage: serv08 [ <host> ] <port#> <#threads>");
    cliaddr = Malloc(addrlen);

    nthreads = atoi(argv[argc-1]);
    tptr = Calloc(nthreads, sizeof(Thread));
    iget = iput = 0;

        /* 4create all the threads */
    for (i = 0; i < nthreads; i++)
        thread_make(i);		/* only main thread returns */

    Signal(SIGINT, sig_int);

    for ( ; ; ) {
        clilen = addrlen;
        connfd = Accept(listenfd, cliaddr, &clilen);

        Pthread_mutex_lock(&clifd_mutex);
        clifd[iput] = connfd;
        // Increment index and check the boundary
        if (++iput == MAXNCLI)  iput = 0;
        if (iput == iget)       err_quit("iput = iget = %d", iput);

        // Signal to clifd_cond, unlock mutex
        Pthread_cond_signal(&clifd_cond);
        Pthread_mutex_unlock(&clifd_mutex);
    }
}
/* end serv08 */

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
	int		connfd;
	void	web_child(int);

	printf("thread %d starting\n", (int) arg);
	for ( ; ; ) {
    	Pthread_mutex_lock(&clifd_mutex);
        // When the lock is obtained, there is nothing to do if the iget and iput indexes are equal.
        // In that case the thread goes to sleep and waiting for signal, then also break while loop since the put > get
		while (iget == iput)
			Pthread_cond_wait(&clifd_cond, &clifd_mutex);
        // signal come, achieve lock
		connfd = clifd[iget];	/* connected socket to service */
        if (++iget == MAXNCLI) iget = 0;
		Pthread_mutex_unlock(&clifd_mutex);
		tptr[(int) arg].thread_count++;

		web_child(connfd);		/* process request */
		Close(connfd);
	}
}

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


