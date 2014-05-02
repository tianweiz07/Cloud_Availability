#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/unistd.h>
#include <linux/cpumask.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <asm/apic.h>
#include <linux/jiffies.h>
#include <asm/xen/events.h>

#define INTERVAL 180
#define SLICE 0
#define FREQUENCY 3292606
#define FUN_ADDRESS 0xffffffff813af320
#define NUM_VCPU 8

#ifdef __i386
__inline__ uint64_t rdtsc(void) {
	uint64_t x;
	__asm__ volatile ("rdtsc" : "=A" (x));
	return x;
}
#elif __amd64
__inline__ uint64_t rdtsc(void) {
	uint64_t a, d;
	__asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
	return (d<<32) | a;
}
#endif

void (*xen_send_IPI_one)(unsigned int cpu,enum ipi_vector vector) = FUN_ADDRESS;


int exe_cpu0 = 0;
int exe_cpu1 = 1;
int exe_cpu2 = 2;
int exe_cpu3 = 3;
int exe_cpu4 = 4;
int exe_cpu5 = 5;
int exe_cpu6 = 6;
int exe_cpu7 = 7;

int cpu_exe(void *data){
	uint64_t credit = 0;
	unsigned long next_time;
	uint64_t tsc;
	int exe_cpu_id;
	int ipi_cpu_id;

	exe_cpu_id = *((int*) data);
	ipi_cpu_id = (exe_cpu_id + 1) % NUM_VCPU;

	set_cpus_allowed_ptr(current, get_cpu_mask(exe_cpu_id));

	next_time = jiffies + INTERVAL*HZ;

	while (time_before(jiffies, next_time)) {
		tsc = rdtsc() + SLICE*FREQUENCY;
		while (rdtsc() < tsc)
			credit ++;
		xen_send_IPI_one(ipi_cpu_id, XEN_CALL_FUNCTION_VECTOR);
	}

	printk("CPU EXECUTION DONE!\n");
	printk("Credit %d: %llu\n", exe_cpu_id, credit);
	return 0;
}

int cpu_ipi(void *data){
	unsigned long next_time;
	uint64_t tsc;
	int ipi_cpu0 = 0;

	set_cpus_allowed_ptr(current, get_cpu_mask(ipi_cpu0));


	next_time = jiffies + INTERVAL*HZ;

	while (time_before(jiffies, next_time)) {
		tsc = rdtsc() + SLICE*FREQUENCY;
		xen_send_IPI_one(exe_cpu0, XEN_CALL_FUNCTION_VECTOR);
		while (rdtsc() < tsc);
	}
	
	printk("CPU INTERRUPTION DONE!\n");

	return 0;
}


static int __init cpu_ipi_init(void){
	struct task_struct *cpu_exe_task0;
	struct task_struct *cpu_exe_task1;
	struct task_struct *cpu_exe_task2;
	struct task_struct *cpu_exe_task3;
	struct task_struct *cpu_exe_task4;
	struct task_struct *cpu_exe_task5;
	struct task_struct *cpu_exe_task6;
	struct task_struct *cpu_exe_task7;
//	struct task_struct *cpu_ipi_task;

	void *exe_cpu_ptr0 = (void *)&exe_cpu0;
	void *exe_cpu_ptr1 = (void *)&exe_cpu1;
	void *exe_cpu_ptr2 = (void *)&exe_cpu2;
	void *exe_cpu_ptr3 = (void *)&exe_cpu3;
	void *exe_cpu_ptr4 = (void *)&exe_cpu4;
	void *exe_cpu_ptr5 = (void *)&exe_cpu5;
	void *exe_cpu_ptr6 = (void *)&exe_cpu6;
	void *exe_cpu_ptr7 = (void *)&exe_cpu7;

	printk("entering cpu_ipi module\n");
	cpu_exe_task0 = kthread_run(&cpu_exe, exe_cpu_ptr0, "CPU EXECUTION");
	cpu_exe_task1 = kthread_run(&cpu_exe, exe_cpu_ptr1, "CPU EXECUTION");
	cpu_exe_task2 = kthread_run(&cpu_exe, exe_cpu_ptr2, "CPU EXECUTION");
	cpu_exe_task3 = kthread_run(&cpu_exe, exe_cpu_ptr3, "CPU EXECUTION");
	cpu_exe_task4 = kthread_run(&cpu_exe, exe_cpu_ptr4, "CPU EXECUTION");
	cpu_exe_task5 = kthread_run(&cpu_exe, exe_cpu_ptr5, "CPU EXECUTION");
	cpu_exe_task6 = kthread_run(&cpu_exe, exe_cpu_ptr6, "CPU EXECUTION");
	cpu_exe_task7 = kthread_run(&cpu_exe, exe_cpu_ptr7, "CPU EXECUTION");
//	cpu_ipi_task = kthread_run(&cpu_ipi, NULL, "CPU INTERRUPTION");

	return 0;
}

static void __exit cpu_ipi_exit(void){
	printk("leaving cpu_ipi module\n");
}

module_init(cpu_ipi_init); 
module_exit(cpu_ipi_exit);

MODULE_LICENSE("GPL");
