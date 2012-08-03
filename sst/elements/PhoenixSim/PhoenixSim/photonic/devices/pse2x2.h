/*************************************************************************
*                                                                       *
*  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
*                       (c) Assaf Shacham 2006-7			*
*						(c) Johnnie Chan  2008	*
*                                                                       *
* file: PSE2x2.h                                                        *
* description:                                                          *
*                                                                       *
*                                                                       *
*************************************************************************/

#include <omnetpp.h>
#include <vector>
#include "packetstat.h"
#include "sim_includes.h"
#include "messages_m.h"
#include "activeringelement.h"

class PSE2x2 : public ActiveRingElement
{
	private:
		double PropagationLoss;
		double PassByRingLoss;
		double PassThroughRingLoss;
		double CrossingLoss;

		double Crosstalk_Cross;

		double RingOn_ER_2x2;
		double RingOff_ER_2x2;

		double CrossDelay_2x2;
		double BarDelay_2x2;

		vector< vector<double> > latencyMatrix;

		//vector<PacketStat*> noiseVector;

		virtual void Setup();
		virtual double GetLatency(int indexIn, int indexOut);
		virtual double GetPropagationLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual double GetCrossingLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual double GetDropIntoRingLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual double GetPassByRingLoss(int indexIn, int indexOut, double wavelength = 0);

		virtual double GetPowerLevel(int state);
		virtual double GetEnergyDissipation(int stateBefore, int stateAfter);
		void SetRoutingTable();
		void SetOffResonanceRoutingTable();
		double RingStaticDissipation;
		double RingDynamicOffOn;
		double RingDynamicOnOff;

	public:
		PSE2x2();
		virtual ~PSE2x2();
};

