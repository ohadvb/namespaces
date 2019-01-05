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

FILE * pids = NULL;
FILE * g_log = NULL;

void report(char * arg)
{
    if (pids == NULL)
    {
        pids = fopen("pids", "a");
    }
    pid_t mypid = getpid();
    pid_t myppid = getppid();
    fprintf(pids, "proc: %s. pid: %d, ppid: %d\n", arg, mypid, myppid);

    uid_t ruid, euid, suid;
    getresuid(&ruid, &euid, &suid); 
    fprintf(pids, "proc: %s. ruid: %d, euid: %d, suid: %d\n", arg, ruid, euid, suid);

    fflush(pids);
}

void write_error(char * src, char * msg)
{
    if (g_log == NULL)
    {
        g_log = fopen("log", "a");
    }
    fprintf(g_log, "%s[%d] ERR: %s %d\n", src, getpid(), msg, errno);
    fflush(g_log);
}

void write_log(char * src, char * msg, int arg)
{
    if (g_log == NULL)
    {
        g_log = fopen("log", "a");
    }
    fprintf(g_log, "%s[%d] LOG: %s %d\n", src, getpid(), msg, arg);
    fflush(g_log);
}
