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
#define __NR_CachePrime 188

/*
static noinline void fetch_and_add(int *variable, int value) {
	asm volatile ("lock; xaddl %%eax, %2\n\t"
		      :"=a" (value)
		      :"a" (value), "m"(*variable)
		      :"memory");
}
*/

/*
static noinline void cache_flush(char *address) {
	__asm__ volatile ("clflush (%0)"
			  :
			  :"r"(address)
			  :"memory");
}
*/

static noinline void flush_fetch(char *flush_address, int *read_address, int value) {
	asm volatile ("clflush (%1)\n\t"
		      "clflush (%2)\n\t"
		      "lock; xaddl %%eax, %3\n\t"
		      :"=a"(value)
		      :"r"(flush_address), "r"(flush_address+CACHE_LINE_SIZE), "m"(*read_address), "a"(value)
		      :"memory");
}

struct page *ptr;
char *cache_list[8];

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
	ptr = alloc_pages(GFP_KERNEL, 3);
	for(i=0;i<8;i++)
		cache_list[i] = (char*)phys_to_virt(page_to_phys(ptr+i));

	for (i=0; i<8; i++) {
		for (j=0; j<64; j++) {
			for (k=0; k<64; k++) {
				cache_list[i][j*64+k] = k;
			}
		}
	}

	return;
}

void CacheFree(void) {
	__free_pages(ptr, 3);
}

asmlinkage void sys_CachePrime(int t) {
	unsigned long tsc;
	tsc = jiffies + t * HZ;
	while (time_before(jiffies, tsc)) {
		flush_fetch(cache_list[0], (int *)(cache_list[0]+CACHE_LINE_SIZE-1), 0x0);
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
	SetPageRO(virt_to_page(my_sys_call_table));
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
