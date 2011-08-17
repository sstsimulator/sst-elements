#ifndef __THERMALMODEL_H__
#define __THERMALMODEL_H__

#include "sim_includes.h"
#include <omnetpp.h>
#include <string.h>
#include <string>
#include <map>
#include <sstream>
#include <fstream>

#define VERBOSE 2

#ifdef ENABLE_HOTSPOT

extern "C"
{
	#include "flp.h"
	#include "temperature.h"
	#include "temperature_grid.h"
	#include "util.h"
}

class ThermalUnit
{
	public:
		ThermalUnit();

		unit_t* unit;

		// power will remain unchanged by the Thermal Model
		double power; // measured in W

		// energy will reset to 0 after every 'cycle'
		double energy; // measured in J
};

class ThermalModel : public cSimpleModule
{
	protected:
		virtual void initialize(int stage);
		virtual int numInitStages() const {return 4;}
		virtual void handleMessage(cMessage *msg);
		virtual void calcThermal(cMessage *msg);
		virtual void dumpThermalFile(cMessage *msg);
		virtual void finish();

	private:
		static flp_t *flp;
		static map<string, ThermalUnit*> functionalUnitList;
		static char *init_file;
		static char *steady_file;
		static char *grid_steady_file;

		static RC_model_t *model;
		static double *temp;
		static double *power;

		static double *overall_power;
		static double *steady_temp;

		cMessage *thermalCycle;
		double thermalCyclePeriod;

		cMessage *thermalFileDumpCycle;
		double thermalFileDumpPeriod;

		double prevTime;
		int samples;
		bool firstCall;
		bool useGridModel;

		void generateFLP();
		static string createUniqueName(const char *name, int id);
		void cycleTemperature();

	public:
		ThermalModel();
		static bool registerThermalObject(const char *name, int id, double width, double height, double leftx, double bottomy);
		static void addThermalObjectEnergy(const char *name, int id, double energy);
		static void setThermalObjectPower(const char *name, int id, double power);
		static double getCurrentTemperature(const char *name, int id, char unitType = 'K');

		void saveFLP(const char *filename);




};

#else

// Stub version
class ThermalModel : public cSimpleModule
{
	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finish();
};
#endif
#endif
