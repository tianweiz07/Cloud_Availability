KDIR := /lib/modules/$(shell uname -r)/build
CC = gcc

obj-m += add_syscall.o

all: kernel user

kernel:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

user:
	$(CC) -o bus_lock_unaligned bus_lock_unaligned.c -lpthread
	$(CC) -o bus_lock_uncached bus_lock_uncached.c -lpthread
clean:
	rm -rf *.o *.ko *.mod.* *.cmd .module* modules* Module* .*.cmd .tmp* bus_lock_unaligned bus_lock_uncached
