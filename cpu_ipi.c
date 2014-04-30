#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/unistd.h>
#include <linux/cpumask.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <asm/apic.h>
#include <linux/jiffies.h>

#define INTERVAL 180
#define SLICE 15
#define FREQUENCY 3292606

#ifdef __i386
__inline__ uint64_t rdtsc() {
	uint64_t x;
	__asm__ volatile ("rdtsc" : "=A" (x));
	return x;
}
#elif __amd64
__inline__ uint64_t rdtsc() {
	uint64_t a, d;
	__asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
	return (d<<32) | a;
}
#endif;

struct task_struct *cpu_exe_task;
struct task_struct *cpu_ipi_task;

int cpu_exe(void *ptr){
	unsigned long long credit = 0;
	int exe_cpu = 1;

	unsigned long current_time;
	unsigned long next_time;

	set_cpus_allowed_ptr(current, get_cpu_mask(exe_cpu));

	current_time = jiffies;
	next_time = current_time + INTERVAL*HZ;

	while (time_before(jiffies, next_time)) {
		credit ++;
	}

	printk("CPU EXECUTION DONE!\n");
	printk("Credit: %llu\n", credit);
	return 0;
}

int cpu_ipi(void *ptr){
	int send_cpu = 0;
	int recv_cpu = 1;

	unsigned long current_time;
	unsigned long next_time;
	unsigned long long tsc;

	set_cpus_allowed_ptr(current, get_cpu_mask(send_cpu));

	current_time = jiffies;
	next_time = current_time + INTERVAL*HZ;

	while (taime_before(jiffies, next_time)) {
		tsc = rdtsc() + SLICE*FREQUENCY;
//		apic->send_IPI_mask(get_cpu_mask(recv_cpu), RESCHEDULE_VECTOR);
		while (rdtsc() < tsc);
	}
	
	printk("CPU INTERRUPTION DONE!\n");
	return 0;
}


static int __init cpu_ipi_init(void){

	printk("entering cpu_ipi module\n");
//	cpu_exe_task = kthread_run(&cpu_exe, NULL, "CPU EXECUTION");
	cpu_ipi_task = kthread_run(&cpu_ipi, NULL, "CPU INTERRUPTION");
	return 0;
}

static void __exit cpu_ipi_exit(void){
	printk("leaving cpu_ipi module\n");
}

module_init(cpu_ipi_init); 
module_exit(cpu_ipi_exit);

MODULE_LICENSE("GPL");
