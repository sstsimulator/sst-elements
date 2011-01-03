#ifndef _dummyPhysicalMemory_h
#define _dummyPhysicalMemory_h

#include <mem/physical.hh>

#include <debug.h>

using namespace std;

class DummyPhysicalMemory : public PhysicalMemory {

    class FuncPort : public Port 
    {
      public:
        FuncPort( const string &name, DummyPhysicalMemory* obj ) :
            Port( name, obj ),
            m_obj( obj )
        {
        } 
      protected:
        virtual bool recvTiming(PacketPtr pkt) 
        { cerr << WHERE <<  "abort" << endl; exit(1); }

        virtual Tick recvAtomic(PacketPtr pkt) 
        { cerr << WHERE <<  "abort" << endl; exit(1); }

        virtual void recvFunctional(PacketPtr pkt) { 
            m_obj->recvFunctional( pkt );
        }

        virtual void recvRetry() { 
            m_obj->recvRetry();
        }

        virtual void recvStatusChange(Status status) { }
      private:
        DummyPhysicalMemory* m_obj;
    }; 

  public:
    typedef PhysicalMemoryParams Params;
    DummyPhysicalMemory( const Params* p ) :
        PhysicalMemory(p), m_funcPort( NULL ), m_realPhysmem( NULL) {
    }    

    virtual ~DummyPhysicalMemory()
    {
        if ( m_funcPort ) delete m_funcPort;
    }

    PhysicalMemory* m_realPhysmem;

    void init() {}

    void recvFunctional(PacketPtr pkt) { 
        if ( m_realPhysmem ) { 
            m_realPhysmem->doFunctionalAccess( pkt );
        }
    }

    void recvRetry( ) { 
        if ( m_realPhysmem ) { 
            m_realPhysmem->recvRetry( );
        }
    }

    virtual Port * getPort(const string &if_name, int idx = -1) {
        DBGX( 1, "name=`%s` idx=%d\n", if_name.c_str(),idx);
        if ( if_name == "functional") {
            if ( m_funcPort ) {
                { cerr << WHERE <<  "abort" << endl; exit(1); }
            }
            return m_funcPort = 
                        new FuncPort(csprintf("%s-functional", name()), this);
        }

        return NULL;
    } 

  private:
    DummyPhysicalMemory( const Params& p );
    const DummyPhysicalMemory &operator=( const Params& p );

    FuncPort*   m_funcPort;
};

static inline DummyPhysicalMemory* 
        create_DummyPhysicalMemory( string name, Addr start, Addr end )
{
    DummyPhysicalMemory::Params* memP   = new DummyPhysicalMemory::Params;

    DBGC(1,"%s start=%#lx end=%lx\n", name.c_str(), start, end );
    memP->name = name;
    memP->range.start = start;
    memP->range.end = end;

    return new DummyPhysicalMemory( memP );
}
#endif
