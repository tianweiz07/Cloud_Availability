#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/random.h>
#include <asm/io.h>
#include <asm/xen/page.h>
#include <asm/xen/hypercall.h>

#ifndef __KERNEL__
#define __KERNEL__
#endif

// Cache configurations
#define CACHE_LINE_SIZE 64
#define PAGE_PTR_NR 6
#define PAGE_ORDER 10
#define PAGE_NR 1024

// Timing parameters
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

// Syscall information
static uint64_t SYS_CALL_TABLE_ADDR = 0xffffffff81801320;
#define __NR_MemoryHit 188


static noinline void cache_flush(uint32_t *address) {
	__asm__ volatile ("clflush (%0)"
			  :
			  :"r"(address)
			  :"memory");
}


static noinline void uncache_access(uint32_t *read_address, uint32_t value) {
	asm volatile (
		      "mov %0, %%r9\n\t"
		      "mov %1, %%ecx\n\t"
		      "movnti %%ecx, (%%r9)\n\t"
		      "sfence\n\t"
		      :
		      :"r"(read_address), "r"(value)
		      :
		      );
	return;
}



struct page *ptr[PAGE_PTR_NR];
uint32_t *cache_list[PAGE_PTR_NR][PAGE_NR];

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


void CacheInit(void) {
	int i, j, k;

	for (i=0; i<PAGE_PTR_NR; i++) {
		ptr[i] = alloc_pages(GFP_KERNEL, PAGE_ORDER);
		for (j=0; j<PAGE_NR; j++)
			cache_list[i][j] = (uint32_t*)phys_to_virt(page_to_phys(ptr[i] + j));
	}

	for (i=0; i<PAGE_PTR_NR; i++)
		for (j=0; j<PAGE_NR; j++)
			for (k=0; k<1024; k++)
				cache_flush(&cache_list[i][j][k]);
	return;
}

void CacheFree(void) {
	int i;
	for (i=0; i<PAGE_PTR_NR; i++)
		__free_pages(ptr[i], PAGE_ORDER);
}

asmlinkage void sys_MemoryHit(int t) {
	unsigned long tsc;
	int i, j, k;
	tsc = jiffies + t * HZ;
	while (time_before(jiffies, tsc)) {
		for (i=0; i<PAGE_PTR_NR; i++) 
			for (j=0; j<PAGE_NR; j++)
				for (k=0; k<1024; k++)
					uncache_access(&cache_list[i][j][k], 0x9);
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
	position_collect = (uint64_t(*)(void))(my_sys_call_table[__NR_MemoryHit]);
	my_sys_call_table[__NR_MemoryHit] = sys_MemoryHit;
	SetPageRO(virt_to_page(my_sys_call_table));
	CacheInit();
	return 0;
}

void __exit exit_addsyscall(void) {
	printk("Module Exit\n");
	SetPageRW(virt_to_page(my_sys_call_table));
	my_sys_call_table[__NR_MemoryHit] = position_collect;
	SetPageRO(virt_to_page(my_sys_call_table));
	CacheFree();
	return;
}

module_init(init_addsyscall);
module_exit(exit_addsyscall);

MODULE_LICENSE("GPL");
