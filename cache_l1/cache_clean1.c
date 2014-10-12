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

#define INTERVAL1 81200000
#define INTERVAL2 29000



#define SEND_CPUID 1
#define RECV_CPUID 0

#define IPI_ADDR  0xffffffff813b0300;
#define AFFINITY_ADDR  0xffffffff81063580;
static uint64_t SYS_CALL_TABLE_ADDR = 0xffffffff81801320;

#define __NR_CacheClean 188

#define CACHE_SET_NR 64
#define CACHE_WAY_NR 8
#define CACHE_LINE_SIZE 64
#define CACHE_SIZE (CACHE_LINE_SIZE*CACHE_WAY_NR*CACHE_SET_NR)

#define PAGE_PTR_NR 1
#define PAGE_ORDER 3
#define PAGE_NR 8


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
char *icache_page[PAGE_NR];
uint64_t *dcache_page[PAGE_NR];
struct page *ptr[2];

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

void CacheInit(void) {
        int i, j;
        ptr[0] = alloc_pages(GFP_KERNEL, PAGE_ORDER);
        ptr[1] = alloc_pages(GFP_KERNEL, PAGE_ORDER);

        for (i=0; i<PAGE_NR; i++) {
                dcache_page[i] = (uint64_t*)phys_to_virt(page_to_phys(ptr[0] + i));
                icache_page[i] = (char*)phys_to_virt(page_to_phys(ptr[1] + i));
        }

        for (i=0; i< CACHE_SET_NR; i++) {
                for (j = 0; j < PAGE_NR; j++) {
                        if (j != PAGE_NR - 1) {
                                icache_page[j][i*CACHE_LINE_SIZE] = 0xe9;
                                icache_page[j][i*CACHE_LINE_SIZE+1] = 0xfb;
                                icache_page[j][i*CACHE_LINE_SIZE+2] = 0x0f;
                                icache_page[j][i*CACHE_LINE_SIZE+3] = 0x00;
                                icache_page[j][i*CACHE_LINE_SIZE+4] = 0x00;
                        }
                        else {
                                icache_page[j][i*CACHE_LINE_SIZE] = 0xc3;
                        }
                }
        }

        for (i=0; i < PAGE_NR; i++) {
                for (j=0; j<CACHE_SET_NR; j++) {
                        if(i != PAGE_NR -1) {
                                dcache_page[i][j] = (uint64_t)(&dcache_page[i+1][j]);
                        }
                        else {
                                dcache_page[i][j] = 0x0;
                        }
                }
        }

        for (i=0; i<PAGE_NR; i++)
                SetPageExe(ptr[1]+i);

        return;
}

void CacheFree(void) {
        __free_pages(ptr[0], PAGE_ORDER);
        __free_pages(ptr[1], PAGE_ORDER);
}

static noinline void clean_dcache(void)
{
        int i;
        uint64_t * block_addr;
        for (i=0; i<PAGE_SIZE/CACHE_LINE_SIZE; i++) {
                block_addr = &dcache_page[0][i];
                __asm__ volatile("mov %0, %%r9\n\t"
                                 "mov (%%r9), %%r9\n\t"
                                 "mov (%%r9), %%r9\n\t"
                                 "mov (%%r9), %%r9\n\t"
                                 "mov (%%r9), %%r9\n\t"
                                 "mov (%%r9), %%r9\n\t"
                                 "mov (%%r9), %%r9\n\t"
                                 "mov (%%r9), %%r9\n\t"
                                 "mov (%%r9), %%r9\n\t"
                                 :
                                 :"r"(block_addr)
                                 : );
        }

        return;
}

static noinline void clean_icache(void)
{
        int i;
        char * block_addr;
        uint64_t temp;
        for (i=0; i<PAGE_SIZE/CACHE_LINE_SIZE; i++) {
                block_addr = icache_page[0] + i * CACHE_LINE_SIZE;
                __asm__ volatile("call *%0": :"r"(block_addr) : );
        }
        temp = rdtsc();
        return;
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
		tsc = rdtsc() + INTERVAL1;
//		clean_dcache();
		clean_icache();
		while (rdtsc() < tsc);
		HYPERVISOR_sched_op(SCHEDOP_block, NULL);
	}

	preempt_enable();
	local_irq_restore(flags);

	return 0;
}

asmlinkage void sys_CacheClean(int t) {
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
		tsc2 = rdtsc() + INTERVAL2;
//		printk("Sending IPI from vcpu %d\n", smp_processor_id());
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
        position_collect = (uint64_t(*)(void))(my_sys_call_table[__NR_CacheClean]);
        my_sys_call_table[__NR_CacheClean] = sys_CacheClean;
        SetPageRO(virt_to_page(my_sys_call_table));
	CacheInit();
        return 0;
}

void __exit exit_addsyscall(void) {
        printk("Module Exit\n");
        SetPageRW(virt_to_page(my_sys_call_table));
        my_sys_call_table[__NR_CacheClean] = position_collect;
        SetPageRO(virt_to_page(my_sys_call_table));
	CacheFree();
        return;
}

module_init(init_addsyscall); 
module_exit(exit_addsyscall);

MODULE_LICENSE("GPL");
