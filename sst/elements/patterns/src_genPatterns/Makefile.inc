# -*- Makefile -*-
#
#

genPatterns_SOURCES = \
	src_genPatterns/gen.c \
	src_genPatterns/gen.h \
	src_genPatterns/main_v2.c \
	src_genPatterns/sst_gen_v2.c \
	src_genPatterns/sst_gen_v2.h \
	src_genPatterns/machine.c \
	src_genPatterns/machine.h \
	src_genPatterns/farlink.c \
	src_genPatterns/farlink.h \
	src_genPatterns/pattern.c \
	src_genPatterns/pattern.h \
	src_genPatterns/topo_mesh2D.c \
	src_genPatterns/topo_mesh2D.h \
	src_genPatterns/main.h

bin_PROGRAMS += genPatterns
