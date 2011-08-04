
#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/component.h>
#include <sst/core/configGraph.h>

#include <util.h>
#include <python/swig/pyobject.hh>
#include <factory.h>

struct LinkInfo {
    std::string compName;
    std::string portName;
    int portNum;
};

typedef std::pair< LinkInfo, LinkInfo>      link_t;
typedef std::map< std::string, link_t >     linkMap_t;

static void createLinkMap( linkMap_t&, SST::SDL_CompMap_t& );
static void printLinkMap( linkMap_t& );
static void connectAll( objectMap_t&, linkMap_t& );
static void createCompLinks( linkMap_t&, std::string, SST::SDL_links_t& );


objectMap_t buildConfigV1( M5* comp, std::string name, std::string configFile, SST::Params& params )
{
    objectMap_t     objectMap;
    linkMap_t       linkMap;
    SST::SDL_CompMap_t   sdlMap;
    
    DBGC( 2, "name=`%s` file=`%s`\n", name.c_str(), configFile.c_str() );

    xml_parse( configFile.c_str(),sdlMap );

    createLinkMap( linkMap, sdlMap );

    Factory factory( comp );

    SST::SDL_CompMap_t::iterator iter;
    for ( iter = sdlMap.begin(); iter != sdlMap.end(); ++iter ) {

        SST::Params tmp = params.find_prefix_params( iter->first + "." );
        (*iter->second).params.insert( tmp.begin(), tmp.end() );

        objectMap[ iter->first ]  = factory.createObject( 
                        name + "." + iter->first, *iter->second );
    }
   
    //printLinkMap( linkMap );
    connectAll( objectMap, linkMap );
    return objectMap;
}

objectMap_t buildConfigV2( M5* comp, std::string name, std::string configFile, SST::Params& params )
{
    objectMap_t     objectMap;
    linkMap_t       linkMap;
    SST::ConfigGraph graph;

    DBGC( 2, "name=`%s` file=`%s`\n", name.c_str(), configFile.c_str() );

    xml_parse( configFile.c_str(), graph );
    //graph.print_graph(std::cout);

    SST::ConfigLinkMap_t::iterator lmIter;

    for ( lmIter = graph.links.begin(); lmIter != graph.links.end(); ++lmIter ) {
        SST::ConfigLink& tmp = *(*lmIter).second;
        DBGC(2,"key=%s name=%s\n",(*lmIter).first.c_str(), tmp.name.c_str());

        LinkInfo l0,l1;
        l0.compName = tmp.component[0]->name.c_str();
        l1.compName = tmp.component[1]->name.c_str();

        l0.portName = tmp.port[0];
        l1.portName = tmp.port[1];

        l0.portNum= -1;
        l1.portNum = -1;

        linkMap[ tmp.name  ] =  std::make_pair( l0, l1 ); 
    } 
    //printLinkMap( linkMap );

    Factory factory( comp );

    SST::ConfigComponentMap_t::iterator iter; 

    for ( iter = graph.comps.begin(); iter != graph.comps.end(); ++iter ) {
        SST::ConfigComponent& tmp = *(*iter).second;
        DBGC(2,"id=%d %s %s\n",(*iter).first, tmp.name.c_str(), tmp.type.c_str());
        SST::SDL_Component sdl( tmp.type) ;
        sdl.params = tmp.params;

        SST::Params tmpParams = params.find_prefix_params( tmp.name + "." );
        sdl.params.insert( tmpParams.begin(), tmpParams.end() );

        objectMap[ tmp.name.c_str() ]  = factory.createObject( 
                        name + "." + tmp.name, sdl );
    }

    connectAll( objectMap, linkMap );

    return objectMap;
}

objectMap_t buildConfig( M5* comp, std::string name, std::string configFile, SST::Params& params )
{
    std::ifstream file( configFile.c_str() );

    assert( file.is_open() );

    std::string line;
    getline(file,line);

    size_t pos1 = line.find_first_of( '"' ) + 1;
    size_t pos2 = line.find_last_of( '"' );
    
    if( line.compare( pos1, pos2-pos1, "1.0" ) == 0 ) {
        return buildConfigV1( comp, name, configFile, params );
    } else if( line.compare( pos1, pos2-pos1, "2.0" ) == 0 ) {
        return buildConfigV2( comp, name, configFile, params );
    } else {
        assert( false );
    }
}

void createLinkMap( linkMap_t& linkMap, SST::SDL_CompMap_t& map )
{
    SST::SDL_CompMap_t::iterator iter;    
    for ( iter = map.begin(); iter != map.end(); ++iter ) {
        DBGC( 2,"comp[ name=%s type=%s ]\n", iter->first.c_str(),
                                        iter->second->type().c_str());
        SST::SDL_Component& comp = *iter->second;
        createCompLinks( linkMap, iter->first, comp.links );
    }
}

void printLinkMap( linkMap_t& map  )
{
    for ( linkMap_t::iterator iter=map.begin(); iter != map.end(); ++iter ) {
        printf("link=%s %s<->%s\n",iter->first.c_str(),
            iter->second.first.compName.c_str(), 
            iter->second.second.compName.c_str());
    } 
}

void connectAll( objectMap_t& objMap, linkMap_t& linkMap  )
{
    linkMap_t::iterator iter; 
    for ( iter=linkMap.begin(); iter != linkMap.end(); ++iter ) {
        DBGC( 2,"connecting %s [%s %s %d]<->[%s %s %d]\n",iter->first.c_str(),
            iter->second.first.compName.c_str(), 
            iter->second.first.portName.c_str(), 
            iter->second.first.portNum, 
            iter->second.second.compName.c_str(),
            iter->second.second.portName.c_str(),
            iter->second.second.portNum);

        if ( ! connectPorts( objMap[iter->second.first.compName],
                        iter->second.first.portName,
                        iter->second.first.portNum,
                        objMap[iter->second.second.compName],
                        iter->second.second.portName,
                        iter->second.second.portNum) ) 
        {
            printf("connectPorts failed\n");
            exit(1);
        }
    } 
}

void createCompLinks( linkMap_t& linkMap, std::string name,
                                            SST::SDL_links_t& links )
{
    SST::SDL_links_t::iterator iter;    
    for ( iter = links.begin(); iter != links.end(); ++iter ) {
        DBGC( 3,"link=%s %s\n", name.c_str(), iter->first.c_str() );

        if ( linkMap.find( iter->first ) == linkMap.end() ) {
            link_t link;
            linkMap[ iter->first ] = link;
        } 
        if ( linkMap[ iter->first ].first.compName.empty() ) {
            linkMap[ iter->first ].first.compName = name;
            linkMap[ iter->first ].first.portName = 
                                iter->second->params["name"];
            linkMap[ iter->first ].first.portNum =
                                atoi( iter->second->params["num"].c_str() );
        } else if ( linkMap[ iter->first ].second.compName.empty() ) {
            linkMap[ iter->first ].second.compName = name;
            linkMap[ iter->first ].second.portName = 
                                iter->second->params["name"];
            linkMap[ iter->first ].second.portNum =
                                atoi( iter->second->params["num"].c_str() );
        } else {
            printf("createCompLinks fatal\n");
            exit(0);
        }
    }
}
