#ifndef _factory_h
#define _factory_h

#include <sst/core/sdl.h>

#include <dlfcn.h>
#include <dll/gem5dll.hh>

#include <debug.h>

class M5;

class SimObject;

class Factory {
  public:
    Factory( M5* );
    ~Factory( );
    SimObject* createObject( std::string name, std::string type, 
            SST::Params&  );
    
  private:     
    SimObject* createObject1( std::string name, std::string type, 
            SST::Params&  );
    SimObject* createObject2( std::string name, std::string type, 
            SST::Params&  );
    void *m_libm5C;
    void *m_libgem5;
    M5* m_comp;
};

#define FAST "fast"
#define OPT  "opt" 
#define PROF "prof" 
#define DEBUG "debug"

inline Factory::Factory( M5* comp ) :
    m_comp( comp )
{
    m_libm5C = dlopen("libm5C.so",RTLD_NOW);
    if ( ! m_libm5C ) {
        printf("Factory::Factory() %s\n",dlerror());
        exit(-1);
    }
    m_libgem5 = dlopen("libgem5_"LIBTYPE".so",RTLD_NOW);
    if ( ! m_libgem5 ) {
        printf("Factory::Factory() %s\n",dlerror());
        exit(-1);
    }
}

inline Factory::~Factory()
{
    dlclose(m_libm5C);
    dlclose(m_libgem5);
}

inline SimObject* Factory::createObject( std::string name, 
                std::string type, SST::Params& params )
{
    if ( type[0] != '+' ) {
        return createObject1( name, type, params );
    } else {
        return createObject2( name, type.substr(1), params );
    }
}

inline SimObject* Factory::createObject1( std::string name, 
                std::string type, SST::Params& params )
{
    typedef SimObject* (*createObjFunc_t)( void*, std::string, SST::Params&);

    std::string tmp = "create_";
    tmp += type;
    DBGX(2,"type `%s`\n", tmp.c_str());
    createObjFunc_t ptr = (createObjFunc_t)(dlsym( m_libm5C, tmp.c_str() ));
    if ( ! ptr ) {
        printf("Factory::Factory() %s\n",dlerror());
        exit(-1);
    }

    SimObject* obj = (*ptr)( m_comp, name, params );
    if ( ! obj ) {
        printf("Factory::Factory() failed to create %s\n", type.c_str() );
        exit(-1);
    }

    return obj;
}

inline char * make_copy( const std::string & str ) 
{
    char * tmp = (char*) malloc( str.size() + 1 );
    assert(tmp);
    return strcpy( tmp, str.c_str() );
}

inline SimObject* Factory::createObject2( const std::string name, 
                std::string type, SST::Params& params )
{
    typedef void* (*createObjFunc_t)( const char*, const xxx * );
    std::string tmp = "Create";
    tmp += type;
    DBGX(2,"type `%s`\n", tmp.c_str());
    createObjFunc_t ptr = (createObjFunc_t)(dlsym( m_libgem5, tmp.c_str() ));
    if ( ! ptr ) {
        printf("Factory::Factory() %s\n",dlerror());
        exit(-1);
    }

    xxx_t *xxx = (xxx_t*) malloc( (params.size() + 1) * sizeof( *xxx ) );   

    SST::Params::iterator iter = params.begin();
    for ( int i=0; iter != params.end(); ++iter, i++ ){

        xxx[i].key   = make_copy( (*iter).first );
        xxx[i].value = make_copy( (*iter).second );
    } 
    xxx[params.size()].key   = NULL;  
    xxx[params.size()].value = NULL;

    void* obj = (*ptr)( name.c_str(), xxx );
    if ( ! obj ) {
        printf("Factory::Factory() failed to create %s\n", type.c_str() );
        exit(-1);
    }

    for ( int i=0; i < params.size(); i++ ){
        free( xxx[i].key );
        free( xxx[i].value );
    }

    free( xxx );
    return (SimObject*) obj;
}

#endif
