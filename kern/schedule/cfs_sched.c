#include <defs.h>
#include <list.h>
#include <proc.h>
#include <assert.h>
#include <cfs_sched.h>
#include <stdio.h>
static int
get_all_priority(struct run_queue *rq){
    int all_priority = 0;
    list_entry_t *le = list_next(&(rq->run_list));
    while (1)
    {
        struct proc_struct *proc = le2proc(le, run_link);
        all_priority +=proc->cfs_priority;
        // cprintf("proc->cfs_priority : %u\n",proc->cfs_priority);
        le = le->next;
        if(le == &(rq->run_list)){
            break;
        }
    }
    return all_priority;
};
static void
cfs_init(struct run_queue * rq) {
    list_init(&(rq->run_list));
    rq->proc_num = 0;
}

static void
cfs_enqueue(struct run_queue *rq, struct proc_struct *proc) {
    // cprintf("call the function cfs_enqueue\n");
    // cprintf("the get_all_priority(rq) is %u \n",get_all_priority(rq));
    list_add_before(&(rq->run_list), &(proc->run_link));
    if (proc->time_slice <= 0 ) {
        if(get_all_priority(rq) == 0){
            proc->time_slice = rq->max_time_slice; //For the pid 1,2
        }else{
            proc->time_slice = (rq->max_time_slice  * 5 * rq->proc_num) * (uint32_t)(proc->cfs_priority/get_all_priority(rq));
        }
    }
    if(proc->cfs_priority != 0){
        proc->real_run_time += proc->time_slice;
        proc->vruntime = ((float)proc->real_run_time) /(float)proc->cfs_priority; 

    }
       proc->rq = rq;
    rq->proc_num ++;
};


static void
cfs_dequeue(struct run_queue *rq, struct proc_struct *proc) {
    assert(!list_empty(&(proc->run_link)) && proc->rq == rq);
    list_del_init(&(proc->run_link));
    rq->proc_num --;
}

static struct proc_struct *
cfs_pick_next(struct run_queue *rq) {
    list_entry_t *le = list_next(&(rq->run_list));
    struct proc_struct * to_return = NULL;
    float min_vruntime = 4294967296; // 2^ 32
    
    while (1)
    {
        struct proc_struct *proc = le2proc(le, run_link);
        if(proc->vruntime < min_vruntime){
            min_vruntime = proc->vruntime;
            to_return = proc;
        }
        le = le->next;
        if(le == &(rq->run_list)){
            break;
        }
    }
    return to_return;
}

static void
cfs_proc_tick(struct run_queue *rq, struct proc_struct *proc) {
    if (proc->time_slice > 0) {
        proc->time_slice --;
    }
    if (proc->time_slice == 0) {
        proc->need_resched = 1;
    }
}

struct sched_class cfs_sched_class = {
    .name = "cfs_scheduler",
    .init = cfs_init,
    .enqueue = cfs_enqueue,
    .dequeue = cfs_dequeue,
    .pick_next = cfs_pick_next,
    .proc_tick = cfs_proc_tick,
};

