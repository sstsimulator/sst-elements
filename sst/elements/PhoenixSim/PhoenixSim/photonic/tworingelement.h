/*
 * ringelement.h
 *
 *  Created on: Oct 23, 2009
 *      Author: Johnnie Chan
 */

#ifndef TWORINGELEMENT_H_
#define TWORINGELEMENT_H_

#include <vector>
#include "ringelement.h"
#include "packetstat.h"
#include <math.h>
#include "statistics.h"

class TwoRingElement : public virtual RingElement
{
	public:
		TwoRingElement();
		virtual ~TwoRingElement();
	protected:
		vector<int> routingTableAlt;
		double ringFSRAlt;
		double filterBaseWavelengthAlt;
		double filterWavelengthSpacingAlt;
		int  numOfResonantWavelengthsAlt;

		double ringLengthAlt;
		double ringFinesseAlt;
		set <double> resonantWavelengthsAlt;

		virtual void initialize();
		void InitializeTwoRingElement();
		virtual int AccessRoutingTable(int index, PacketStat *ps);
		int RouteAlt(int index);

		virtual void SetRoutingTable() = 0;
		virtual void SetOffResonanceRoutingTable() = 0;
		virtual void SetRoutingTableAlt() = 0;

		bool IsInResonanceAlt(double wavelength);

};

#endif /* RINGELEMENT_H_ */
