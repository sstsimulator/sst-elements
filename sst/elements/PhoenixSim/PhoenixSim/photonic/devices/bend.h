/************************************************************************
*                                                                       *
*  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
*						(c) Johnnie Chan  2008							*
*                                                                       *
* file: bend.h                                                          *
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

class Bend : public BasicElement
{
	private:
		double PropagationLoss;
		double BendingLoss;
		double Latency_Bend;
		double Angle_Bend;
		//vector<PacketStat*> noiseVector;

		virtual void Setup();
		virtual double GetLatency(int indexIn, int indexOut);

		virtual double GetPropagationLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual double GetBendingLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual void SetRoutingTable();

	public:
		Bend();
		virtual ~Bend();
};

