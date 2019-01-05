#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>

FILE * g_log = NULL;

void report(char * arg)
{
    if (g_log == NULL)
    {
        g_log = fopen("log", "a");
    }
    pid_t mypid = getpid();
    pid_t myppid = getppid();
    fprintf(g_log, "%ld %s[%d] PIDS: ppid: %d\n", time(NULL), arg, mypid, myppid);

    uid_t ruid, euid, suid;
    getresuid(&ruid, &euid, &suid); 
    fprintf(g_log, "%ld %s[%d] UIDS: ruid: %d, euid: %d, suid: %d\n", time(NULL), arg, mypid, ruid, euid, suid);

    fflush(g_log);
}

void write_error(char * src, char * msg)
{
    if (g_log == NULL)
    {
        g_log = fopen("log", "a");
    }
    fprintf(g_log, "%ld %s[%d] ERR: %s %d\n", time(NULL), src, getpid(), msg, errno);
    fflush(g_log);
}

void write_log(char * src, char * msg, int arg)
{
    if (g_log == NULL)
    {
        g_log = fopen("log", "a");
    }
    fprintf(g_log, "%ld %s[%d] LOG: %s %d\n", time(NULL), src, getpid(), msg, arg);
    fflush(g_log);
}

#define CONTROLLEN  CMSG_LEN(sizeof(int))
static struct cmsghdr   *cmptr = NULL;      /* malloc'ed first time */

int recv_fd(int fd)
{
	int             newfd, nr, status;
	char            *ptr;
	char            buf[4096];
	struct iovec    iov[1];
	struct msghdr   msg;

	status = -1;
	iov[0].iov_base = buf;
	iov[0].iov_len  = sizeof(buf);
	msg.msg_iov     = iov;
	msg.msg_iovlen  = 1;
	msg.msg_name    = NULL;
	msg.msg_namelen = 0;
	if (cmptr == NULL && (cmptr = malloc(CONTROLLEN)) == NULL)
		return(-1);
	msg.msg_control    = cmptr;
	msg.msg_controllen = CONTROLLEN;
	if ((nr = recvmsg(fd, &msg, 0)) < 0) {
		write_error("framework", "recvmsg error");
	} else if (nr == 0) {
		write_error("framework", "connection closed by server");
		return(-1);
	}
	/*
	* See if this is the final data with null & status.  Null
	* is next to last byte of buffer; status byte is last byte.
	* Zero status means there is a file descriptor to receive.
	*/
	for (ptr = buf; ptr < &buf[nr]; ) {
		if (*ptr++ == 0) {
			if (ptr != &buf[nr-1])
				write_error("framework", "message format error");
			status = *ptr & 0xFF;  /* prevent sign extension */
			if (status == 0) {
				if (msg.msg_controllen != CONTROLLEN)
					write_error("framework", "status = 0 but no fd");
				newfd = *(int *)CMSG_DATA(cmptr);
			} else {
				newfd = -status;
			}
			nr -= 2;
		}
	}
	return newfd;
}

/*
 * Pass a file descriptor to another process.
 * If fd<0, then -fd is sent back instead as the error status.
 */
int send_fd(int fd, int fd_to_send)
{
    struct iovec    iov[1];
    struct msghdr   msg;
    char            buf[2]; /* send_fd()/recv_fd() 2-byte protocol */

    iov[0].iov_base = buf;
    iov[0].iov_len  = 2;
    msg.msg_iov     = iov;
    msg.msg_iovlen  = 1;
    msg.msg_name    = NULL;
    msg.msg_namelen = 0;

	if (cmptr == NULL && (cmptr = malloc(CONTROLLEN)) == NULL)
		return(-1);
	cmptr->cmsg_level  = SOL_SOCKET;
	cmptr->cmsg_type   = SCM_RIGHTS;
	cmptr->cmsg_len    = CONTROLLEN;
	msg.msg_control    = cmptr;
	msg.msg_controllen = CONTROLLEN;
	*(int *)CMSG_DATA(cmptr) = fd_to_send;     /* the fd to pass */
	buf[1] = 0;          /* zero status means OK */
	buf[0] = 0;              /* null byte flag to recv_fd() */
    if (sendmsg(fd, &msg, 0) != 2)
        return(-1);
    return(0);
}


