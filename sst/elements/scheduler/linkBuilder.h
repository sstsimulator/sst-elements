
#ifndef _LINKBUILDER_H
#define _LINKBUILDER_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include "Job.h"

#include <string>

enum linkTypes { PARENT, CHILD };


class linkChanger {
  public:
    virtual void addLink( SST::Link * link, enum linkTypes type ) = 0;
    virtual void rmLink( SST::Link * link, enum linkTypes type ) = 0;
    virtual void disconnectYourself() = 0;
    virtual std::string getType() = 0;  // should describe the class of component (CPU, mem, NIC, etc)
    virtual std::string getID() = 0;    // should describe the exact component uniquely within each class (CPU1, CPU2, CPU3, NIC1, NIC2, NIC3, etc)
    virtual ~linkChanger() {};
};


class linkBuilder : public SST::Component {
  public:
    linkBuilder( SST::ComponentId_t id, SST::Component::Params_t & params );
    void connectGraph( Job * job );
    void disconnectGraph( Job * job );

  private:
    void initNodePtrRequests( SST::Event * event );
    void handleNewNodePtr( SST::Event * event );

    typedef std::map<std::string, linkChanger *> nodeMap;
    typedef std::map<std::string, nodeMap> nodeTypeMap;

    nodeTypeMap nodes;
      // node pointers are indexed by Type and then by ID

    SST::Link * selfLink;
    std::vector<SST::Link *> nodeLinks;
};

#endif

