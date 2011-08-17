

#ifndef ARBITERCANROUTE_H_
#define ARBITERCANROUTE_H_


#include <omnetpp.h>
#include "messages_m.h"

#include "NetworkAddress.h"

#include <map>

using namespace std;

class ArbiterCanRoute {

public:

	virtual int route(ArbiterRequestMsg* rmsg) = 0;

	NetworkAddress* myAddr;

	int level;

	virtual int getUpPort(ArbiterRequestMsg* rmsg, int lev);
	virtual int getDownPort(ArbiterRequestMsg* rmsg, int lev);

	int numPorts;



};


#endif
