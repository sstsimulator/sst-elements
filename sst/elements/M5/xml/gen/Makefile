
SRCS = \
	genBus.c\
	genCache.c\
	genCpu.c\
	genMain.c\
	genBusParams.c\
	genCacheParams.c\
	genCpuParams.c\
	genPhysmem.c

OBJS = ${SRCS:.c=.o}

APPS = genXML

all: ${APPS} 

CFLAGS = -Wall

genXML: ${OBJS}
	${CC} ${OBJS} -o $@

clean:
	rm ${OBJS} ${APPS}
