#include <linux/module.h>
#include <linux/cpumask.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <asm/apic.h>
#include <linux/jiffies.h>


static int __init cpu_exe_init(void){
	int num = 0;
	int exe_cpu = 1;

	unsigned long current_time;
	unsigned long next_time;
	
	printk("entering cpu_exe module\n");

	set_cpus_allowed_ptr(current, get_cpu_mask(exe_cpu));

	current_time = jiffies;
	next_time = current_time + 600*HZ;

	while (time_before(jiffies, next_time)) {
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
