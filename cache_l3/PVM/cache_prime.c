#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/random.h>
#include <asm/io.h>
#include <asm/xen/page.h>
#include <asm/types.h>
#include <asm/xen/hypercall.h>

#ifndef __KERNEL__
#define __KERNEL__
#endif

#define CACHE_LINE_SIZE 64

#define PAGE_PTR_NR 8
#define PAGE_ORDER 10
#define PAGE_NR 1024

#define ROUND 1000
#define THRESHOLD 80

static uint64_t SYS_CALL_TABLE_ADDR = 0xffffffff81801320;
#define __NR_CacheSearch 188
#define __NR_CacheClean 189

struct page *ptr[PAGE_PTR_NR];
void **my_sys_call_table;
static uint64_t (*position_collect1)(void);
static uint64_t (*position_collect2)(void);

uint64_t *cache_page[32][150];
int cache_index[32];
unsigned long cache_timing[150];
int cache_order[150];

uint8_t cache_location[2048][6];

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


static noinline void working_thread(uint64_t *cache_addr, unsigned long *timing_location)
{
	unsigned long tsc;
        __asm__ volatile(
		"mov %1, %%r9\n\t"
		"lfence\n\t"
                "rdtsc\n\t"
                "mov %%eax, %%edi\n\t"
		"mov (%%r9), %%r10\n\t"
		"lfence\n\t"
                "rdtsc\n\t"
                "sub %%edi, %%eax\n\t"
                :"=a"(tsc)
                :"r"(cache_addr)
                :
                );
	*timing_location += tsc;
        return;
}


static noinline void working_thread1(uint64_t *cache_addr)
{
        __asm__ volatile(
		"mov %0, %%r9\n\t"
		"mov (%%r9), %%r10\n\t"
                :
                :"r"(cache_addr)
                :
                );
        return;
}

static noinline void working_thread2(uint64_t *cache_addr, unsigned long *timing_location)
{
	unsigned long tsc = 0;
        __asm__ volatile(
		"mov %1, %%r9\n\t"
		"lfence\n\t"
                "rdtsc\n\t"
                "mov %%eax, %%edi\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
//		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"lfence\n\t"
                "rdtsc\n\t"
                "sub %%edi, %%eax\n\t"
                :"=a"(tsc)
                :"r"(cache_addr)
                :
                );
	*timing_location += tsc;
        return;
}


unsigned long page_to_virt(struct page* page){
	return (unsigned long)phys_to_virt(page_to_phys(page));
}

void update_order(int size) {
	unsigned int i, t1, t2, temp;
	for (i = 0; i<size/2; i++) {
		get_random_bytes(&t1, sizeof(t1));
		get_random_bytes(&t2, sizeof(t2));
		temp = cache_order[t1%size];
		cache_order[t1%size] = cache_order[t2%size];
		cache_order[t2%size] = temp;
	}
}


static noinline void working_thread3(uint64_t *cache_addr)
{
        __asm__ volatile(
		"mov %0, %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
//		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
		"mov (%%r9), %%r9\n\t"
                :
                :"r"(cache_addr)
                :
                );
        return;
}

void CacheInit(void) {
	int i, j, k, c;
	for (i=0; i<PAGE_PTR_NR; i++)
		ptr[i] = alloc_pages(GFP_KERNEL, PAGE_ORDER);

	for (i=0; i<32; i++) {
		cache_index[i] = 0;
		for (j=0; j<150; j++) 
			cache_page[i][j] = 0;
	}


	for (i=0; i<PAGE_PTR_NR; i++) 
		for (j=0; j<PAGE_NR; j++) {
			c = pfn_to_mfn(page_to_pfn(ptr[i]+j)) % 32;
			if (cache_index[c] < 150) {
				cache_page[c][cache_index[c]] = (uint64_t *)page_to_virt(ptr[i] + j);
				cache_index[c] ++;
			}
		}
	
	for (i=0; i<2048; i++)
		for (j=0; j<6; j++) {
			cache_location[i][j] = 200;
		}
/*
	for (i=0; i<2048; i++)
		for (j=0; j<6; j++) {
			cache_location[i][j] = 0;
	}
	
	for (i=0; i<32; i++){
		for (j=0; j<25; j++){
			for (k=0; k<64; k++) {
				*(cache_page[i][j]+k*CACHE_LINE_SIZE/8) = (uint64_t)(cache_page[i][j+1]+k*CACHE_LINE_SIZE/8);
			}
		}
	}

*/

	return;
}

int CacheCheck(void) {
	int i, j;
	int val = 1;
	for (i=0; i<150; i++)
		for (j=0; j<32; j++)
			if (j != pfn_to_mfn(virt_to_pfn(cache_page[j][i]))%32)
				val = 0;
	return val;
}

void CacheFree(void) {
	int i;
	for (i=0; i<PAGE_PTR_NR; i++)
		__free_pages(ptr[i], PAGE_ORDER);
}

