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

#ifndef __PSE1X2NXR3_H__
#define __PSE1X2NXR3_H__

#include <omnetpp.h>
#include <vector>
#include "packetstat.h"
#include "sim_includes.h"
#include "messages_m.h"
#include "activemultiringelement.h"

/**
 * TODO - Generated class
 */
class PSE1x2NXR3 : public ActiveMultiRingElement
{
private:
		double PropagationLoss;
		double PassByRingLoss;
		double PassThroughRingLoss;
		double CrossingLoss;

		double Crosstalk_Cross;

		double RingOn_ER_1x2NX;
		double RingOff_ER_1x2NX;

		double DropDelay_1x2NX;
		double ThroughDelay_1x2NX;

		vector< vector<double> > latencyMatrix;

		virtual void Setup();
		virtual double GetLatency(int indexIn, int indexOut);
		virtual double GetPropagationLoss(int indexIn, int indexOut, double wavelength);
		virtual double GetDropIntoRingLoss(int indexIn, int indexOut, double wavelength);
		virtual double GetPassByRingLoss(int indexIn, int indexOut, double wavelength);

		virtual double GetPowerLevel(int state);
		virtual double GetEnergyDissipation(int stateBefore, int stateAfter);
		void SetRoutingTable();
		int GetMultiRingRoutingTable(int index, int ringSet);

		double RingStaticDissipation;
		double RingDynamicOffOn;
		double RingDynamicOnOff;

	public:
		PSE1x2NXR3();
		virtual ~PSE1x2NXR3();
};

#endif
