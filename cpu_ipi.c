#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/cpumask.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <asm/apic.h>
#include <linux/jiffies.h>

#define INTERVAL 600

struct task_struct *cpu_ipi_task;

int cpu_ipi(void *ptr){
	int send_cpu = 0;
	int recv_cpu = 1;

	unsigned long current_time;
	unsigned long next_time;

	set_cpus_allowed_ptr(current, get_cpu_mask(send_cpu));

	current_time = jiffies;
	next_time = current_time + INTERVAL*HZ;

	while (time_before(jiffies, next_time)) {
		apic->send_IPI_mask(get_cpu_mask(recv_cpu), RESCHEDULE_VECTOR);
	}
	
	printk("CPU INTERRUPTION DONE!\n");
	return 0;
}


static int __init cpu_ipi_init(void){

	printk("entering cpu_ipi module\n");
	cpu_ipi_task = kthread_run(&cpu_ipi, NULL, "CPU INTERRUPTION");
	return 0;
}

static void __exit cpu_ipi_exit(void){
	printk("leaving cpu_ipi module\n");
}

module_init(cpu_ipi_init); 
module_exit(cpu_ipi_exit);

MODULE_LICENSE("GPL");
