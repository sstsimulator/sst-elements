# For VIRAM compiler:
VCC=vcc
# You can say "gmake REPORT=yes ..." to turn on vectorization reporting.
# Default is no reporting. Also see test-scripts/cc-output-filter.
REPORTFLAG=$(subst yes,-hreport=isv,$(REPORT))
VCFLAGS=-hrestrict=f $(REPORTFLAG)
VLIBS=-lm
# For R10000 compiler:
#CC=cc
#CFLAGS=-O
#LIBS=-lm
CFLAGS=-O3 -D__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__=1030 -U__LDBL_MANT_DIG__
INC = -I../../FE -I../
CC = gcc-3.3  $(CFLAGS) $(INC) $(LIB) -static -arch ppc
LIB = -L../allStatLibs
LIBS = -lc_static -lm_ppc -lgcc_static -lstdc++ -lmCommon_ppc


POINTER=pointer
POINTER_OBJS=pointer.o DISstressRNG.o
POINTER_VIRAMOBJS=$(subst .o,.vo,$(POINTER_OBJS))

UPDATE=update
UPDATE_OBJS=update.o DISstressRNG.o
UPDATE_VIRAMOBJS=$(subst .o,.vo,$(UPDATE_OBJS))

MATRIX=matrix
MATRIX_OBJS=matrix.o DISstressRNG.o
MATRIX_VIRAMOBJS=$(subst .o,.vo,$(MATRIX_OBJS))

FIELD=field
FIELD_OBJS=field.o DISstressRNG.o
FIELD_VIRAMOBJS=$(subst .o,.vo,$(FIELD_OBJS))

TRANSITIVE=transitive
TRANSITIVE_OBJS=transitive.o DISstressRNG.o
TRANSITIVE_VIRAMOBJS=$(subst .o,.vo,$(TRANSITIVE_OBJS))

NEIGHBOR=neighbor
NEIGHBOR_OBJS=neighbor.o DISstressRNG.o
NEIGHBOR_VIRAMOBJS=$(subst .o,.vo,$(NEIGHBOR_OBJS))

GUPS=gups_serial gups_indexserial gups_parallel
GUPS_OBJS=gups_serial.o gups_indexserial.o gups_parallel.o
GUPS_VIRAMOBJS=$(subst .o,.vo,$(GUPS_OBJS)) gups_skeleton.vo

MARKS=$(GUPS)
viram_MARKS=$(patsubst %,%.viram,$(MARKS))
r10k_MARKS=$(patsubst %,%.r10k,$(MARKS))
ALL_MARKS=$(sort $(viram_MARKS) $(r10k_MARKS))
ALL_OBJS=$(sort $(GUPS_OBJS) $(GUPS_VIRAMOBJS))

nonserial: gups_notserial gups_notserial_amo

gups_notserial: gups_notserial.o 
	$(CC) -o $@ $(LIB) $< $(LIBS)

gups_notserial_amo: gups_notserial_amo.o 
	$(CC) -o $@ $(LIB) $< $(LIBS)

gups_notserial.o: gups_notserial.c gups_sysheaders.h gups_types.h
	$(CC) $(CFLAGS) -funroll-loops -O3  -DRANDOM_IS_LFSR -o $@ -c $<

gups_notserial_amo.o: gups_notserial.c gups_sysheaders.h gups_types.h
	$(CC) $(CFLAGS) -DUSE_AMO -funroll-loops -O3  -DRANDOM_IS_LFSR -o $@ -c $<

gups_serial: gups_serial.o 
	$(CC) -o $@ $(LIB) $< $(LIBS)

gups_serial.o: gups_serial.c gups_sysheaders.h gups_types.h
	$(CC) $(CFLAGS) -O3 -UDO_RANDOMS_BEFOREHAND -DRANDOM_IS_LFSR -o $@ -c $<

all: $(ALL_MARKS)

$(POINTER).viram: $(POINTER_VIRAMOBJS)
	$(VCC) $(VCFLAGS) -o $@ $^ $(VLIBS)
