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
#define CACHE_SET_NR 64
#define CACHE_WAY_NR 8
#define CACHE_LINE_SIZE 64
#define CACHE_SIZE (CACHE_LINE_SIZE*CACHE_WAY_NR*CACHE_SET_NR)

// Page parameters
#define PAGE_PTR_NR 1
#define PAGE_ORDER 3
#define PAGE_NR 8

// Syscall information
static uint64_t SYS_CALL_TABLE_ADDR = 0xffffffff81801320;
#define __NR_CacheClean 188

char *icache_page[PAGE_NR];
uint64_t *dcache_page[PAGE_NR];
struct page *ptr[2];


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

asmlinkage void sys_CacheClean(int t) {
	unsigned long tsc1;

	tsc1 = jiffies + t * HZ;
	while (time_before(jiffies, tsc1)) {
		clean_dcache();
		clean_icache();
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
