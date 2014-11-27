KDIR := /lib/modules/$(shell uname -r)/build
CC = gcc

obj-m += cache_prime.o

all: kernel user

kernel:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

user:
	$(CC) -o search_cache search_cache.c
	$(CC) -o clean_cache clean_cache.c

clean:
	rm -rf *.o *.ko *.mod.* *.cmd .module* modules* Module* .*.cmd .tmp* search_cache clean_cache
