CXX=mipsel-linux-musl-gcc

all: sqrt-double sqrt-float

sqrt-double: sqrt-double.c
	$(CXX) -o sqrt-double -static sqrt-double.c

sqrt-float: sqrt-float.c
	$(CXX) -o sqrt-float -static sqrt-float.c

clean:
	rm sqrt-double sqrt-float
