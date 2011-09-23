#ifndef __m5_syscall_h
#define __m5_syscall_h

#ifdef __alpha__  
#define SYS_virt2phys 442
#define SYS_mmap_dev  443   
#define SYS_fdmap     444   
#elif __x86_64__
#define SYS_virt2phys 273 
#define SYS_mmap_dev  274 
#define SYS_fdmap     275 
#elif __sparc__
#define SYS_virt2phys 284
#define SYS_mmap_dev  285
#else
    #error "What's your cpu?"
#endif


extern long _m5_syscall( long number, long arg0, long arg1, long arg2 );

#endif
