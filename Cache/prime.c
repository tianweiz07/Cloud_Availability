#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/unistd.h>
#include <linux/cpumask.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <linux/syscalls.h>
#include <asm/io.h>
#include <asm/apic.h>
#include <asm/xen/events.h>
#include <asm/xen/hypercall.h>
#include <asm/xen/hypervisor.h>


#define CACHE_SIZE 32768
#define CACHE_SET_NR 64
#define CACHELINE_SIZE 64
#define PAGE_NR (CACHE_SIZE/PAGE_SIZE)
#define PAGE_ORDER 3

#define INTERVAL1 10 // unit: s, for the whole program
#define INTERVAL2 1 // unit: ms, for the cache scan interval
#define FREQUENCY 3292600


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


struct page *g_pages;        // used for prime and probe
char *cache_list[PAGE_NR];

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

void MemInit(void) {
	int i, j;
	g_pages = alloc_pages(GFP_KERNEL, PAGE_ORDER);
	for(i=0; i<PAGE_NR; i++) {
		SetPageRW(g_pages+i);
		cache_list[i] = (char*)phys_to_virt(page_to_phys(g_pages+i));
	
		for(j=0; j<PAGE_SIZE/sizeof(char); j++)	
			cache_list[i][j] = 0x00;
	}
	return;
}

void cache_scan(void) {
	int i, j;
	char value;
	for (i=0; i<PAGE_NR; i++) 
		for (j=0; j<PAGE_SIZE/CACHELINE_SIZE; j++) 
			value = cache_list[i][j*CACHELINE_SIZE/sizeof(char)];
	return;
}

static int __init prime_init(void) {
	unsigned long tsc1;
	uint64_t tsc2;
	printk("Entering prime module\n");

	MemInit();
	tsc1 = jiffies + INTERVAL1 * HZ;

	while (time_before(jiffies, tsc1)) {
		tsc2 = rdtsc() + INTERVAL2 * FREQUENCY;
		cache_scan();
		while (rdtsc() < tsc2);
	}

	return 0;
}

static void __exit prime_exit(void) {
	printk("Leaving prime module\n");
	__free_pages(g_pages, PAGE_ORDER);
}

module_init(prime_init);
module_exit(prime_exit);

MODULE_LICENSE("GPL");
