#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include "report.h"

void my_err(char * msg)
{
    write_error("makens", msg);
}
void my_log(char * msg, int arg)
{
    write_log("makens", msg, arg);
}

char * socket_path = "tmp/chroots/old_0/socket";

int main(int argc, char * argv[])
{
	struct sockaddr_un addr;
	char buf[100];
	int fd,rc;
    g_log = fopen("/tmp/log", "a");
    report("makens");
    int ret = unshare(CLONE_NEWUSER|CLONE_NEWPID|CLONE_NEWNS);

    int setgroups = open("proc/self/setgroups", 1);
    if (setgroups < 0)
    {
        my_err("open setgroups");
    }
    char * deny = "deny";
    ret = write(setgroups, deny, strlen(deny));

    int uid_map = open("proc/self/uid_map", 1);
    if (uid_map < 0)
    {
        my_err("open uid_map");
    }
    char * mapping = "0 1 1";
    ret = write(uid_map, mapping, strlen(mapping));

    int gid_map = open("proc/self/gid_map", 1);
    if (gid_map < 0)
    {
        my_err("open gid_map");
    }
    ret = write(gid_map, mapping, strlen(mapping));

    if (fork())
    {
        return 0;
    }
    report("makens");

    if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		my_err("socket error");
		exit(-1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path));

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
    
    /* char * mount_point = "tmp/chroots/old_0/proc"; */
    /* ret = mkdir(mount_point, 0755); */
    /* if (ret != 0) */
    /* { */
    /*     my_err("mkdir"); */
    /* } */
    /*  */
    /* ret = mount("none", mount_point, "proc", 0, ""); */
    /* if (ret != 0) */
    /* { */
    /*     my_err("mount"); */
    /* } */

    sleep(3000);
}
