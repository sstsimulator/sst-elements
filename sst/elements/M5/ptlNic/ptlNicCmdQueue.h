#include <nicCmdQueue.h>
#include <sys/syscall.h>

#define SYS_foo  443

int initNicCmdQueue()
{
    char* tmp = getenv("NIC_CMD_QUEUE_ADDR");
    printf("`%s`\n",tmp);
    unsigned long addr = 0;
    if ( tmp )
        addr = strtoul(tmp,NULL,16);

    m_rtrRegs  = (RtrRegs*)syscall( SYS_foo, addr, sizeof( nicCmdQueue_t) );
    printf("%#lx\n",m_rtrRegs);
}
