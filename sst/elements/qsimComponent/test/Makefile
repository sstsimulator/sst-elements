all: test.xml test-app.tar state.4

test.xml: ../gen_sdl.sh
	N=4 STATE=`pwd`/state.4 APP=`pwd`/test-app.tar ../gen_sdl.sh > test.xml

## test-app.tar: test-app runme.sh
test-app.tar: runme.sh
	tar -cf test-app.tar test-app runme.sh

## test-app: test-app.c
##	gcc -O2 -m32 -static -o test-app test-app.c

state.4: state.4.bz2
	bunzip2 -k state.4.bz2

clean:
	rm -f test-app test-app.tar test.xml state.4
