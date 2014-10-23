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

struct page *ptr[PAGE_PTR_NR];

unsigned long page_to_virt(struct page* page){
	return (unsigned long)phys_to_virt(page_to_phys(page));
}

void MemInit(void) {
	int i;
	for (i=0; i<PAGE_PTR_NR; i++){
		ptr[i] = alloc_pages(GFP_KERNEL, PAGE_ORDER);
		printk("%lu\n", ((unsigned long)page_to_phys(ptr[i]))%(4096*1024));
	}
	return;
}

void MemFree(void) {
	int i;
	for (i=0; i<PAGE_PTR_NR; i++)
		__free_pages(ptr[i], PAGE_ORDER);
}

int __init init_addsyscall(void) {
	printk("Module Init\n");
	MemInit();
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
