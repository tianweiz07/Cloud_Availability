#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/unistd.h>
#include <linux/cpumask.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <asm/apic.h>
#include <asm/xen/events.h>
#include <asm/xen/hypercall.h>
#include <asm/xen/hypervisor.h>

//unit: millisecond
#define INTERVAL1 1 
#define INTERVAL2 4

#define ROUND_NUM 20

// Check /proc/kallsyms to get the following addresses
#define IPI_ADDR 0xffffffff813af770
#define AFFINITY_ADDR 0xffffffff81063580

// iscpu to get the CPU frequency
#define FREQUENCY 3292600

#define SEND_CPUID 0
#define RECV_CPUID 1


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

atomic_t round_count;

void (*xen_send_IPI_one)(unsigned int cpu, enum ipi_vector vector) = IPI_ADDR;
long (*m_sched_setaffinity)(pid_t pid, const struct cpumask *in_mask) = AFFINITY_ADDR;

void set_affinity(unsigned int cpu) {
	struct task_struct *m_task, *m_thread;
	for_each_process(m_task) {
		m_sched_setaffinity(m_task->pid, cpumask_of(cpu));
		list_for_each_entry(m_thread, &m_task->thread_group, thread_group) {
			m_sched_setaffinity(m_thread->pid, cpumask_of(cpu));
		}
	}
}

void working_thread(void *ptr) {
	uint64_t tsc;
	uint64_t credit;
	printk("Running working_thread @ vcpu %d\n", smp_processor_id());
	tsc = rdtsc() + INTERVAL1 * FREQUENCY;

	while (rdtsc() < tsc)
		credit ++;
	return;
}

int loop(void *ptr) {
	int val;
	unsigned long flags;
	printk("Entering loop function\n");
	set_cpus_allowed_ptr(current, cpumask_of(RECV_CPUID));

	local_irq_save(flags);
	preempt_disable();

	while (1) {
		atomic_inc(&round_count);
		val = atomic_read(&round_count);
		if (val >= ROUND_NUM)
			break;
		working_thread(NULL);
		HYPERVISOR_sched_op(SCHEDOP_block, NULL);
	}

	preempt_enable();
	local_irq_restore(flags);

	return 0;
}

static int __init ipi_init(void) {
	uint64_t tsc;
	printk("Entering ipi module\n");
	set_affinity(SEND_CPUID);
	set_cpus_allowed_ptr(current, cpumask_of(SEND_CPUID));
	atomic_set(&round_count, -1);

	kernel_thread(loop, NULL, 0);

	while (atomic_read(&round_count) < 0)
		schedule();

	
	while (atomic_read(&round_count) < ROUND_NUM) {
		tsc = rdtsc() + INTERVAL2 * FREQUENCY;
		printk("Sending IPI from vcpu %d\n", smp_processor_id());
		xen_send_IPI_one(RECV_CPUID, XEN_CALL_FUNCTION_VECTOR);
		while (rdtsc() < tsc);
	}
	return 0;
}

static void __exit ipi_exit(void){
	printk("Leaving ipi module\n");
}

module_init(ipi_init); 
module_exit(ipi_exit);

MODULE_LICENSE("GPL");
