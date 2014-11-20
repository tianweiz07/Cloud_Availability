KDIR := /lib/modules/$(shell uname -r)/build
CC = gcc

obj-m += memory_hit.o

all: kernel

kernel:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -rf *.o *.ko *.mod.* *.cmd .module* modules* Module* .*.cmd .tmp*
