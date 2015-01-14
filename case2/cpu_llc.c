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


#define CACHE_SET_NR 12288
#define CACHE_WAY_NR 20
#define CACHE_LINE_SIZE 64
#define CACHE_SIZE (CACHE_LINE_SIZE*CACHE_WAY_NR*CACHE_SET_NR)

#define PAGE_PTR_NR 8
#define PAGE_ORDER 10
#define PAGE_NR 1024
#define PAGE_COLOR (CACHE_SET_NR*CACHE_LINE_SIZE/PAGE_SIZE)

struct page *ptr[PAGE_PTR_NR];
int cache_idx[PAGE_COLOR];
unsigned long cache_pg[PAGE_COLOR][CACHE_WAY_NR];

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

int page_to_color(struct page* page) {
	return pfn_to_mfn(page_to_pfn(page))%PAGE_COLOR;
}

int virt_to_color(unsigned long virt) {
	return pfn_to_mfn(virt_to_pfn((unsigned long)virt))%PAGE_COLOR;
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

int CacheInit(void) {
	int i, j, c;
	int val = 1;
	for (i=0; i<PAGE_PTR_NR; i++)
		ptr[i] = alloc_pages(GFP_KERNEL, PAGE_ORDER);

	for (i=0; i<PAGE_COLOR; i++) {
		cache_idx[i] = 0;
		for (j=0; j<CACHE_WAY_NR; j++)
			cache_pg[i][j] = 0;
	}

	for (i=0; i<PAGE_PTR_NR; i++) {
		for (j=0; j<PAGE_NR; j++) {
			c = page_to_color(ptr[i]+j);
			if (cache_idx[c] < CACHE_WAY_NR) {
				cache_pg[c][cache_idx[c]] = page_to_virt(ptr[i]+j);
				cache_idx[c] ++;
			}
		}
	}

	for (i=0; i<PAGE_COLOR; i++)
		for (j=0; j<CACHE_WAY_NR; j++)
			if (i != virt_to_color(cache_pg[i][j]))
				val = 0;
	return val;
}

void CacheFree(void) {
	int i;
	for (i=0; i<PAGE_PTR_NR; i++) 
		__free_pages(ptr[i], PAGE_ORDER);
}

static noinline unsigned long CacheScan(void) {
	int i, j, p, q;
	unsigned long temp = 0;
	uint64_t tsc;
	
	tsc = rdtsc();
	for (i=0; i<CACHE_SET_NR; i++) {
		for (j=0; j<CACHE_WAY_NR; j++) {
			p = i / (PAGE_SIZE/CACHE_LINE_SIZE);
			q = i % (PAGE_SIZE/CACHE_LINE_SIZE);
			temp = *(unsigned long*)(cache_pg[p][j] + q * CACHE_LINE_SIZE);
		}
	}

	return temp;
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
		while (rdtsc() < tsc) {
			CacheScan();
		}
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
	int val;
        printk("Module Init\n");
        my_sys_call_table = (void**) SYS_CALL_TABLE_ADDR;
        SetPageRW(virt_to_page(my_sys_call_table));
        position_collect = (uint64_t(*)(void))(my_sys_call_table[__NR_CpuPreempt]);
        my_sys_call_table[__NR_CpuPreempt] = sys_CpuPreempt;
        SetPageRO(virt_to_page(my_sys_call_table));
	val = CacheInit();
	printk("CacheInit: %d\n", val);
        return 0;
}

void __exit exit_addsyscall(void) {
        printk("Module Exit\n");
        SetPageRW(virt_to_page(my_sys_call_table));
        my_sys_call_table[__NR_CpuPreempt] = position_collect;
        SetPageRO(virt_to_page(my_sys_call_table));
	CacheFree();
        return;
}

module_init(init_addsyscall); 
module_exit(exit_addsyscall);

MODULE_LICENSE("GPL");
