APR=apr-1.4.6
APRUTIL=apr-util-1.4.1

all:buildapr

buildapr:
	tar vxf $(APR).tar.gz
	make -C $(APR) -f Makefile_apr
	tar vxf $(APRUTIL).tar.gz
	make -C $(APRUTIL) -f Makefile_aprutil

clean:
	rm -rf apr-build
	rm -rf aprutil-build
	rm -rf $(APR)
	rm -rf $(APRUTIL)
