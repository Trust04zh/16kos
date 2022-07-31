#include <defs.h>
#include <stdio.h>
#include <string.h>
#include <console.h>
#include <kdebug.h>
#include <trap.h>
#include <clock.h>
#include <intr.h>
#include <pmm.h>
#include <vmm.h>
#include <ide.h>
#include <swap.h>
#include <proc.h>
#include <kmonitor.h>

int kern_init(void) __attribute__((noreturn));
int
kern_init(void) {
    extern uint64_t boot_page_table_sv47[];
    boot_page_table_sv47[0] = 0;
    extern char edata[], end[];
    memset(edata, 0, end - edata);
    cons_init();                // init the console

    const char *message = "OS is loading ...";
    cprintf("%s\n\n", message);

    pmm_init();                 // init physical memory management

    idt_init();                 // init interrupt descriptor table

    vmm_init();                 // init virtual memory management
    sched_init();
    proc_init();                // init process table
    
    ide_init();                 // init ide devices
    swap_init();                // init swap

    clock_init();               // init clock interrupt
    intr_enable();              // enable irq interrupt
    cprintf("Start run the idle process\n");
    cpu_idle();                 // run idle process
}
