/*
 * Wrapper functions for our own library functions.
 * Most are included in the source file for the function itself.
 */

#include	"unp.h"

const char *
Inet_ntop(int family, const void *addrptr, char *strptr, size_t len)
{
	const char	*ptr;

	if (strptr == NULL)		/* check for old code */
		err_quit("NULL 3rd argument to inet_ntop");
	if ( (ptr = inet_ntop(family, addrptr, strptr, len)) == NULL)
		err_sys("inet_ntop error");		/* sets errno */
	return(ptr);
}

void
Inet_pton(int family, const char *strptr, void *addrptr)
{
	int		n;

	if ( (n = inet_pton(family, strptr, addrptr)) < 0)
		err_sys("inet_pton error for %s", strptr);	/* errno set */
	else if (n == 0)
		err_quit("inet_pton error for %s", strptr);	/* errno not set */

	/* nothing to return */
}

ssize_t Readline(int fd, void *vptr, size_t maxlen)
{
    ssize_t n, rc;
    char    c, *ptr;

    ptr = vptr;
    for (n = 1; n < maxlen; n++) {
        if ( (rc = Recv(fd, &c, 1, 0)) == 1 ) {
            *ptr++ = c;
            if (c == '\n')
                break;      // newline is stored, like fgets()
        } else if (rc == 0) {
            *ptr = 0;
            return (n - 1); // EOF, no data read
        } else {
            if (errno == EINTR)
                continue;
            return -1;      // error
        }
    }
    *ptr = 0;
    return n;
}
