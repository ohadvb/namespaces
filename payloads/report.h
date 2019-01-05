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

void write_error(char * src, char * msg)
{
    FILE *f = fopen("log", "a");
    fprintf(f, "%s[%d] ERR: %s %d\n", src, getpid(), msg, errno);
    fclose(f);
}

void write_log(char * src, char * msg, int arg)
{
    FILE *f = fopen("log", "a");
    fprintf(f, "%s[%d] LOG: %s %d\n", src, getpid(), msg, arg);
    fclose(f);
}
