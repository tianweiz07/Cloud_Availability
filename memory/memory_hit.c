#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/random.h>
#include <asm/io.h>

#ifndef __KERNEL__
#define __KERNEL__
#endif

// Cache configurations
#define PAGE_PTR_NR 16
#define PAGE_ORDER 10
#define PAGE_NR 1024

#define ROUND_NR 1000000

struct page *ptr[PAGE_PTR_NR];
uint64_t *list[PAGE_PTR_NR];

struct page *ptr_bk[PAGE_PTR_NR];
uint64_t *list_bk[PAGE_PTR_NR];

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

void MemInit(void) {
	int i, j;
	for (i=0; i<PAGE_PTR_NR; i++){
		ptr[i] = alloc_pages(GFP_KERNEL, PAGE_ORDER);
		list[i] = (uint64_t *)page_to_virt(ptr[i]);
	}

	for (i=0; i<PAGE_PTR_NR; i++) {
		for (j=0; j<4096*1024/sizeof(uint64_t); j++) {
			if (i==(PAGE_PTR_NR-1))
				list[i][j] = (uint64_t) (&list[0][j]);
			else
				list[i][j] = (uint64_t) (&list[i+1][j]);
		}
	}


	for (i=0; i<PAGE_PTR_NR; i++){
		ptr_bk[i] = alloc_pages(GFP_KERNEL, PAGE_ORDER);
		list_bk[i] = (uint64_t *)page_to_virt(ptr_bk[i]);
	}

	for (i=0; i<PAGE_PTR_NR; i++) {
		for (j=0; j<4096*1024/sizeof(uint64_t); j++) {
			if (i==(PAGE_PTR_NR-1))
				list_bk[i][j] = (uint64_t) (&list[0][j]);
			else
				list_bk[i][j] = (uint64_t) (&list[i+1][j]);
		}
	}
	return;
}

int bk_access(void *argv) {
	volatile uint64_t next = list[0][0];
	set_cpus_allowed_ptr(current, cpumask_of(1));
	atomic_set(&flag, 1);
	while (atomic_read(&flag)!=2) {
		next = *(uint64_t *)next;
	}
	return 0;

}

int main_access(int offset) {
	int i;
	volatile uint64_t next = list[0][offset];
	for (i=0; i<ROUND_NR; i++) {
		next = *(uint64_t *)next;
	}
	return 0;
}

void MemFree(void) {
	int i;
	for (i=0; i<PAGE_PTR_NR; i++)
		__free_pages(ptr[i], PAGE_ORDER);
	for (i=0; i<PAGE_PTR_NR; i++)
		__free_pages(ptr_bk[i], PAGE_ORDER);
}

int __init init_addsyscall(void) {
	int offset, bit;
	uint64_t tsc;
	printk("Module Init\n");
	set_cpus_allowed_ptr(current, cpumask_of(0));
	MemInit();
	atomic_set(&flag, 0);
	kernel_thread(bk_access, NULL, 0);
	while (atomic_read(&flag)!=1)
		schedule();

	for (bit=6; bit<22; bit++) {
		offset = (1<<bit)/sizeof(uint64_t);
		tsc = rdtsc();
		main_access(offset);
		printk("BIT %d: %llu\n", bit, rdtsc()-tsc);
	}
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