asmlinkage void sys_CacheSearch(int *result) {
	int i, j, k;
	int val;
	int counter = 0;
	uint8_t trial;
	uint8_t start = 105;
 	uint8_t end = 149;
	int num = 0;
	int index, offset;
	uint64_t *temp_addr;

	val = CacheCheck();
	if (val == 0) {
		printk("Buffer initialization fails, exit\n");
		return;
	}
	for (index = 0; index < 32; index ++) {
		for (offset = 0; offset < 64; offset ++) {

			for (i = 0; i<150; i++) 
				cache_order[i] = i;
			num = 0;

			for (trial = start; trial < end; trial ++ ) {
				for (i=0; i<trial; i++) 
					cache_timing[i] = 0;
				for (k=0; k<ROUND; k++) {
					update_order(trial);

					for (i = 0; i<trial; i++) {
						j = cache_order[i];
						if (j != -1)
							working_thread1(cache_page[index][j]+offset*CACHE_LINE_SIZE/8);
					}

					for (i = 0; i<trial; i++) {
						j = cache_order[i];
						if (j != -1)
							working_thread1(cache_page[index][j]+offset*CACHE_LINE_SIZE/8);
					}

					working_thread1(cache_page[index][trial]+offset*CACHE_LINE_SIZE/8);

					for (i = 0; i<trial; i++) {
						j = cache_order[i];
						if (j != -1)
							working_thread(cache_page[index][j]+offset*CACHE_LINE_SIZE/8, &cache_timing[j]);
					}
				}

				counter = 0;
				temp_addr = 0;
				for (i = 0; i<trial; i++) 
					if (cache_timing[i]/ROUND > THRESHOLD)
						counter ++;
				k = counter;
				if (counter >= 19) {
					k = 19;
					for (i = 0; i<trial; i++) {
						j = cache_order[i];
						if ((j != -1)&&(cache_timing[j]/ROUND > THRESHOLD)) {
							cache_order[i] = -1;
							if (k > 0) {
								if ((counter == 19)&&(k == 18)) 
									temp_addr = cache_page[index][j]+offset*CACHE_LINE_SIZE/8;

								*(cache_page[index][j]+offset*CACHE_LINE_SIZE/8) = (uint64_t)temp_addr;
								temp_addr = cache_page[index][j] + offset*CACHE_LINE_SIZE/8;
								k--;
							}
						}
					}
					*(cache_page[index][trial]+offset*CACHE_LINE_SIZE/8) = (uint64_t)temp_addr;
					cache_location[index*64+offset][num] = trial;
					cache_order[trial] = -1;
					
					num ++;
				}
				if (num == 6)
					break;

				for (i = 0; i<trial; i++) {
					j = cache_order[i];
					if ((j != -1)&&(cache_timing[j]/ROUND > THRESHOLD)) {
						cache_order[i] = -1;
					}
				}
			}
		}
	}

	num = 0;
	for (i=0; i<2048; i++)
		for (j=0; j<6; j++) 
			if (cache_location[i][j] == 200) {
				num ++; 
			}

	*result = num;

	return;
}


asmlinkage void sys_CacheClean(int mode, int time, unsigned long **timestamp, unsigned long *num, int cpu_id) {
	int i, j;
	unsigned long n = 0;
	int index, offset;
	unsigned long tsc;
	uint64_t *temp_addr;

	set_cpus_allowed_ptr(current, cpumask_of(cpu_id));

	tsc = jiffies + time * HZ;
	if (mode == 0) {
		while (time_before(jiffies, tsc)) {
			n++;
			for (i=cpu_id*1024; i<(cpu_id+1)*1024; i++) {
				index = i / 64;
				offset = i % 64;
				for (j=0; j<6; j++) {
					if (cache_location[i][j] != 200) {
						temp_addr = cache_page[index][cache_location[i][j]] + offset*CACHE_LINE_SIZE/8;
						working_thread2(temp_addr, &timestamp[i-cpu_id*1024][j]);
					}
				}
			}
		}
	}
	else {
		while (time_before(jiffies, tsc)) {
			n++;
			for (i=cpu_id*1024; i<(cpu_id+1)*1024; i++) {
				index = i / 64;
				offset = i % 64;
				for (j=0; j<6; j++) {
					if (cache_location[i][j] != 200) {
						temp_addr = cache_page[index][cache_location[i][j]] + offset*CACHE_LINE_SIZE/8;
						working_thread3(temp_addr);
					}
				}
			}
		}
	}
	*num = n;
	return;

}


int __init init_addsyscall(void) {
	printk("Module Init\n");
	my_sys_call_table = (void**) SYS_CALL_TABLE_ADDR;
	SetPageRW(virt_to_page(my_sys_call_table));
	position_collect1 = (uint64_t(*)(void))(my_sys_call_table[__NR_CacheSearch]);
	position_collect2 = (uint64_t(*)(void))(my_sys_call_table[__NR_CacheClean]);
	my_sys_call_table[__NR_CacheSearch] = sys_CacheSearch;
	my_sys_call_table[__NR_CacheClean] = sys_CacheClean;
	SetPageRO(virt_to_page(my_sys_call_table));
	CacheInit();
	return 0;
}

void __exit exit_addsyscall(void) {
	printk("Module Exit\n");
	SetPageRW(virt_to_page(my_sys_call_table));
	my_sys_call_table[__NR_CacheSearch] = position_collect1;
	my_sys_call_table[__NR_CacheClean] = position_collect2;
	SetPageRO(virt_to_page(my_sys_call_table));
	CacheFree();
	return;
}


module_init(init_addsyscall);
module_exit(exit_addsyscall);

MODULE_LICENSE("GPL");
