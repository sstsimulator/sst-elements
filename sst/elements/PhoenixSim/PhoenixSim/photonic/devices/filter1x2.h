//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#ifndef __FILTER1X2_H__
#define __FILTER1X2_H__

#include <omnetpp.h>

#include <vector>
#include "packetstat.h"
#include "sim_includes.h"
#include "messages_m.h"
#include "ringelement.h"

using namespace std;


class Filter1x2 : public RingElement
{
	public:
		Filter1x2();
		virtual ~Filter1x2();


	private:
		double PropagationLoss;
		double PassByRingLoss;
  		double PassThroughRingLoss;
    	double CrossingLoss;

    	double Crosstalk_Cross;

        double RingThrough_ER_Filter1x2;
        double RingDrop_ER_Filter1x2;
        double ThroughDelay_Filter1x2;
        double DropDelay_Filter1x2;

		virtual void Setup();

		virtual double GetLatency(int indexIn, int indexOut);

		virtual double GetPropagationLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual double GetCrossingLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual double GetDropIntoRingLoss(int indexIn, int indexOut, double wavelength = 0);
		virtual double GetPassByRingLoss(int indexIn, int indexOut, double wavelength = 0);
		void SetRoutingTable();
		void SetOffResonanceRoutingTable();
			};

#endif
