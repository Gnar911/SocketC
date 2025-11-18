/* include web1 */
#include	"../thread/unpthread.h"
#include    "../unp.h"
//#include	<thread.h>		/* Solaris threads */

// This one-thread-per-client version is also many times faster than the one-child-per-client

#define	MAXFILES	20
#define	SERV		"80"	/* port number or service name */

struct file {
  char	*f_name;			/* filename */
  char	*f_host;			/* hostname or IP address */
  int    f_fd;				/* descriptor */
  int	 f_flags;			/* F_xxx below */
  pthread_t	 f_tid;			/* thread ID */
} file[MAXFILES];
#define	F_CONNECTING	1	/* connect() in progress */
#define	F_READING		2	/* connect() complete; now reading */
#define	F_DONE			4	/* all done */

#define	GET_CMD		"GET %s HTTP/1.0\r\n\r\n"

int		nconn, nfiles, nlefttoconn, nlefttoread;
pthread_mutex_t ndone_mutex = PTHREAD_MUTEX_INITIALIZER;
// We want a method for the main loop to go to sleep until one of its threads notifies it
// A condition variable, in conjunction with a mutex, provides this facility.
pthread_cond_t ndone_cond = PTHREAD_COND_INITIALIZER;
int ndone;

void	*do_get_read(void *);
void	home_page(const char *, const char *);
void	write_get_cmd(struct file *);

// As we know the TCP/Ip or UDP is the transport layer protocol.
// It gives a byte stream channel. It doesn’t know anything about HTTP, files, or commands — just bytes.
// The HTTP layer define the convention for those byte means.
// The format of early web servers (HTTP/1.0) is served HTML files, was designed for simple web pages, not interactive apps.
// Opens one TCP connection per request
// → connect → send one request → get full response → close the socket
// Request format:
//    GET /index.html HTTP/1.1\r\n
//    Host: www.example.com\r\n
//    User-Agent: MyClient\r\n
//    \r\n
//
// Response format:
//    HTTP/1.1 200 OK\r\n
//    Content-Type: text/html\r\n
//    Content-Length: 125\r\n
//    \r\n
//    <html> ... </html>
//
int
main(int argc, char **argv)
{
    int			i, n, maxnconn;
	pthread_t	tid;
	struct file	*fptr;

	if (argc < 5)
		err_quit("usage: web <#conns> <IPaddr> <homepage> file1 ...");
	maxnconn = atoi(argv[1]);

	nfiles = min(argc - 4, MAXFILES);
	for (i = 0; i < nfiles; i++) {
		file[i].f_name = argv[i + 4];
		file[i].f_host = argv[2];
		file[i].f_flags = 0;
	}
	printf("nfiles = %d\n", nfiles);

	home_page(argv[2], argv[3]);

	nlefttoread = nlefttoconn = nfiles;
	nconn = 0;
/* end web1 */
/* include web2 */
	while (nlefttoread > 0) {
		while (nconn < maxnconn && nlefttoconn > 0) {
				/* 4find a file to read */
			for (i = 0 ; i < nfiles; i++)
				if (file[i].f_flags == 0)
					break;
			if (i == nfiles)
				err_quit("nlefttoconn = %d but nothing found", nlefttoconn);

			file[i].f_flags = F_CONNECTING;
			Pthread_create(&tid, NULL, &do_get_read, &file[i]);
			file[i].f_tid = tid;
			nconn++;
			nlefttoconn--;
		}

        // thr_join(0,...)
        // Waits for any finished thread → simpler, Solaris-specific convenience.
        // pthread_join(tid,...)
        // Waits for that thread only → portable but requires you to track thread completions manually.
//        if ( (n = thr_join(0, &tid, (void **) &fptr)) != 0)
//			errno = n, err_sys("thr_join error");
//		nconn--;
//		nlefttoread--;
        //printf("thread id %d for %s done\n", tid, fptr->f_name);


        // Checking ndone every time around loop.
        // This is called polling and is considered a waste of CPU time.
//        Pthread_mutex_lock(&ndone_mutex);
//        if (ndone > 0) {
//            for (i = 0; i < nfiles; i++) {
//                if (file[i].f_flags & F_DONE) {
//                    Pthread_join(file[i].f_tid, (void **) &fptr);
//                    /* update file[i] for terminated thread */
//                    printf("thread id %d for %s done\n", (int)file[i].f_tid, fptr->f_name);
//                }
//            }
//        }
//        Pthread_mutex_unlock(&ndone_mutex);

        // We want a method for the main loop to go to sleep until one of its threads notifies it
        // A condition variable, in conjunction with a mutex, provides this facility.
        // The Pthread_join always call at main loop for management
        Pthread_mutex_lock(&ndone_mutex);
        while (ndone == 0)
            // This puts the calling thread to sleep and releases the mutex lock it holds
            Pthread_cond_wait (&ndone_cond, &ndone_mutex);
            // After return here by signal, hold the lock again
            for (i = 0; i < nfiles; i++) {
                if (file[i].f_flags & F_DONE) {
                    Pthread_join(file[i].f_tid, (void **) &fptr);
                    /* update file[i] for terminated thread */
                    printf("thread id %d for %s done\n", (int)file[i].f_tid, fptr->f_name);
                }
            }
        Pthread_mutex_unlock(&ndone_mutex);

    }
	exit(0);
}
/* end web2 */

/* include do_get_read */
void *
do_get_read(void *vptr)
{
	int					fd, n;
	char				line[MAXLINE];
	struct file			*fptr;

	fptr = (struct file *) vptr;

	fd = Tcp_connect(fptr->f_host, SERV);
	fptr->f_fd = fd;
	printf("do_get_read for %s, fd %d, thread %d\n",
			fptr->f_name, fd, fptr->f_tid);

	write_get_cmd(fptr);	/* write() the GET command */

		/* 4Read server's reply */
	for ( ; ; ) {
		if ( (n = Read(fd, line, MAXLINE)) == 0)
			break;		/* server closed connection */

		printf("read %d bytes from %s\n", n, fptr->f_name);
    }
	printf("end-of-file on %s\n", fptr->f_name);
	Close(fd);

    // Mark the flag done one, increase the done and notify for main thread
	fptr->f_flags = F_DONE;		/* clears F_READING */
    Pthread_mutex_lock(&ndone_mutex);
    ndone++;
    // manually signal to the conditon variable, it looks like it notifies update UI but it actually not
    Pthread_cond_signal(&ndone_cond);
    Pthread_mutex_unlock(&ndone_mutex);

	return(fptr);		/* terminate thread */
}
/* end do_get_read */

/* include write_get_cmd */
void
write_get_cmd(struct file *fptr)
{
	int		n;
	char	line[MAXLINE];

    // Send HTTP request format over TCP
	n = snprintf(line, sizeof(line), GET_CMD, fptr->f_name);
	Writen(fptr->f_fd, line, n);
	printf("wrote %d bytes for %s\n", n, fptr->f_name);

	fptr->f_flags = F_READING;			/* clears F_CONNECTING */
}
/* end write_get_cmd */

/* include home_page */
void
home_page(const char *host, const char *fname)
{
	int		fd, n;
	char	line[MAXLINE];

	fd = Tcp_connect(host, SERV);	/* blocking connect() */

	n = snprintf(line, sizeof(line), GET_CMD, fname);
	Writen(fd, line, n);

	for ( ; ; ) {
		if ( (n = Read(fd, line, MAXLINE)) == 0)
			break;		/* server closed connection */

		printf("read %d bytes of home page\n", n);
		/* do whatever with data */
	}
	printf("end-of-file on home page\n");
	Close(fd);
}
/* end home_page */
