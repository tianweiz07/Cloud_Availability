#include <linux/module.h>
#include <linux/cpumask.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <asm/apic.h>
#include <linux/jiffies.h>


static int __init cpu_exe_init(void){
	int num = 0;
	
	printk("entering cpu_exe module\n");

	while (1) {
		num ++;
		num --;
	}

    return 0;
}

static void __exit cpu_exe_exit(void){
    printk("leaving cpu_exe module\n");
}

module_init(cpu_exe_init); 
module_exit(cpu_exe_exit);

MODULE_LICENSE("GPL");
