/************************************************************************
*                                                                       *
*  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
*						(c) Johnnie Chan  2008	*
*                                                                       *
* file: pse1x2.h                                                        *
* description:                                                          *
*                                                                       *
*                                                                       *
*************************************************************************/

#ifndef __PSE1X2R2_H__
#define __PSE1X2R2_H__

#include <omnetpp.h>
#include <vector>
#include "packetstat.h"
#include "sim_includes.h"
#include "messages_m.h"
#include "activetworingelement.h"

using namespace std;



class PSE1x2R2 : public ActiveTwoRingElement
{
	private:
		double PropagationLoss;
		double PassByRingLoss;
  		double PassThroughRingLoss;
		double CrossingLoss;
		double Crosstalk_Cross;
		double BendingLoss;

		double RingOn_ER_1x2;
		double RingOff_ER_1x2;

		double ThroughDelay_1x2;
		double DropDelay_1x2;

		vector< vector<double> > latencyMatrix;

		//vector<PacketStat*> noiseVector;

		virtual void Setup();
		virtual void HandleControlMessage(ElementControlMessage *msg);
		virtual double GetLatency(int indexIn, int indexOut);

		virtual double GetPropagationLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual double GetCrossingLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual double GetDropIntoRingLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual double GetPassByRingLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual double GetBendingLoss(int indexIn, int indexOut, double wavelength = 0);


		virtual double GetPowerLevel(int state);
		virtual double GetEnergyDissipation(int stateBefore, int stateAfter);
		void SetRoutingTable();
		void SetOffResonanceRoutingTable();
		void SetRoutingTableAlt();
		double RingStaticDissipation;
		double RingDynamicOffOn;
		double RingDynamicOnOff;

		double Power_Switch_Static;
		double Power_Off_to_On;
		double Power_On_to_Off;


	public:
		PSE1x2R2();
		virtual ~PSE1x2R2();


};

#endif
