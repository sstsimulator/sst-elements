API_DIR = ../../../../api

all-local: stream stream_mlm

clean-local:
	rm -f stream stream_mlm

stream:
	$(CXX) stream.c -o stream -I$(API_DIR) -L$(API_DIR) -larielapi

stream_mlm:
	$(CXX) stream_malloc.c -o stream_mlm -I$(API_DIR) -L$(API_DIR) -larielapi
