#ifndef _MODULATOR_H_
#define _MODULATOR_H_

#include <omnetpp.h>
#include <vector>
#include "packetstat.h"
#include "sim_includes.h"
#include "messages_m.h"
#include "convert.h"
#include "statistics.h"
#include "phylayer.h"
#include "modulatorelement.h"

using namespace PhyLayer;

class Modulator : public ModulatorElement
{

	private:
		double resonantWavelength;
		double Latency_Modulator;
		double Power_Laser;
		// currently defaults to true
		bool sourceOn;
		double currSourcePower;
		double currSourceWavelength;
		double photonicBitRate;


		double EperBit;
		double Estatic;

		int numOfWavelengths;
		bool useWDM;

        double PropagationLoss;
        double PassByRingLoss;
        double PassThroughRingLoss;
        double CrossingLoss;

        double NoiseBandwidth;
        double ModulatorExtinctionRatio;
		double LaserRelativeIntensityNoise;

		simtime_t lastSwitchTimestamp;
		bool driverOn;
		unsigned long cumulativeData;

		PacketStat *currPacketStat;

		// used for Multiring Designs
		bool useMultiCircuits;
		int numOfCircuits;

		virtual void Setup();

		virtual double GetLatency(int indexIn, int indexOut);

		virtual double GetPropagationLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual double GetPassByRingLoss(int indexIn, int indexOut, double wavelength = 0);

		virtual double GetModulatorLevel();

		double GetModulatorWavelength();

		virtual void HandleModulatorMessage(PhotonicMessage *pmsg, int indexIn);
		virtual double AddModulationEnergyDissipation(double time);

		void SetRoutingTable();
		void SetOffResonanceRoutingTable();
	public:
		Modulator();
		virtual ~Modulator();
};

#endif
