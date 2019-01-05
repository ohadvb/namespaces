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
    my_log("sent", 0);
    return 0;
}
