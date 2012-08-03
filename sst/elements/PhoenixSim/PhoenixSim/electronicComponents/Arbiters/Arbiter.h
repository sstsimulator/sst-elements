/*
 * Arbiter.h
 *
 *  Created on: Mar 9, 2009
 *      Author: SolidSnake
 */

#ifndef ARBITER_H_
#define ARBITER_H_

#include "messages_m.h"

#include "NetworkAddress.h"
#include "AddressTranslator.h"
#include "ArbiterCanRoute.h"

#include "orion/ORION_Arbiter.h"
#include "VirtualChannelControl.h"

//#include "Processor.h"

using namespace std;


#define NA -1

class Arbiter : public ArbiterCanRoute {
public:
	Arbiter();
	virtual ~Arbiter();

	virtual void init(string id, int lev, int x, int y, int vc, int ports, int numpse, int buff_size,  string n);

	virtual void newRequest(ArbiterRequestMsg* msg, int port);
	virtual list<RouterCntrlMsg*>* grant(double time);
	void creditsUp(int port, int vc, int size);
	void unblock(int in);
	void setPowerModels(map<int, ORION_Arbiter*> p);

	bool serveAgain();

	static AddressTranslator* translator;


	int maxGrants;
	int variant;

	map<int, int> xbar;
	map<int, int> xbarIn;

	map<int, list<ArbiterRequestMsg*>* > reqs;

protected:

	//virtuals
	virtual list<RouterCntrlMsg*>* setupPath(ArbiterRequestMsg* rmsg, int outport);
	virtual void pathNotSetup(ArbiterRequestMsg* rmsg, int p){};
	virtual int getOutport(ArbiterRequestMsg* rmsg);
	virtual int getBcastOutport(ArbiterRequestMsg* bmsg);
	virtual bool requestComplete(ArbiterRequestMsg* rmsg);
	virtual bool isPathGood(int p);


	virtual int pathStatus(ArbiterRequestMsg* rmsg, int outport) = 0;

	virtual int routeLevel(ArbiterRequestMsg* rmsg);

	virtual void cleanup(int numG);


	map<int, bool> blockedOut;
	map<int, bool> blockedIn;

	map<int, map<int, int>* > credits;

	int numVC;

	int numPSE;



	double lastEnergy;

	int myX;
	int myY;
	int numX;
	int numY;

	string name;

	int currVC;



	bool stalled;

	enum PATH_CODE {
		NO_GO = 0,
		GO_OK
	};

#ifdef ENABLE_ORION
	map<int, ORION_Arbiter*> power;
#endif
};

#endif /* ARBITER_H_ */
