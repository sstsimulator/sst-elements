#include <omnetpp.h>
#include <vector>
#include "packetstat.h"
#include "sim_includes.h"
#include "messages_m.h"
#include "basicelement.h"

class Facet : public BasicElement
{
	private:
		double IL_Facet;

		virtual void Setup();
		virtual double GetLatency(int indexIn, int indexOut);
		virtual double GetPropagationLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual void SetRoutingTable();
	public:
		Facet();
		virtual ~Facet();
};
