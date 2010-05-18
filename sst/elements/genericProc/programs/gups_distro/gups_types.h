
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
#if defined(_MIPS_SZLONG) && (_MIPS_SZLONG == 64)
typedef unsigned long uint64;
#else
typedef unsigned long long uint64;
#endif

