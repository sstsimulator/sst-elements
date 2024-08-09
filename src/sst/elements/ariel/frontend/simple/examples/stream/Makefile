API_DIR = ../../../../api

all: stream stream_mlm

clean:
	rm -f stream stream_mlm

stream:
	g++ stream.c -o stream $(CFLAGS_LOCAL)

stream_mlm:
	g++ stream_malloc.c -o stream_mlm $(CFLAGS_LOCAL) -I$(API_DIR) -L. -L$(API_DIR) -larielapi
