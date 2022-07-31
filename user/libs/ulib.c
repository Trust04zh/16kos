#include <defs.h>
#include <syscall.h>
#include <stdio.h>
#include <ulib.h>

void
exit(int error_code) {
    //cprintf("eee\n");
    sys_exit(error_code);
    cprintf("BUG: exit failed.\n");
    while (1);
}

int
fork(void) {
    return sys_fork();
}

int
wait(void) {
    return sys_wait(0, NULL);
}

int
waitpid(int pid, int *store) {
    return sys_wait(pid, store);
}

void
yield(void) {
    sys_yield();
}

int
kill(int pid) {
    return sys_kill(pid);
}

int
getpid(void) {
    return sys_getpid();
}

unsigned int
gettime_msec(void) {
    return (unsigned int)sys_gettime();
}

int
get_pcb_address(void){
    return sys_get_pcb_address();
}

int 
set_priority_cfs(int p){
    // cprintf("set_priority_cfs : %d,%d\n",p,(int64_t)p);
    return sys_set_priority_cfs((int64_t)p);
}