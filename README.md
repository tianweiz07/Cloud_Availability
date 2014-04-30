Cloud_AVailability
==================

This file use the Inter Processor Interrupt (IPI) to perform the availability in the cloud

#include <linux/sched.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/smp.h>
#include <linux/irq_work.h>
#include <linux/tick.h>
#include <linux/init.h>
#include <linux/clocksource.h>
#include <linux/irqreturn.h>
#include <linux/linkage.h>

#include <asm/paravirt.h>
#include <asm/desc.h>
#include <asm/pgtable.h>
#include <asm/cpu.h>
#include <asm/xen/interface.h>
#include <asm/xen/hypercall.h>
#include <asm/page.h>

#include <xen/interface/xen.h>
#include <xen/interface/vcpu.h>
#include <xen/xen-ops.h>
#include <xen/xen.h>
#include <xen/page.h>
#include <xen/events.h>
#include <xen/hvc-console.h>




