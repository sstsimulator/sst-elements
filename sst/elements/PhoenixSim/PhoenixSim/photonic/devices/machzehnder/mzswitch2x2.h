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

#ifndef __MZSWITCH2X2_H__
#define __MZSWITCH2X2_H__

#include <omnetpp.h>
#include "activeelement.h"
/**
 * TODO - Generated class
 */
class MZSwitch2x2 : public ActiveElement
{
private:

    double PropagationLoss;
    double PropagationDelay;

    double LatencyBarPath;
    double LatencyCrossPath;


    double InsertionLossBarState;
    double InsertionLossCrossState;
    double CrosstalkBarState;
    double CrosstalkCrossState;

    double StaticPowerDissipationOffState;
    double StaticPowerDissipationOnState;

    double DynamicPowerDissipationOffToOn;
    double DynamicPowerDissipationOnToOff;

protected:
	void Setup();
	double GetLatency(int indexIn, int indexOut);
	double GetPropagationLoss(int indexIn, int indexOut);
	double GetBendingLoss(int indexIn, int indexOut);

	double GetPowerLevel(int state);
	double GetEnergyDissipation(int stateBefore, int stateAfter);

	void SetRoutingTable();

public:
	MZSwitch2x2();
	virtual ~MZSwitch2x2();
};

#endif
