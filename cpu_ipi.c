#include <linux/module.h>
#include <linux/cpumask.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <asm/apic.h>
#include <linux/jiffies.h>


static int __init cpu_ipi_init(void){
	int total_cpus;
	int attacker_cpu;
	int victim_cpu;
	
	unsigned long current_time;
	unsigned long next_time;

	printk("entering cpu_ipi module\n");

	total_cpus = num_online_cpus();
	printk("total_cpus: %d\n", total_cpus);

	attacker_cpu = task_cpu(current);
	printk("attacker_cpu: %d\n", attacker_cpu);

	victim_cpu = (attacker_cpu + 1) % total_cpus;
	printk("victim_cpu: %d\n", victim_cpu);	
	current_time = jiffies;
	next_time = current_time + 10*HZ;
	printk("current_time: %lu\n", current_time);	
	printk("next_time: %lu\n", next_time);	
	while (time_before(jiffies, next_time)) {
		mdelay(1000);
		apic->send_IPI_mask(get_cpu_mask(victim_cpu), RESCHEDULE_VECTOR);
	}


//	set_cpus_allowed_ptr(current, get_cpu_mask(new_cpu));

//	cur_cpu = task_cpu(current);
//	printk("After migration, cur_cpu: %d\n", cur_cpu);
	

    return 0;
}

static void __exit cpu_ipi_exit(void){
    printk("leaving cpu_ipi module\n");
}

module_init(cpu_ipi_init); 
module_exit(cpu_ipi_exit);

MODULE_LICENSE("GPL");
