bin_PROGRAMS = stream stream_mlm
stream_SOURCES = stream.c
stream_mlm_SOURCES = stream_malloc.c

API_DIR = ../../../../api
AM_CPPFLAGS += -I$(API_DIR)
stream_LDADD = $(API_DIR)/libarielapi.so
stream_mlm_LDADD = $(API_DIR)/libarielapi.so
