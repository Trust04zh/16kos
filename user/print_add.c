#include <ulib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(){
    int i = 1;
    for(;i<=2;i++){
        fork();
    }
    cprintf("pid number: %d , the address of the pcb block: %u\n",getpid(),get_pcb_address());
    return 0;
}