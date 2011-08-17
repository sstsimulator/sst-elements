/*************************************************************************
*                                                                       *
*  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
*						(c) Johnnie Chan  2008							*
*                                                                       *
* file: Cross.h                                                         *
* description:                                                          *
*                                                                       *
*                                                                       *
*************************************************************************/

#include <omnetpp.h>
#include <vector>
#include "packetstat.h"
#include "sim_includes.h"
#include "messages_m.h"
#include "basicelement.h"

class Cross : public BasicElement
{
	private:
		//double IL_Cross;
		double Latency_Cross;

		double PropagationLoss;
		double CrossingLoss;
		double Crosstalk_Cross;


		virtual void Setup();
		virtual double GetLatency(int indexIn, int indexOut);
		virtual double GetPropagationLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual double GetCrossingLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual void SetRoutingTable();

	public:
		Cross();
		virtual ~Cross();
};

