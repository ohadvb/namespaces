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

static char child_stack[4096];

void report(char * arg)
{
    FILE *f = fopen("pids", "a");
    pid_t mypid = getpid();
    pid_t myppid = getppid();
    fprintf(f, "proc: %s. pid: %d, ppid: %d\n", arg, mypid, myppid);

    uid_t ruid, euid, suid;
    getresuid(&ruid, &euid, &suid); 
    fprintf(f, "proc: %s. ruid: %d, euid: %d, suid: %d\n", arg, ruid, euid, suid);

    fclose(f);
}

int child_func(void * arg)
{
    report("child");
    sleep(3000);
}

void check(int r, char * err)
{
    if (!r)
    {
        FILE * ef = fopen("error", "a");
        fprintf(ef, "%s errno: %d\n", err, errno);
        fclose(ef);
    }
}

int main(int argc, char * argv[])
{
    /* if (!strcmp(argv[0], "init")) */
    /* { */
    /*     sleep(3000); */
    /*     return 1; */
    /* } */
    report(argv[0]);

    mkdir("/bla", 0755);
    int root_dir = open("/", 0);
    check(root_dir >= 0, "open");

    int ret = chroot("/bla");
    check(ret == 0, "chroot1");

    ret = fchdir(root_dir);
    check(ret == 0, "fchdir");
    ret = chroot(".");
    check(ret == 0, "chroot2");

    pid_t child_pid = clone(child_func, child_stack + 4096, CLONE_NEWUSER | SIGCHLD,  NULL);
    check(child_pid >= 0, "clone");
 
    sleep(3000);
}
