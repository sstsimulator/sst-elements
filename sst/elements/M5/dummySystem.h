#ifndef _dummySystem_h
#define _dummySystem_h

#include <sim/system.hh>
#include <dummyThreadContext.h>

#include <debug.h>

using namespace std;

class DummySystem : public System {

  public:
    typedef SystemParams Params;
    DummySystem( Params *p ) : System(p) {}
    ~DummySystem() {}

    ThreadContext *getThreadContext(ThreadID tid) { 
        // NOTE: we only have one threadContext 
        // given the thread context does nothing this works 
        return& m_threadContext; 
    }

  private:
    DummyThreadContext m_threadContext;
};

static inline DummySystem* create_DummySystem( string name, 
                    PhysicalMemory* physmem,
                    Enums::MemoryMode mem_mode) 
{
    
    DummySystem::Params& params = *new DummySystem::Params;
    
    DBGC( 1, "%s\n", name.c_str());

    params.name = name;
    params.physmem = physmem;
    params.mem_mode = mem_mode;

    return new DummySystem( &params );
}

#endif
