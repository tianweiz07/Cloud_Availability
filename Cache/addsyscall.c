#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/unistd.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <linux/syscalls.h>
#include <asm/io.h>
#include <asm/apic.h>
#include <asm/xen/events.h>
#include <asm/xen/page.h>
#include <asm/xen/hypercall.h>
#include <asm/xen/hypervisor.h>


// Cache configurations
#define CACHE_SET_NR 30720
#define CACHE_WAY_NR 8
#define CACHE_LINE_SIZE 64
#define CACHE_SIZE (CACHE_LINE_SIZE*CACHE_WAY_NR*CACHE_SET_NR)

// Page parameters
#define PAGE_COLOR (CACHE_SET_NR*CACHE_LINE_SIZE/PAGE_SIZE)
#define PAGE_PTR_NR 6
#define PAGE_ORDER 10
#define PAGE_NR 1024

// Timing parameters
#define SCAN_NR 1000
//#define INTERVAL1 10 // unit: s, for the whole program
//#define INTERVAL2 1 // unit: ms, for the cache scan interval

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
static uint64_t SYS_CALL_TABLE_ADDR = 0xffffffff81571420
#define __NR_CachePrime 188


unsigned long cache_pg[PAGE_COLOR][CACHE_WAY_NR];
int cache_idx[PAGE_COLOR];
struct page *ptr[PAGE_PTR_NR];

unsigned long page_to_virt(struct page* pg){
	return (unsigned long)phys_to_virt(page_to_phys(pg));
}

int page_to_color(struct page* pg) {
	return pfn_to_mfn(page_to_pfn(p))%PAGE_COLOR;
}

int virt_to_color(unsigned long virt) {
	return pfn_to_mfn(virt_to_pfn(virt))%PAGE_COLOR;
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
	return;
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
	return;
}


void CacheInit(void) {
	int i, j, c;
	for(i=0; i<PAGE_COLOR; i++) {
		cache_idx[i] = 0;
		for (j=0; j<CACHE_WAY_NR; j++)
			cache_pg[i][j] = 0;
	}

	for (i=0; i<PAGE_PTR_NR; i++) {
		ptr[i] = alloc_pages(GFP, PAGE_ORDER);
		for (int j=0; j<PAGE_NR; j++) {
			c = page_to_color(ptr[i] + j);
			if (cache_idx[c] < 24) {
				cache_pg[c][cache_idx] = page_to_virt(ptr[i] + j);
				cache_idx[c] ++;
			}
		}
	}
	return;
}

void CacheFree(void) {
	int i;
	for (i=0; i<PAGE_PTR_NR; i++)
		__free_pages(ptr[i], PAGE_ORDER)
}

int CacheCheck(void) {
	int i, j;
	int val = 1;
	for (i=0; i<PAGE_COLOR; i++)
		for (j=0; j<CACHE_WAY_NR; j++)
			if (i != virt_to_color(cache_pg[i][j]))
				val = 0;
	return val;
}

asmlinkage void sys_CachePrime(void) {
	int i, j, k;
	int p, q;
	unsigned long val;

	for (i=0; i<SCAN_NR; i++)
		for (j=0; j<CACHE_SET_NR; j++)
			for (k=0; k<CACHE_WAY_MR; k++) {
				p = j / PAGE_COLOR;
				q = j % PAGE_COLOR;
				val = *(unsigned long*)(cache_pg[p][k] + q * CACHE_LINE_SIZE);
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
	position_collect = (uint64_t(*)(void))(my_sys_call_table[__NR_CachePrime]);
	my_sys_call_table[__NR_CachePrime] = sys_CachePrime;
	SetPageR0(virt_to_page(my_sys_call_table));
	CacheInit();
	return 0;
}

void __exit exit_addsyscall(void) {
	printk("Module Exit\n");
	SetPageRW(virt_to_page(my_sys_call_table));
	my_sys_call_table[__NR_CachePrime] = position_collect;
	SetPageRO(virt_to_page(my_sys_call_table));
	CacheFree();
	return;
}

module_init(init_addsyscall);
module_exit(exit_addsyscall);

MODULE_LICENSE("GPL");