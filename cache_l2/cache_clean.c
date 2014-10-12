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
#define CACHE_SET_NR 512
#define CACHE_WAY_NR 8
#define CACHE_LINE_SIZE 64
#define CACHE_SIZE (CACHE_LINE_SIZE*CACHE_WAY_NR*CACHE_SET_NR)

// Page parameters
#define PAGE_PTR_NR 1
#define PAGE_ORDER 7
#define PAGE_NR 128
#define PAGE_COLOR (CACHE_SET_NR*CACHE_LINE_SIZE/PAGE_SIZE)

// Syscall information
static uint64_t SYS_CALL_TABLE_ADDR = 0xffffffff81801320;
#define __NR_CacheClean 188

uint64_t *cache_page[PAGE_COLOR][CACHE_WAY_NR];
int cache_idx[PAGE_COLOR];
struct page *ptr[PAGE_PTR_NR];


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

int page_to_color(struct page* page) {
	return (page_to_virt(page)/PAGE_SIZE)%PAGE_COLOR;
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
	int i, j, k, c;
	for (i=0; i<PAGE_PTR_NR; i++)
		ptr[i] = alloc_pages(GFP_KERNEL, PAGE_ORDER);

	for(i=0; i<PAGE_COLOR; i++) {
		cache_idx[i] = 0;
		for (j=0; j<CACHE_WAY_NR; j++)
			cache_page[i][j] = 0;
	}


	for (i=0; i<PAGE_PTR_NR; i++) {
		for (j=0; j<PAGE_NR; j++) {
			c = page_to_color(ptr[i] + j);
			if (cache_idx[c] < CACHE_WAY_NR) {
				cache_page[c][cache_idx[c]] = (uint64_t*)page_to_virt(ptr[i] + j);
				cache_idx[c] ++;
			}
		}
	}

	for (i=0; i<PAGE_COLOR; i++)
		if (cache_idx[i] < CACHE_WAY_NR) {
			printk("Initialization Error!\n");
			return;
		}


	for (i=0; i < PAGE_COLOR; i++) {
		for (j=0; j < PAGE_SIZE/CACHE_LINE_SIZE; j++) {
			for (k=0; k < CACHE_WAY_NR; k++) {
				if (k != CACHE_WAY_NR-1) {
					cache_page[i][k][j] = (uint64_t)(&cache_page[i][k+1][j]);
				}
				else {
					cache_page[i][k][j] = 0x0;
				}
			}
		}

	}
	return;
}

void CacheFree(void) {
	int i;
	for (i=0; i<PAGE_PTR_NR; i++)
		__free_pages(ptr[i], PAGE_ORDER);
}

static noinline void clean_cache(void)
{
	int i, j;
	uint64_t * block_addr;
	for (i=0; i<PAGE_SIZE/CACHE_LINE_SIZE; i++) {
		for (j=0; j<PAGE_COLOR; j++) {
			block_addr = &cache_page[j][0][i];
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
	}
        return;
}

asmlinkage void sys_CacheClean(int t) {
	unsigned long tsc1;

	tsc1 = jiffies + t * HZ;
	while (time_before(jiffies, tsc1)) {
		clean_cache();
//		clean_icache();
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
