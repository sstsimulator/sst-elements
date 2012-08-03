#ifndef STATISTICS__H
#define STATISTICS__H


#include <omnetpp.h>
#include <fstream>
#include <string>
#include <sstream>
#include <omnetpp.h>
#include <map>
#include <list>

#include "sim_includes.h"
#include "phylayer.h"

#include "StatObject.h"
#include "StatGroup.h"
#include "LogFile.h"

#include "orion/ORION_Config.h"

using namespace std;
using namespace PhyLayer;

class Statistics : public cSimpleModule
{
	public:
		Statistics();
		virtual ~Statistics();


		static list<LogFile*> logs;
		static list<StatGroup*> groups;

		static list<StatObject*> allStats;

		static StatObject* registerStat(string name, int type);
		static StatObject* registerStat(string name, int type, string modifiers);

		static void done();

		static bool isDone;

		static ORION_Tech_Config* DEFAULT_ORION_CONFIG_CNTRL;
		static ORION_Tech_Config* DEFAULT_ORION_CONFIG_DATA;
		static ORION_Tech_Config* DEFAULT_ORION_CONFIG_PROC;

	protected:
		virtual void initialize();
		virtual void finish();
		virtual void handleMessage(cMessage* msg);

		void open(string fileAppend="", string logDirectory="");
		void close();




		//static bool enableNetworkLoadStatistics;
		//static bool enablePhyLayerStatistics;
		//static bool enableDetailILStats;


		simtime_t warmup;
		simtime_t cooldown;

};


#endif