$(POINTER).r10k: $(POINTER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(UPDATE).viram: $(UPDATE_VIRAMOBJS)
	$(VCC) $(VCFLAGS) -o $@ $^ $(VLIBS)
$(UPDATE).r10k: $(UPDATE_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(MATRIX).viram: $(MATRIX_VIRAMOBJS)
	$(VCC) $(VCFLAGS) -o $@ $^ $(VLIBS)
$(MATRIX).r10k: $(MATRIX_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(FIELD).viram: $(FIELD_VIRAMOBJS)
	$(VCC) $(VCFLAGS) -o $@ $^ $(VLIBS)
$(FIELD).r10k: $(FIELD_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(TRANSITIVE).viram: $(TRANSITIVE_VIRAMOBJS)
	$(VCC) $(VCFLAGS) -o $@ $^ $(VLIBS)
$(TRANSITIVE).r10k: $(TRANSITIVE_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(NEIGHBOR).viram: $(NEIGHBOR_VIRAMOBJS)
	$(VCC) $(VCFLAGS) -o $@ $^ $(VLIBS)
$(NEIGHBOR).r10k: $(NEIGHBOR_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

gups_parallel.r10k: gups_parallel.o
	$(CC) $(LDFLAGS) -o $@ $+ $(LIBS)

gups_parallel.viram: gups_parallel.vo
	$(VCC) $(LDFLAGS) -o $@ $+ $(LIBS)

gups_serial.r10k: gups_serial.o
	$(CC) $(LDFLAGS) -o $@ $+ $(LIBS)

gups_serial.viram: gups_serial.vo
	$(VCC) $(LDFLAGS) -o $@ $+ $(LIBS)

gups_indexserial.r10k: gups_indexserial.o
	$(CC) $(LDFLAGS) -o $@ $+ $(LIBS)

gups_indexserial.viram: gups_indexserial.vo
	$(VCC) $(LDFLAGS) -o $@ $+ $(LIBS)

gups_skeleton.viram: gups_skeleton.vo 
	$(VCC) $(LDFLAGS) -o $@ $+ optimized_asm/*.o $(LIBS)
	mv gups_skeleton.viram gups_parallel.viram

gups_parallel.o: gups_parallel.c gups_sysheaders.h gups_types.h
	$(CC) $(CFLAGS) -DDO_RANDOMS_BEFOREHAND -DRANDOM_IS_LFSR -o $@ -c $<

gups_parallel.vo: gups_parallel.c gups_sysheaders.h gups_types.h
	$(VCC) $(VCFLAGS) -DDO_RANDOMS_BEFOREHAND -DRANDOM_IS_LFSR -DGETTIMEOFDAY_IS_BROKEN -o $@ -c $<

gups_skeleton.vo: gups_skeleton.c gups_sysheaders.h gups_types.h
	$(VCC) $(VCFLAGS) -DDO_RANDOMS_BEFOREHAND -DRANDOM_IS_LFSR -DGETTIMEOFDAY_IS_BROKEN -o $@ -c $<

gups_parallel.s: gups_parallel.c gups_sysheaders.h gups_types.h
	$(VCC) $(VCFLAGS) -DDO_RANDOMS_BEFOREHAND -DRANDOM_IS_LFSR -DGETTIMEOFDAY_IS_BROKEN -S $<

#gups_serial.o: gups_serial.c gups_sysheaders.h gups_types.h
#	$(CC) $(CFLAGS) -UDO_RANDOMS_BEFOREHAND -o $@ -c $<

gups_serial.vo: gups_serial.c gups_sysheaders.h gups_types.h
	$(VCC) $(VCFLAGS) -UDO_RANDOMS_BEFOREHAND -DGETTIMEOFDAY_IS_BROKEN -o $@ -c $<

gups_indexserial.o: gups_serial.c gups_sysheaders.h gups_types.h
	$(CC) $(CFLAGS) -DDO_RANDOMS_BEFOREHAND -o $@ -c $<

gups_indexserial.vo: gups_serial.c gups_sysheaders.h gups_types.h
	$(VCC) $(VCFLAGS) -DDO_RANDOMS_BEFOREHAND -DGETTIMEOFDAY_IS_BROKEN -o $@ -c $<

clean:
	rm -f $(ALL_MARKS) $(ALL_OBJS) *~ core

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
%.vo: %.c
	$(VCC) $(VCFLAGS) -o $@ -c $<
%.s: %.c
	$(VCC) $(VCFLAGS) -S $<

DISstressRNG.o: DISstressRNG.c DISstressRNG.h
field.o: field.c DISstressRNG.h
matrix.o: matrix.c DISstressRNG.h
pointer.o: pointer.c DISstressRNG.h
transitive.o: transitive.c DISstressRNG.h
update.o: update.c DISstressRNG.h

