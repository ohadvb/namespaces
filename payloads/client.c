#define _GNU_SOURCE
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
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include "report.h"

char *socket_path = "\0hidden";

void my_err(char * msg)
{
    write_error("client", msg);
}
void my_log(char * msg, int arg)
{
    write_log("client", msg, arg);
}

#define CONTROLLEN  CMSG_LEN(sizeof(int))

static struct cmsghdr   *cmptr = NULL;  /* malloc'ed first time */

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

int main(int argc, char * argv[])
{
	struct sockaddr_un addr;
	char buf[100];
	int fd,rc;

	report("client");
	if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		my_err("socket error");
		exit(-1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	*addr.sun_path = '\0';
	strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2);

    my_log("connecting", 0);
	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		my_err("connect error");
	}

    my_log("opening", 0);
    int root_fd = open("/", 0);
    if (root_fd < 0)
    {
        my_err("couldn't open rootdir");
        return -1;
    }
    my_log("opened", root_fd);

    send_fd(fd, root_fd);
    return 0;
}
