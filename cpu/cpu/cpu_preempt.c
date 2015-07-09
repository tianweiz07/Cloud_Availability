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
#include <asm/xen/page.h>
#include <asm/xen/events.h>
#include <asm/xen/hypercall.h>
#include <asm/xen/hypervisor.h>

#ifndef __KERNEL__
#define __KERNEL__
#endif

#define INTERVAL1 30
#define INTERVAL2 0.01

#define FREQUENCY 2900054

#define SEND_CPUID 1
#define RECV_CPUID 0

#define IPI_ADDR  0xffffffff813b0450
#define AFFINITY_ADDR  0xffffffff81063550
static uint64_t SYS_CALL_TABLE_ADDR = 0xffffffff81801320;

#define __NR_CpuPreempt 188

uint64_t rdtsc(void) {
	uint64_t a, d;
	__asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
	return (d<<32) | a;
}

atomic_t round_count;

void (*xen_send_IPI_one)(unsigned int cpu, enum ipi_vector vector) = IPI_ADDR;
long (*m_sched_setaffinity)(pid_t pid, const struct cpumask *in_mask) = AFFINITY_ADDR;


unsigned long page_to_virt(struct page* page){
        return (unsigned long)phys_to_virt(page_to_phys(page));
}

void SetPageRW(struct page* page)
{
        pte_t *pte, ptev;
        unsigned long address = (unsigned long)phys_to_virt(page_to_phys(page));
        unsigned int level;

        pte = lookup_address(address, &level);
        BUG_ON(pte == NULL);

        ptev = pte_mkwrite(*pte);
        if (HYPERVISOR_update_va_mapping(address, ptev, 0))
                BUG();
}

void SetPageRO(struct page* page)
{
        pte_t *pte, ptev;
        unsigned long address = (unsigned long)phys_to_virt(page_to_phys(page));
        unsigned int level;

        pte = lookup_address(address, &level);
        BUG_ON(pte == NULL);

        ptev = pte_wrprotect(*pte);
        if (HYPERVISOR_update_va_mapping(address, ptev, 0))
                BUG();
}

void SetPageExe(struct page* page)
{
        pte_t *pte, ptev;
        unsigned long address = (unsigned long)phys_to_virt(page_to_phys(page));
        unsigned int level;

        pte = lookup_address(address, &level);
        BUG_ON(pte == NULL);

        ptev = pte_mkexec(*pte);
        if (HYPERVISOR_update_va_mapping(address, ptev, 0))
               BUG();
}

void set_affinity(unsigned int cpu) {
	struct task_struct *m_task, *m_thread;
	for_each_process(m_task) {
		m_sched_setaffinity(m_task->pid, cpumask_of(cpu));
		list_for_each_entry(m_thread, &m_task->thread_group, thread_group) {
			m_sched_setaffinity(m_thread->pid, cpumask_of(cpu));
		}
	}
}

int loop(void *ptr) {
	unsigned long flags;
	uint64_t tsc;
	set_cpus_allowed_ptr(current, cpumask_of(RECV_CPUID));

	local_irq_save(flags);
	preempt_disable();

	atomic_inc(&round_count);
	while (1) {
		if (atomic_read(&round_count) >= 1)
			break;
		tsc = rdtsc() + INTERVAL1 * FREQUENCY;
		while (rdtsc() < tsc);
		HYPERVISOR_sched_op(SCHEDOP_block, NULL);
	}

	preempt_enable();
	local_irq_restore(flags);

	return 0;
}

asmlinkage void sys_CpuPreempt(int t) {
	unsigned long tsc1;
	uint64_t tsc2;
	set_affinity(SEND_CPUID);
	set_cpus_allowed_ptr(current, cpumask_of(SEND_CPUID));
	atomic_set(&round_count, -1);

	kernel_thread(loop, NULL, 0);

	while (atomic_read(&round_count) < 0)
		schedule();

	tsc1 = jiffies + t * HZ;
	while (time_before(jiffies, tsc1)) {
		tsc2 = rdtsc() + INTERVAL2 * FREQUENCY;
		xen_send_IPI_one(RECV_CPUID, XEN_CALL_FUNCTION_VECTOR);
		while (rdtsc() < tsc2);
	}
	atomic_inc(&round_count);
	return;
}

void **my_sys_call_table;
void (*temp_func)(void);
static uint64_t (*position_collect)(void);

int __init init_addsyscall(void) {
        printk("Module Init\n");
        my_sys_call_table = (void**) SYS_CALL_TABLE_ADDR;
        SetPageRW(virt_to_page(my_sys_call_table));
        position_collect = (uint64_t(*)(void))(my_sys_call_table[__NR_CpuPreempt]);
        my_sys_call_table[__NR_CpuPreempt] = sys_CpuPreempt;
        SetPageRO(virt_to_page(my_sys_call_table));
        return 0;
}

void __exit exit_addsyscall(void) {
        printk("Module Exit\n");
        SetPageRW(virt_to_page(my_sys_call_table));
        my_sys_call_table[__NR_CpuPreempt] = position_collect;
        SetPageRO(virt_to_page(my_sys_call_table));
        return;
}

module_init(init_addsyscall); 
module_exit(exit_addsyscall);

MODULE_LICENSE("GPL");
