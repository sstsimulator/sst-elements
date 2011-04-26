#ifndef _Syscall_h
#define _Syscall_h

#include <dev/io_device.hh>

#include <debug.h>

using namespace std;
class M5;

struct SyscallParams : public DmaDeviceParams
{
    M5*  m5Comp; 
    Addr startAddr;
};

class Syscall : public DmaDevice 
{
    class DmaEvent : public ::Event
    {
      public:
        DmaEvent( Syscall* ptr) : m_ptr( ptr ) {}  

        void process() {
            m_ptr->process();
        }       
        const char *description() const {
            return "DMA event completed";
        }
        uint8_t *buf;  
        int64_t retval;
      private:
        Syscall* m_ptr;
    };

    class SyscallEvent : public ::Event
    {
      public:
        SyscallEvent( Syscall* ptr) : m_ptr( ptr ) {}  

        void process() {
            m_ptr->startSyscall();
        }       
        const char *description() const {
            return "Syscall event";
        }
      private:
        Syscall* m_ptr;
    };
  
  public:
    typedef SyscallParams Params;
    Syscall( const Params* p );
    virtual ~Syscall();

    virtual Tick write(Packet*);
    virtual Tick read(Packet*);
    virtual void addressRanges(AddrRangeList&); 

  private:
    void startSyscall( void );
    void finishSyscall( void );

    void process( void );

    void startOpen( Addr path );
    int64_t finishOpen( int oflag, mode_t );

    int64_t close( int );

    int64_t startRead( int, Addr, size_t );
    int64_t finishRead( int, size_t );

    int64_t startWrite( int, Addr, size_t );
    int64_t finishWrite( int, size_t );

    SST::Event::Handler<Syscall> m_barrierHandler;

    void barrierReturn( SST::Event* );

    uint64_t      m_mailbox[0x10];

    DmaEvent      m_dmaEvent;
    SyscallEvent  m_syscallEvent;
    uint64_t      m_startAddr;
    uint64_t      m_endAddr;

    M5*           m_comp;

    void foo( int64_t retval ) {
        DBGX(2,"retval %d\n",retval);
        m_mailbox[0] = retval;
        memset( m_mailbox + 1, 0, sizeof(m_mailbox) - sizeof(m_mailbox[0]));
    }
};

#endif
