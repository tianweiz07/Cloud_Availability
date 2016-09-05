all: libapr network_connect

libapr:
	make -C ./apr/ all
	
network_connect: network_connect.c
	gcc network_connect.c -o network_connect -I./apr/aprutil-build/include/apr-1/ -I./apr/apr-build/include/apr-1/ -L./apr/apr-build/lib/ -L./apr/aprutil-build/lib/ -lapr-1 -laprutil-1 -lm  -lpthread -D_REENTRANT -D_GNU_SOURCE -D_LARGEFILE64_SOURCE

clean:
	make -C ./apr/ clean
	rm -rf network_connect
