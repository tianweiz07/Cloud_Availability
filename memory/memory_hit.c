#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/random.h>
#include <asm/io.h>

#ifndef __KERNEL__
#define __KERNEL__
#endif

// Cache configurations
#define PAGE_ORDER 10
#define PAGE_NR 1024

#define ROUND_NR 10000000

struct page *ptr;
uint64_t *list;

struct page *ptr_bk;
uint64_t *list_bk;

atomic_t flag;

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

static noinline void cache_flush(uint64_t *address) {
	__asm__ volatile("clflush (%0)"
			 :
			 :"r"(address)
			 :"memory"
			);
	return;
}

static noinline void single_access(uint64_t *addr, uint64_t value)
{
        __asm__ volatile(
		"mov %0, %%r9\n\t"
		"mov %1, %%r10\n\t"
		"mov %%r10, (%%r9)\n\t"
		"lfence\n\t"
                :
                :"r"(addr), "r"(value)
                :
                );
        return;
}

void MemInit(void) {
	ptr = alloc_pages(GFP_KERNEL, PAGE_ORDER);
	list = (uint64_t *)page_to_virt(ptr);

	ptr_bk = alloc_pages(GFP_KERNEL, PAGE_ORDER);
	list_bk = (uint64_t *)page_to_virt(ptr_bk);

	return;
}

int bk_access(void *argv) {
	uint64_t *next;
	set_cpus_allowed_ptr(current, cpumask_of(1));
	atomic_set(&flag, 1);
	while (atomic_read(&flag)!=2) {
		next = &list_bk[0];
		cache_flush(next);
		single_access(next, 0x9);
	}
	return 0;

}

int main_access(int offset) {
	int i;
	uint64_t *next;
	for (i=0; i<ROUND_NR; i++) {
		next = &list[offset];
		cache_flush(next);
		single_access(next, 0x9);
	}
	return 0;
}

void MemFree(void) {
	__free_pages(ptr, PAGE_ORDER);
	__free_pages(ptr_bk, PAGE_ORDER);
}

int __init init_addsyscall(void) {
	int offset, bit;
//	int bit1;
//	int bank_index[4];
//	int access_index[16];
	uint64_t tsc;
	printk("Module Init\n");
	set_cpus_allowed_ptr(current, cpumask_of(0));
	MemInit();
	atomic_set(&flag, 0);
	kernel_thread(bk_access, NULL, 0);
	while (atomic_read(&flag)!=1)
		schedule();
/*************** Step 1: identify the bank bits *****************/

	for (bit=6; bit<22; bit++) {
		offset = (1<<bit)/sizeof(uint64_t);
		tsc = rdtsc();
		main_access(offset);
		printk("BIT %d: %llu\n", bit, rdtsc()-tsc);
	}

/*************** Step 2: identify xor **************************/

//	bank_index[0] = 13;
//	bank_index[1] = 14;
//	bank_index[2] = 15;
//	bank_index[3] = 17;
//	for (i=0; i<16; i++) {
//		access_index[i] = (((i>>3)&0x1)<<bank_index[3])/sizeof(uint64_t) +
//				  (((i>>2)&0x1)<<bank_index[2])/sizeof(uint64_t) +
//				  (((i>>1)&0x1)<<bank_index[1])/sizeof(uint64_t) +
//				  (((i>>0)&0x1)<<bank_index[0])/sizeof(uint64_t);
//}
//      for (bit=0; bit<5; bit++) {
//              for (bit1=bit+1; bit1<6; bit1++) {
//                      offset = ((1<<bank_index[bit]) + (1<<bank_index[bit1]))/sizeof(uint64_t);
//                      tsc = rdtsc();
//                      main_access(offset);
//                      printk("BIT %d XOR %d: %llu\n", bank_index[bit], bank_index[bit1], rdtsc()-tsc);
//              }
//      }

/************** Step 3: perform the attacks *******************/

//	tsc = jiffies + time * HZ;
//	while (time_before(jiffies, tsc)) {
//		for (bit=0; bit<16; j++) {
//			cache_flush(&list[access_index[j]]);
//			single_access(&list[access_index[j]], 0x9);
//		}
//	}

	atomic_set(&flag, 2);
	return 0;
}

void __exit exit_addsyscall(void) {
	printk("Module Exit\n");
	MemFree();
	return;
}

module_init(init_addsyscall);
module_exit(exit_addsyscall);

MODULE_LICENSE("GPL");
