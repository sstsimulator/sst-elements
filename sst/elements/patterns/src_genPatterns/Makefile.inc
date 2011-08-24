# -*- Makefile -*-
#
#

genPatterns_SOURCES = \
	src_genPatterns/gen.c \
	src_genPatterns/gen.h \
	src_genPatterns/main.c \
	src_genPatterns/sst_gen.c \
	src_genPatterns/topo_mesh2D.c \
	src_genPatterns/sst_gen.h \
	src_genPatterns/topo_mesh2D.h \
	src_genPatterns/main.h

genPatternsV2_SOURCES = \
	src_genPatterns/gen.c \
	src_genPatterns/gen.h \
	src_genPatterns/main_v2.c \
	src_genPatterns/sst_gen_v2.c \
	src_genPatterns/topo_mesh2D.c \
	src_genPatterns/sst_gen_v2.h \
	src_genPatterns/topo_mesh2D.h \
	src_genPatterns/main.h



bin_PROGRAMS += genPatterns genPatternsV2
