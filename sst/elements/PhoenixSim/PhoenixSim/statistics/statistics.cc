#include "statistics.h"

#include "StatObject_EnergyStatic.h"
#include "StatObject_EnergyEvent.h"
#include "StatObject_EnergyOn.h"

#include "StatObject_MMA.h"
#include "StatObject_Count.h"
#include "StatObject_TimeAvg.h"
#include "StatObject_Total.h"

#include "StatGroup_ListAll.h"
#include "StatGroup_Sum.h"

Define_Module( Statistics)
;

list<LogFile*> Statistics::logs;
list<StatGroup*> Statistics::groups;
list<StatObject*> Statistics::allStats;

bool Statistics::isDone = false;

ORION_Tech_Config* Statistics::DEFAULT_ORION_CONFIG_CNTRL = NULL;
ORION_Tech_Config* Statistics::DEFAULT_ORION_CONFIG_DATA = NULL;
ORION_Tech_Config* Statistics::DEFAULT_ORION_CONFIG_PROC = NULL;

Statistics::Statistics() {
	StatGroup* sg = new StatGroup_ListAll("Electronic Energy",
			StatObject::ENERGY_STATIC, StatObject::ENERGY_LAST, "electronic");
	groups.push_back(sg);

	sg = new StatGroup_Sum("Electronic Energy", StatObject::ENERGY_STATIC,
			StatObject::ENERGY_LAST, "electronic");
	groups.push_back(sg);

	sg = new StatGroup_ListAll("Photonic Energy", StatObject::ENERGY_STATIC,
			StatObject::ENERGY_LAST, "photonic");
	groups.push_back(sg);

	sg = new StatGroup_Sum("Photonic Energy", StatObject::ENERGY_STATIC,
			StatObject::ENERGY_LAST, "photonic");
	groups.push_back(sg);

	sg = new StatGroup_ListAll("CPU Energy", StatObject::ENERGY_STATIC,
			StatObject::ENERGY_LAST, "cpu");
	groups.push_back(sg);

	sg = new StatGroup_ListAll("DRAM Energy", StatObject::ENERGY_STATIC,
			StatObject::ENERGY_LAST, "DRAM");
	groups.push_back(sg);

	sg = new StatGroup_Sum("DRAM Energy", StatObject::ENERGY_STATIC,
			StatObject::ENERGY_LAST, "DRAM");
	groups.push_back(sg);

	sg = new StatGroup_Sum("All Energy", StatObject::ENERGY_STATIC,
			StatObject::ENERGY_LAST, "");
	groups.push_back(sg);

	sg = new StatGroup_ListAll("Component Counts", StatObject::COUNT,
			StatObject::COUNT, "");
	groups.push_back(sg);

	sg = new StatGroup_ListAll("Performance: Application",
			StatObject::TIME_AVG, StatObject::TIME_AVG, "application");
	groups.push_back(sg);

	sg = new StatGroup_ListAll("Performance: Application", StatObject::MMA,
			StatObject::MMA, "application");
	groups.push_back(sg);

	sg = new StatGroup_ListAll("Performance: NIF", StatObject::MMA,
			StatObject::TIME_AVG, "NIF");
	groups.push_back(sg);

	sg = new StatGroup_ListAll("Performance: DRAM", StatObject::BEGIN,
			StatObject::TOTAL, "DRAM");
	groups.push_back(sg);

	sg = new StatGroup_ListAll("Physical Layer Stats", StatObject::MMA,
			StatObject::MMA, "PhyStats");
	groups.push_back(sg);

	sg = new StatGroup_ListAll("Insertion Loss", StatObject::MMA,
			StatObject::MMA, "IL");
	groups.push_back(sg);
}

Statistics::~Statistics() {
	list<LogFile*>::iterator it;

	for (it = logs.begin(); it != logs.end(); it++) {
		(*it)->write();
		(*it)->close();
		delete (*it);
	}

	list<StatGroup*>::iterator it2;

	for (it2 = groups.begin(); it2 != groups.end(); it2++) {
		delete (*it2);
	}

	list<StatObject*>::iterator it3;
	for (it3 = allStats.begin(); it3 != allStats.end(); it3++) {
		delete (*it3);
	}
}

void Statistics::initialize() {

	int technology = par("O_technology_cntrl");
	int trans_type = par("O_trans_type_cntrl");
	double voltage = par("O_voltage_cntrl");
	double frequency = par("O_frequency_cntrl");

	DEFAULT_ORION_CONFIG_CNTRL = new ORION_Tech_Config(technology, trans_type,
			voltage, frequency);

	technology = par("O_technology_data");
	trans_type = par("O_trans_type_data");
	voltage = par("O_voltage_data");
	frequency = par("O_frequency_data");

	DEFAULT_ORION_CONFIG_DATA = new ORION_Tech_Config(technology, trans_type,
			voltage, frequency);

	technology = par("O_technology_proc");
	trans_type = par("O_trans_type_proc");
	voltage = par("O_voltage_proc");
	frequency = par("O_frequency_proc");

	DEFAULT_ORION_CONFIG_PROC = new ORION_Tech_Config(technology, trans_type,
			voltage, frequency);

	string netn = par("networkName");
	string cust = par("customInfo");
	stringstream st;
	string suf = netn + "_" + st.str();
	if (cust != "")
		suf += "_" + cust;

	//open(suf, par("logDirectory"));

	string dir = par("logDirectory");

	LogFile* lf = new LogFile(dir + "simStats_" + suf + ".csv");

	list<StatGroup*>::iterator iter;
	for (iter = groups.begin(); iter != groups.end(); iter++) {
		lf->addGroup(*iter);
	}
	logs.push_back(lf);

	warmup = par("STATS_warmup");
	cooldown = par("STATS_cooldown");

	StatObject::warmup = SIMTIME_DBL(warmup);
	StatObject::cooldown = SIMTIME_DBL(cooldown);

}

void Statistics::finish() {

}

void Statistics::done() {
	isDone = true;
}

void Statistics::handleMessage(cMessage* msg) {

}

StatObject* Statistics::registerStat(string name, int type) {
	return registerStat(name, type, "");
}

StatObject* Statistics::registerStat(string name, int type, string modifiers) {

	StatObject *ret = NULL;

	switch (type) {
	case StatObject::ENERGY_STATIC:
		ret = new StatObject_EnergyStatic(name);
		break;

	case StatObject::ENERGY_EVENT:
		ret = new StatObject_EnergyEvent(name);
		break;

	case StatObject::ENERGY_ON:
		ret = new StatObject_EnergyOn(name);
		break;

	case StatObject::COUNT:
		ret = new StatObject_Count(name);
		break;

	case StatObject::MMA:
		ret = new StatObject_MMA(name);
		break;

	case StatObject::TIME_AVG:
		ret = new StatObject_TimeAvg(name);
		break;

	case StatObject::TOTAL:
		ret = new StatObject_Total(name);
		break;

	default:
		opp_error("Statistics::registerStat - unknown stat type");

	}

	list<StatObject*>::iterator it2;

	for (it2 = allStats.begin(); it2 != allStats.end(); it2++) {
		if ((*it2)->equals(ret)) {
			delete ret;
			return *it2;
		}
	}

	allStats.push_back(ret);

	list<StatGroup*>::iterator it;

	for (it = groups.begin(); it != groups.end(); it++) {
		(*it)->addStat(ret, modifiers);
	}

	return ret;
}

