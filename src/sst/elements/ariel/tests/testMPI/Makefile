CC := mpic++
API_DIR := ../../api
CFLAGS := -fopenmp -I$(API_DIR)
LDFLAGS := -L$(API_DIR) -larielapi
TARGETS := hello fakepin reduce

all: $(TARGETS)

%: %.cc libarielapi
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

libarielapi:
	CC=mpicc $(MAKE) -C $(API_DIR)

clean:
	rm -rf $(TARGETS)

.PHONY: clean libarielapi
