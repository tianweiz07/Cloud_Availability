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

#define ROUND_NR 9000000

struct page *ptr[PAGE_PTR_NR];
int *list[PAGE_PTR_NR];

struct page *ptr_bk[PAGE_PTR_NR];
int *list_bk[PAGE_PTR_NR];

atomic_t flag;

unsigned long page_to_virt(struct page* page){
	return (unsigned long)phys_to_virt(page_to_phys(page));
}

void MemInit(void) {
	int i, j;
	for (i=0; i<PAGE_PTR_NR; i++){
		ptr[i] = alloc_pages(GFP_KERNEL, PAGE_ORDER);
		list[i] = (int *)page_to_virt(ptr[i]);
	}

	for (i=0; i<PAGE_PTR_NR; i++) {
		for (j=0; j<4096*1024/sizeof(int); j++) {
			if (i==(PAGE_PTR_NR-1))
				list[i][j] = 0;
			else
				list[i][j] = i+1;
		}
	}


	for (i=0; i<PAGE_PTR_NR; i++){
		ptr_bk[i] = alloc_pages(GFP_KERNEL, PAGE_ORDER);
		list_bk[i] = (int *)page_to_virt(ptr_bk[i]);
	}

	for (i=0; i<PAGE_PTR_NR; i++) {
		for (j=0; j<4096*1024/sizeof(int); j++) {
			if (i==(PAGE_PTR_NR-1))
				list_bk[i][j] = 0;
			else
				list_bk[i][j] = i+1;
		}
	}
	return;
}

int bk_access(void *argv) {
	int next;
	int i;
	set_cpus_allowed_ptr(current, cpumask_of(1));
	atomic_set(&flag, 1);
	while (atomic_read(&flag)!=2) {
		get_random_bytes(&next, sizeof(next));
		next = next % PAGE_PTR_NR;
		for (i=0; i<PAGE_PTR_NR; i++) {
			next = list[next][0];
		}
	}
	return 0;

}

int main_access(int offset) {
	int i, j;
	unsigned int next;
	for (i=0; i<ROUND_NR; i++) {
		get_random_bytes(&next, sizeof(next));
		next = next % PAGE_PTR_NR;
		for (j=0; j<PAGE_PTR_NR; j++) {
			next = list_bk[next][offset];
		}
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
	printk("Module Init\n");
	set_cpus_allowed_ptr(current, cpumask_of(0));
	MemInit();
	atomic_set(&flag, 0);
	kernel_thread(bk_access, NULL, 0);
	while (atomic_read(&flag)!=1)
		schedule();

	for (bit=6; bit<22; bit++) {
		offset = (1<<bit)/sizeof(int);
		main_access(offset);
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
