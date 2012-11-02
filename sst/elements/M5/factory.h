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
    Gem5Object_t* createObject( std::string name, std::string type, 
            SST::Params&  );
    
  private:     
    Gem5Object_t* createObject1( std::string name, std::string type, 
            SST::Params&  );
    Gem5Object_t* createObject2( std::string name, std::string type, 
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

#ifdef USE_MACOSX_DYLIB
    m_libgem5 = dlopen("libgem5_"LIBTYPE".dylib",RTLD_NOW);
#else
    m_libgem5 = dlopen("libgem5_"LIBTYPE".so",RTLD_NOW);
#endif

    if ( ! m_libgem5 ) {
        printf("Factory::Factory() %s\n",dlerror());
	printf("SST was unable to find a GEM5 library to use. This may have happened because GEM5 is not referenced in your LD_LIBRARY_PATH. Please see the SST documentation.");
        exit(-1);
    }
}

inline Factory::~Factory()
{
    dlclose(m_libm5C);
    dlclose(m_libgem5);
}

inline Gem5Object_t* Factory::createObject( std::string name, 
                std::string type, SST::Params& params )
{
    if ( type[0] != '+' ) {
        return createObject1( name, type, params );
    } else {
        return createObject2( name, type.substr(1), params );
    }
}

inline Gem5Object_t* Factory::createObject1( std::string name, 
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

    
    Gem5Object_t* obj = new Gem5Object_t;
    assert(obj);
    obj->memObject = (*ptr)( m_comp, name, params );
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

inline Gem5Object_t* Factory::createObject2( const std::string name, 
                std::string type, SST::Params& params )
{
    typedef Gem5Object_t* (*createObjFunc_t)( const char*, const xxx * );
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

    Gem5Object_t* obj = (*ptr)( name.c_str(), xxx );
    if ( ! obj ) {
        printf("Factory::Factory() failed to create %s\n", type.c_str() );
        exit(-1);
    }

    for ( int i=0; i < params.size(); i++ ){
        free( xxx[i].key );
        free( xxx[i].value );
    }

    free( xxx );
    return obj;
}

#endif
