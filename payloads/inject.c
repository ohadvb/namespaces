#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>

#define wordsize sizeof(long)

#if defined (__x86_64__) || defined (__i386__)
    #include <sys/reg.h>
    #define BREAKPOINT "\xcc"
    #define BREAKPOINT_LEN 1
#endif

#ifdef __x86_64__
    #define IP RIP
#elif __i386__
    #define IP EIP
#elif __arm__
    #define IP 15 /* PC register */
    #define BREAKPOINT "\xe7\xf0\x01\xf0"
    #define BREAKPOINT_LEN 4
#else
    #error unsupported architeture
#endif

#define ps_inject_default (ps_inject_t){ 1, 0, 0, 0 }

void good(char * msg)
{
	write_log("inject", msg, 0);	
}

void info(char * msg)
{
	good(msg);
}


typedef ssize_t (*ps_inject_writecallback)(int, const void *, size_t, off_t);
typedef ssize_t (*ps_inject_readcallback)(int, void *, size_t, off_t);

typedef struct {
    int restore;
    int use_ptrace;
    int restore_ip;
    pid_t pid;
} ps_inject_t;


void ps_inject(const char *sc, size_t len, int pid);

inline long getip(pid_t pid){
    return ptrace(PTRACE_PEEKUSER, pid, sizeof(long)*IP, 0L);
}

inline long setip(pid_t pid, long ip){
    return ptrace(PTRACE_POKEUSER, pid, sizeof(long)*IP, ip);
}

ssize_t ptrace_write(pid_t pid, const void *data, size_t len, long addr){
    size_t i;
    long word, old;
    int final_size;

    for(i=0; i<len; i+=wordsize){
        if((i+wordsize) > len){
            final_size = len-i;
            word = 0;

            memcpy(&word, data+i, final_size);
            old = ptrace(PTRACE_PEEKDATA, pid, addr+i, 0L);
            old &= (unsigned long)-1 << (8*final_size);
            word |= old;
            ptrace(PTRACE_POKEDATA, pid, addr+i, word);

        } else {
            word = *(long *)(data+i);
            ptrace(PTRACE_POKEDATA, pid, addr+i, word);
        }
    }

    return 0;

}


ssize_t ptrace_read(pid_t pid, void *output, size_t n, long addr){
    size_t i;
    long bytes;


    for(i=0; i<n; i+=wordsize){
        bytes = ptrace(PTRACE_PEEKDATA, pid, addr+i, 0L);
        if((i+wordsize) > n){
            memcpy((output+i), &bytes, n-i);
        } else {
            *(long *)(output+i) = bytes;
        }
    }

    return 0;

}

void ps_inject(const char *sc, size_t len, int pid){
    ps_inject_writecallback writecallback = NULL;
    ps_inject_readcallback readcallback = NULL;
    char *instructions_backup, memfile[100];
    int status, identifier = 0;
    long instruction_point;

    instruction_point =  ptrace(PTRACE_PEEKUSER, pid, sizeof(long)*IP, 0L);

    /* if(!options->use_ptrace){ */
    /*     info("opening /proc/%d/mem\n", options->pid); */
    /*     snprintf(memfile, sizeof(memfile), "/proc/%d/mem", options->pid); */
    /*     identifier = xopen(memfile, O_RDWR); */
    /*     good("sucess\n"); */
    /*  */
    /*     writecallback = pwrite; */
    /*     readcallback = pread; */
    /*  */
    /* } else { */
	writecallback = ptrace_write;
	readcallback = ptrace_read;
	identifier = pid;
    /* } */

    /* if(options->restore){ */
    /*     instructions_backup = xmalloc(len+BREAKPOINT_LEN); */
    /*     info("backup previously instructions\n"); */
    /*     readcallback(identifier, instructions_backup, len+BREAKPOINT_LEN, instruction_point); */
    /* } */

    info("writing shellcode on memory\n");
    writecallback(identifier, sc, len, instruction_point);

    good("Shellcode inject !!!\n");

    /* if(options->restore){ */
    /*     info("resuming application ...\n"); */
    /*     writecallback(identifier, BREAKPOINT, BREAKPOINT_LEN, instruction_point+len); */
    /*  */
    /*     ptrace(PTRACE_CONT, options->pid, NULL, 0); */
    /*     waitpid(options->pid, &status, 0); */
    /*  */
    /*     info("restoring memory instructions\n"); */
    /*     writecallback(identifier, instructions_backup, len+BREAKPOINT_LEN, instruction_point); */
    /*  */
    /*     xfree(instructions_backup); */
    /*  */
    /*     if(options->restore_ip){ */
    /*         setip(options->pid, instruction_point); */
    /*     } */
    /*  */
    /*     #if defined(__x86_64__) || defined(__i386__) */
    /*     else { */
    /*         setip(options->pid, getip(options->pid)-BREAKPOINT_LEN); */
    /*     } */
    /*     #endif */
    /* } */
    /*  */
    info("detaching pid ...\n");
    ptrace(PTRACE_DETACH, pid, NULL, NULL);


}
