#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <asm/io.h>
#include <asm/xen/page.h>
#include <asm/xen/hypercall.h>

#ifndef __KERNEL__
#define __KERNEL__
#endif

// Cache configurations
#define CACHE_LINE_SIZE 64


// Syscall information
static uint64_t SYS_CALL_TABLE_ADDR = 0xffffffff81801320;

#define __NR_BusLock 188

struct page *ptr;
char *cache_list;


static noinline void atomic_fetch(int *read_address, int value) {
	asm volatile (
		      "lock; xaddl %%eax, %1\n\t"
		      :"=a"(value)
		      :"m"(*read_address), "a"(value)
		      :"memory");
}

uint64_t rdtsc(void) {
	uint64_t a, d;
	__asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
	return (d<<32) | a;
}


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

void CacheInit(void) {
	ptr = alloc_pages(GFP_KERNEL, 1);
	cache_list = (char*)phys_to_virt(page_to_phys(ptr));
	return;
}

void CacheFree(void) {
	__free_pages(ptr, 1);
}

asmlinkage void sys_BusLock(int t) {
	unsigned long tsc;
	tsc = jiffies + t * HZ;
	set_cpus_allowed_ptr(current, cpumask_of(0));
	while (time_before(jiffies, tsc)) {
		atomic_fetch((int *)(cache_list + CACHE_LINE_SIZE - 1), 0x0);
	}
	return;
}

void **my_sys_call_table;
void (*temp_func)(void);
static uint64_t (*position_collect)(void);

int __init init_addsyscall(void) {
	printk("Module Init\n");
	my_sys_call_table = (void**) SYS_CALL_TABLE_ADDR;
	SetPageRW(virt_to_page(my_sys_call_table));
	position_collect = (uint64_t(*)(void))(my_sys_call_table[__NR_BusLock]);
	my_sys_call_table[__NR_BusLock] = sys_BusLock;
	SetPageRO(virt_to_page(my_sys_call_table));
	CacheInit();
	return 0;
}

void __exit exit_addsyscall(void) {
	printk("Module Exit\n");
	SetPageRW(virt_to_page(my_sys_call_table));
	my_sys_call_table[__NR_BusLock] = position_collect;
	SetPageRO(virt_to_page(my_sys_call_table));
	CacheFree();
	return;
}

module_init(init_addsyscall);
module_exit(exit_addsyscall);

MODULE_LICENSE("GPL");
