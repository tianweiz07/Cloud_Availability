KDIR := /lib/modules/$(shell uname -r)/build
CC = gcc

obj-m += memory_hit.o

all: kernel user

kernel:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

user:
	$(CC) -o mem_user mem_user.c

clean:
	rm -rf *.o *.ko *.mod.* *.cmd .module* modules* Module* .*.cmd .tmp* test
