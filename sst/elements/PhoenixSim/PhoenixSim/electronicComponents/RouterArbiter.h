/*
 * RouterArbiter.h
 *
 *  Created on: Mar 8, 2009
 *      Author: Gilbert
 */

#ifndef ROUTERARBITER_H_
#define ROUTERARBITER_H_

#include <omnetpp.h>
#include <map>
#include "messages_m.h"
#include "sim_includes.h"

#include "Arbiter.h"

#include "AddressTranslator.h"

#include "Processor.h"

#ifdef ENABLE_ORION
#include "orion/ORION_Arbiter.h"
#include "orion/ORION_Util.h"
#include "orion/ORION_Config.h"
#endif

using namespace std;

class RouterArbiter: public cSimpleModule {
public:
	RouterArbiter();

	virtual ~RouterArbiter();

protected:
	int numInitStages() const;
	virtual void initialize(int stage);
	virtual void handleMessage(cMessage *msg);
	virtual void finish();

	void ServeClock();

	Arbiter* arbiter;

	map<int, cGate*> reqIn;
	map<int, cGate*> reqOut;
	cGate* XbarCntrlIn;
	cGate* XbarCntrlOut;
	map<int, cGate*> pseCntrl;
	map<int, cGate*> ackIn;

	int numPorts;
	int numVC;
	int numPSE;
	int type;

	int numGrant;

	static AddressTranslator* translator;

	int flit_width;
	int buffer_size;
	double clockPeriod;

	string myId;
	int level;
	int numX;
	int numY;


	cMessage* clk;

	StatObject* P_static;
	StatObject* E_dynamic;

#ifdef ENABLE_ORION
	map<int, ORION_Arbiter*> power_out;


	static double totalEnergy;
	static int numRouters;
#endif

	//Router Arbiter types - this is what differentiates specific routers (besides the port count and pse cntrl)
	//common-

	// 1 - gateway control

	// 5 - Processor Router

	// blocking torus -
	// 10 - ejection router (path to left)
	// 11 - injection router
	// 12 - gateway router
	// 13 - 4x4 router logic for BT
	//-concentrated versions-->
	// 15 - ejection router (path to left)
	// 16 - injection router
	// 17 - 4x4 router logic for BT

	// electronic
	// 20 - regular mesh node
	// 21 - concentrated mesh
	// 22 - torus node
	// 23 - concentrated torus node
	// 24 - hypershaft node
	// 26 - flattened butterfly
	// 27 - CMesh

	// square root
	// 30 - square root node
	// 31 - square root 8x8 node


	//fat tree
	// 40 - fat tree node
	// 41 - fat tree root node

	//nonblocking torus
	//50 - node router
	//51 - Gateway inj/ej router
	//  52,53 concentrated versions
	//54 - GWC (it has different grouping of id's for concentrated)

	//60 - gateway inj/ej router
	//61 - node router

	// torus NX (no crossing)
	//70 - node router
	//71 - gateway router

	// Photonic Mesh
	//80 - gateway switch node (5 port almost)


     //electronic mesh - circuit switched
	//90 - main node

	// For standalone switches
	//1000 - crossbar (with single corner optimization)


	// For networks from other groups


	// MIT UCB - Proc to DRAM network
	//10000 - Local Group Mesh Router
	//10001 - Memory Group Router

	// MIT Clos Network
	//10010 - ClosNetworkNode

	enum ARBITER_TYPES {

		ARB_GWC = 1,

		ARB_PR = 5,

		ARB_BT_EJ = 10,
		ARB_BT_INJ = 11,
		ARB_BT_GWS = 12,
		ARB_BT_4x4 = 13,

		ARB_BT_EJ_Conc = 15,
		ARB_BT_INJ_Conc = 16,
		ARB_BT_4x4_Conc = 17,

		ARB_EM = 20,
		ARB_EM_Conc = 21,
		ARB_ET = 22,
		ARB_ET_Conc = 23,
		ARB_EHS = 24,
		ARB_FB = 26,
		ARB_CM = 27,

		ARB_SQ4x4 = 30,
		ARB_SQ8x8 = 31,
		ARB_SQ16x16 = 32,
		ARB_SR_QUAD = 33,
		ARB_SR_TOP = 34,
		ARB_SR_MID = 35,

		ARB_FT = 40,
		ARB_FT_Root = 41,
		ARB_FT_GWC = 42,

		ARB_NBT_Gateway = 51,
		ARB_NBT_Node = 50,
		ARB_NBT_Gateway_Conc = 53,
		ARB_NBT_Node_Conc = 52,
		ARB_NBT_GWC = 54,

		ARB_XB_Gateway = 60,
		ARB_XB_Node = 61,

		ARB_TNX_Node = 70,
		ARB_TNX_Gateway = 71,

		ARB_PM_Node = 80,

		ARB_EM_CC = 90,

		ARB_SiN_Node = 100,

		ARB_CrossbarSwitch = 1000,

		ARB_MITUCB_MeshNode = 10000,
		ARB_MITUCB_MemoryNode = 10001,

		ARB_MIT_PhotonicClos = 10010
	};

};

#endif /* ROUTERARBITER_H_ */
