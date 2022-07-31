#include <defs.h>
#include <list.h>
#include <proc.h>
#include <assert.h>
#include <priority_sched.h>
#include <stdio.h>
static void
priority_init(struct run_queue *rq) {
    list_init(&(rq->run_list));
    rq->proc_num = 0;
}

static void
priority_enqueue(struct run_queue *rq, struct proc_struct *proc) {


    list_add_before(&(rq->run_list), &(proc->run_link));
    list_entry_t * temp = &(proc->run_link);
    while(1){
        if (temp->prev != &(rq->run_list)){
            struct proc_struct * pcb_pre = le2proc(temp->prev, run_link);
            struct proc_struct * pcb = le2proc(temp, run_link);
            if(pcb->cfs_priority > pcb_pre->cfs_priority){
                list_exchange_pre(temp);
            }else{
                break;
            }
        }
        else{
            break;
        }
        
    }
    proc->rq = rq;
    rq->proc_num ++;
}

static void
priority_dequeue(struct run_queue *rq, struct proc_struct *proc) {
    assert(!list_empty(&(proc->run_link)) && proc->rq == rq);
    list_del_init(&(proc->run_link));
    rq->proc_num --;
}

static struct proc_struct *
priority_pick_next(struct run_queue *rq) {
    list_entry_t *le = list_next(&(rq->run_list));
    if (le != &(rq->run_list)) {
        return le2proc(le, run_link);
    }
    return NULL;
}

static void
priority_proc_tick(struct run_queue *rq, struct proc_struct *proc) {
    if (proc->time_slice > 0) {
        proc->time_slice --;
    }
    if (proc->time_slice == 0) {
        proc->need_resched = 1;
    }
}

struct sched_class priority_sched_class = {
    .name = "priority_scheduler",
    .init = priority_init,
    .enqueue = priority_enqueue,
    .dequeue = priority_dequeue,
    .pick_next = priority_pick_next,
    .proc_tick = priority_proc_tick,
};

