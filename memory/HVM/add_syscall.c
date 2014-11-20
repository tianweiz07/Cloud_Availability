#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/random.h>
#include <asm/io.h>
#include <asm/cacheflush.h>

#ifndef __KERNEL__
#define __KERNEL__
#endif

static uint64_t SYS_CALL_TABLE_ADDR = 0xffffffff81801420;
#define __NR_UnCached 188

void **my_sys_call_table;
static uint64_t (*position_collect)(void);

static void disable_page_protection(void) {
    unsigned long value;
    asm volatile("mov %%cr0,%0" : "=r" (value));
    if (value & 0x00010000) {
            value &= ~0x00010000;
            asm volatile("mov %0,%%cr0": : "r" (value));
    }
}

static void enable_page_protection(void) {
    unsigned long value;
    asm volatile("mov %%cr0,%0" : "=r" (value));
    if (!(value & 0x00010000)) {
            value |= 0x00010000;
            asm volatile("mov %0,%%cr0": : "r" (value));
    }
}

pte_t * lookup_userspace_address(struct mm_struct *mm, unsigned long address, unsigned int *level)
{
        pgd_t *pgd;
        pud_t *pud;
        pmd_t *pmd;
        pte_t *pte;

	*level = PG_LEVEL_NONE;

        pgd = pgd_offset(mm, address);
        if(!pgd_present(*pgd))
                return NULL;

        pud = pud_offset(pgd, address);
        if (!pud_present(*pud))
                return NULL;

	*level = PG_LEVEL_1G;
        if (pud_large(*pud) || !pud_present(*pud))
                return (pte_t *)pud;

        pmd = pmd_offset(pud, address);
        if (!pmd_present(*pmd))
                return NULL;

        *level = PG_LEVEL_2M;
        if (pmd_large(*pmd) || !pmd_present(*pmd))
                return (pte_t *)pmd;

        *level = PG_LEVEL_4K;
        pte = pte_offset_map(pmd, address);
	return pte;
}

asmlinkage void sys_UnCached(unsigned long vir_addr) {
	pte_t *p_pte;
	unsigned int level;
	p_pte = lookup_userspace_address(get_current()->active_mm, vir_addr, &level);
	if (pte_present(*p_pte)) {
     	        set_pte(p_pte, pte_set_flags(*p_pte, _PAGE_PCD));
	}

	return;
}

int __init init_addsyscall(void) {
        my_sys_call_table = (void**) SYS_CALL_TABLE_ADDR;
	disable_page_protection();
        position_collect = (uint64_t(*)(void))(my_sys_call_table[__NR_UnCached]);
        my_sys_call_table[__NR_UnCached] = sys_UnCached;
	enable_page_protection();
	return 0;
}

void __exit exit_addsyscall(void) {
	printk("Module Exit\n");
	disable_page_protection();
        my_sys_call_table[__NR_UnCached] = position_collect;
	enable_page_protection();
	return;
}

module_init(init_addsyscall);
module_exit(exit_addsyscall);

MODULE_LICENSE("GPL");
