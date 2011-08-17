/************************************************************************
*                                                                       *
*  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
*						(c) Johnnie Chan  2008							*
*                                                                       *
* file: line.h                                                          *
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

class Line : public BasicElement
{
	private:
		double Length_Line;
		double LatencyRate_Line;
		double PropagationLoss;

		virtual void Setup();
		virtual double GetLatency(int indexIn, int indexOut);
		virtual double GetPropagationLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual void SetRoutingTable();
	public:
		Line();
		virtual ~Line();
};


