
#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/component.h>

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

objectMap_t buildConfig( M5* comp, std::string name, std::string configFile )
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

        objectMap[ iter->first ]  = factory.createObject( 
                        name + "." + iter->first, *iter->second );
    }
   
    //printLinkMap( linkMap );
    connectAll( objectMap, linkMap );
    return objectMap;
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
