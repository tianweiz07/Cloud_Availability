#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/cpumask.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <asm/apic.h>
#include <linux/jiffies.h>

#define INTERVAL 600

struct task_struct *cpu_exe_task;

int cpu_exe(void *ptr){
	int num = 0;
	int exe_cpu = 1;

	unsigned long current_time;
	unsigned long next_time;

	set_cpus_allowed_ptr(current, get_cpu_mask(exe_cpu));

	current_time = jiffies;
	next_time = current_time + INTERVAL*HZ;

	while (time_before(jiffies, next_time)) {
		num ++;
		num --;
	}

	printk("CPU EXECUTION DONE!\n");
	return 0;
}

static int __init cpu_exe_init(void){

	printk("entering cpu_exe module\n");
	cpu_exe_task = kthread_run(&cpu_exe, NULL, "CPU EXECUTION");
	return 0;
}

static void __exit cpu_exe_exit(void){
	printk("leaving cpu_exe module\n");
}

module_init(cpu_exe_init); 
module_exit(cpu_exe_exit);

MODULE_LICENSE("GPL");
