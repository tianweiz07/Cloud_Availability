#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/io.h>
#include <asm/xen/page.h>
#include <asm/xen/hypercall.h>

#ifndef __KERNEL__
#define __KERNEL__
#endif

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


unsigned long cache_pg[PAGE_COLOR][CACHE_WAY_NR];
int cache_idx[PAGE_COLOR];
struct page *ptr[PAGE_PTR_NR];

unsigned long cache_timestamp[CACHE_SET_NR];
unsigned long cache_timestamp_copy[CACHE_SET_NR];
int cache_flag[CACHE_SET_NR];

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


void CacheInit(void) {
	int i, j, c;
	for (i=0; i<PAGE_PTR_NR; i++)
		ptr[i] = alloc_pages(GFP_KERNEL, PAGE_ORDER);

	for(i=0; i<PAGE_COLOR; i++) {
		cache_idx[i] = 0;
		for (j=0; j<CACHE_WAY_NR; j++)
			cache_pg[i][j] = 0;
	}

	for (i=0; i<PAGE_PTR_NR; i++) {
		for (j=0; j<PAGE_NR; j++) {
			c = page_to_color(ptr[i] + j);
			if (cache_idx[c] < CACHE_WAY_NR) {
				cache_pg[c][cache_idx[c]] = page_to_virt(ptr[i] + j);
				cache_idx[c] ++;
			}
		}
	}

	for (i=0; i<CACHE_SET_NR; i++) {
		cache_timestamp[i] = 0;
		cache_timestamp_copy[i] = 0;
		cache_flag[i] = 0;
	}
	return;
}

void CacheFree(void) {
	int i;
	for (i=0; i<PAGE_PTR_NR; i++)
		__free_pages(ptr[i], PAGE_ORDER);
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

int partition(unsigned long *array, int left, int right) {
	int pos = right;
	int p = left;
	int q = right-1;
	int temp;

	if (left>=right)
		return -1;

	while (p<=q) {
		while ((p<right)&&(array[p]<=array[pos]))
			p++;
		while ((q>left)&&(array[q]>array[pos]))
			q--;
		if (p>=q)
			break;
		temp = array[p];
		array[p] = array[q];
		array[q] = temp;
	}
	temp = array[p];
	array[p] = array[pos];
	array[pos] = temp;
	return p;
}

int getMedian(unsigned long *array, int size) {
	int p = 0;
	int q = size -1;
	int mid = size/2;
	int i = 0;

	if ((array == NULL)||(size <=0))
		return -1;

	while (i!= mid) {
		i = partition(array, p, q);
		if (i < mid)
			p = i + 1;
		if (i> mid)
			q = i - 1;
	}
	return i;
}

void CacheTrain(void) {
	int i, median;
	for (i=0; i<CACHE_SET_NR; i++)
		cache_timestamp_copy[i] = cache_timestamp[i];
	median = getMedian(cache_timestamp_copy, CACHE_SET_NR);
	for (i=0; i<CACHE_SET_NR; i++)
		if (cache_timestamp[i]<cache_timestamp_copy[median])
			cache_flag[i] = 1;
}

static noinline unsigned long CacheScan0(void) {
	int i, j, p, q;
	unsigned long temp;
	uint64_t tsc;
	
	for (i=0; i<CACHE_SET_NR; i++) {
			tsc = rdtsc();
			for (j=0; j<CACHE_WAY_NR; j++) {
				p = i / (PAGE_SIZE/CACHE_LINE_SIZE);
				q = i % (PAGE_SIZE/CACHE_LINE_SIZE);
				temp = *(unsigned long*)(cache_pg[p][j] + q * CACHE_LINE_SIZE);
			}
			cache_timestamp[i] += rdtsc() - tsc;
	}

	return temp;
}

static noinline unsigned long CacheScan1(void) {
	int i, j, p, q;
	unsigned long temp;
	
	for (i=0; i<CACHE_SET_NR; i++) {
		if (cache_flag[i] == 0) {
			for (j=0; j<CACHE_WAY_NR; j++) {
				p = i / (PAGE_SIZE/CACHE_LINE_SIZE);
				q = i % (PAGE_SIZE/CACHE_LINE_SIZE);
				temp = *(unsigned long*)(cache_pg[p][j] + q * CACHE_LINE_SIZE);
			}
		}
	}

	return temp;
}

asmlinkage void sys_CachePrime(int t1, int t2, unsigned long *timestamp, int *flag) {
	int k;
	int val;
	unsigned long tsc1;
	unsigned long tsc2;

	val = CacheCheck();
	if (!val) {
		printk("Cache Initialization Error\n");
		return;
	}
	else {
		printk("Cache Initialization Correct\n");
		tsc1 = jiffies + t1 * HZ;
		tsc2 = jiffies + t2 * HZ;
		while (time_before(jiffies, tsc1)) {
			CacheScan0();
		}

		CacheTrain();

		while (time_before(jiffies, tsc2)) {
			CacheScan1();
		}
		for (k=0; k<CACHE_SET_NR; k++) {
			timestamp[k] = cache_timestamp[k];
			flag[k] = cache_flag[k];
		}
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
