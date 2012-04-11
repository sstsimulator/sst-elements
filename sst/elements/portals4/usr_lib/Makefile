
all:
	make -C nic
	make -C api 
	make -C runtime 

clean:
	make clean -C nic
	make clean -C api 
	make clean -C runtime 

install:
	test -d include || mkdir include 
	test -d lib || mkdir lib
	make install -C runtime 
	make install -C nic
	make install -C api 
