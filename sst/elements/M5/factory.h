#ifndef _factory_h
#define _factory_h

#include <sst/core/sdl.h>

#include <dlfcn.h>
#include <sim/sim_object.hh>

#include <debug.h>

class M5;

class Factory {
  public:
    Factory( M5* );
    ~Factory( );
    SimObject* createObject( std::string name, SST::SDL_Component& sdl );
    
  private:     
    void *m_dl;
    M5* m_comp;
};

typedef SimObject* (*createObjFunc_t)( void*, std::string, SST::Params&);

inline Factory::Factory( M5* comp ) :
    m_comp( comp )
{
    m_dl = dlopen("libm5C.so",RTLD_NOW);
    if ( ! m_dl ) {
        printf("Factory::Factory() %s\n",dlerror());
        exit(-1);
    }
}

inline Factory::~Factory()
{
    dlclose(m_dl);
}

inline SimObject* Factory::createObject( std::string name, 
                SST::SDL_Component& sdl )
{
    std:string tmp = "create_";
    tmp += sdl.type().c_str();
    DBGX(2,"type `%s`\n", tmp.c_str());
    createObjFunc_t ptr = (createObjFunc_t)(dlsym( m_dl, tmp.c_str() ));
    if ( ! ptr ) {
        printf("Factory::Factory() %s\n",dlerror());
        exit(-1);
    }

    SimObject* obj;
    obj = (*ptr)( m_comp, name, sdl.params );
    if ( ! obj ) {
        printf("Factory::Factory() failed to create %s\n",
                                sdl.type().c_str() );
        exit(-1);
    }

    return obj;
}

#endif
