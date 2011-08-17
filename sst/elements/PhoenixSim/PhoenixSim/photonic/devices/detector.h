#ifndef _DETECTOR_H_
#define _DETECTOR_H_

#include <omnetpp.h>
#include <vector>
#include <math.h>
#include "packetstat.h"
#include "sim_includes.h"
#include "messages_m.h"
#include "convert.h"
#include "statistics.h"
#include "detectorelement.h"

#include "phylayer.h"

using namespace PhyLayer;

class Detector : public DetectorElement
{
	private:
		double resonantWavelength;
		double Latency_Detector;

		simtime_t lastSwitchTimestamp;
		bool dataOn;
		double cumulativeData;

		bool useWDM;

		bool enablePhyLayerStatistics;
		bool enableDetailILStats;
		bool enableCommPairILStats;

		double EperBit;
		double Detector_StaticPower;

		bool enabled;

		double laserPower;

        double PropagationLoss;
        double PassByRingLoss;
        double PassThroughRingLoss;
        double CrossingLoss;
        double RingThrough_ER_DetectorFilter;
        double RingDrop_ER_DetectorFilter;
		int numOfWavelengths;






	protected:
		virtual void Setup();
		virtual double GetLatency(int indexIn, int indexOut);
		virtual double GetPropagationLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual double GetPassByRingLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual double GetDropIntoRingLoss(int indexIn, int indexOut, double wavelength = 0);

		virtual double AddDetectionEnergyDissipation(double time);
		void SetRoutingTable();
		void SetOffResonanceRoutingTable();
	public:
		Detector();
		virtual ~Detector();
};

#endif
