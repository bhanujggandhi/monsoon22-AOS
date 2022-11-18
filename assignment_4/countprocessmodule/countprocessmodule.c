#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/pid_namespace.h>
#include <linux/proc_fs.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>


/*
Method to count number of processes
Type of the process can be found from __state variable which is defined as volatile long
0 -> Running
1 -> Interruptable
2 -> Uninterruptable
*/
void count_proc(void) {
  int total = 0, running = 0, interruptable = 0, uninterruptible = 0;
    struct task_struct *proc;
    
    for_each_process(proc) {
        total++;
        if(proc->__state == TASK_RUNNING)
          running++;
        else if(proc->__state == TASK_INTERRUPTIBLE)
          interruptable++;
        else if(proc->__state == TASK_UNINTERRUPTIBLE)
          uninterruptible++;
    }
    printk(KERN_INFO "countprocessmodule: Total number of processes: %d\n", total);
    printk(KERN_INFO "countprocessmodule: Number of running processes: %d\n", running);
    printk(KERN_INFO "countprocessmodule: Number of interruptible processes: %d\n", interruptable);
    printk(KERN_INFO "countprocessmodule: Number of uninterruptible processes: %d\n", uninterruptible);

}

int proc_init(void) {
    printk(KERN_INFO "countprocessmodule: Initialising count process module\n");
    count_proc();
    return 0;
}

void proc_cleanup(void) {
    printk(KERN_INFO "countprocessmodule: performing cleanup of module\n");
}

MODULE_LICENSE("MIT");
MODULE_AUTHOR("Bhanuj Gandhi");
MODULE_DESCRIPTION("A module that counts number of tasks running, interrupt-able, and uniterruptible.");

module_init(proc_init);
module_exit(proc_cleanup);
