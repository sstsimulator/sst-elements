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

#include "Arbiter_NBT_Node_SP.h"

Arbiter_NBT_Node_SP::Arbiter_NBT_Node_SP() {

}

Arbiter_NBT_Node_SP::~Arbiter_NBT_Node_SP() {

}

void Arbiter_NBT_Node_SP::PSEsetup(int inport, int outport, PSE_STATE st)
{
	if (inport == outport)
	{
		opp_error("no u turns allowed in this nonblocking switch");
	}

	if (inport == SW_N)
	{
		if (outport == SW_E)
		{
			nextPSEState[4] = st;
		}
		else if (outport == SW_S)
		{
			//   vvv need to check this incase another path is using the same PSE
			nextPSEState[1] = (InputGateState[2] == gateActive && InputGateDir[2] == SW_W && OutputGateState[3] == gateActive && OutputGateDir[3] == SW_E) ? PSE_ON : st;
		}
		else if (outport == SW_W)
		{
			// do nothing
		}
	}
	else if (inport == SW_S)
	{
		if (outport == SW_W)
		{
			nextPSEState[5] = st;
		}
		else if (outport == SW_N)
		{
			nextPSEState[0] = (InputGateState[3] == gateActive && InputGateDir[3] == SW_E && OutputGateState[2] == gateActive && OutputGateDir[2] == SW_W) ? PSE_ON : st;
		}
		else if (outport == SW_E)
		{
			// do nothing
		}
	}
	else if (inport == SW_E)
	{
		if (outport == SW_N)
		{;
			nextPSEState[2] = st;
		}
		else if (outport == SW_W)
		{
			nextPSEState[1] = (InputGateState[0] == gateActive && InputGateDir[0] == SW_S && OutputGateState[1] == gateActive && OutputGateDir[1] == SW_N) ? PSE_ON : st;
		}
		else if (outport == SW_S)
		{
			// do nothing
		}
	}
	else if (inport == SW_W)
	{
		if (outport == SW_S)
		{
			nextPSEState[3] = st;
		}
		else if (outport == SW_E)
		{
			nextPSEState[0] = (InputGateState[1] == gateActive && InputGateDir[1] == SW_N && OutputGateState[0] == gateActive && OutputGateDir[0] == SW_S) ? PSE_ON : st;
		}
		else if (outport == SW_N)
		{
			// do nothing
		}
	}
	else
	{
		opp_error("unknown input direction specified");
	}
}
