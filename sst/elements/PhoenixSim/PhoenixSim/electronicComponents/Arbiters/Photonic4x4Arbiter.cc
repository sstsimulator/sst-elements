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

#include "Photonic4x4Arbiter.h"

Photonic4x4Arbiter::Photonic4x4Arbiter() {

}

Photonic4x4Arbiter::~Photonic4x4Arbiter() {
	// TODO Auto-generated destructor stub
}

void Photonic4x4Arbiter::PSEsetup(int inport, int outport, PSE_STATE st)
{
	if (variant == 0)
	{     //original 4x4
		if (inport == SW_N)
		{
			if (outport == SW_E)
			{
				nextPSEState[4] = st;
			}
			else if (outport == SW_S)
			{
				//   vvv need to check this incase another path is using the same PSE
				nextPSEState[1] = (InputGateState[SW_E] == gateActive
						&& InputGateDir[SW_E] == SW_W && OutputGateState[SW_W]
						== gateActive && OutputGateDir[SW_W] == SW_E) ? PSE_ON
						: st;
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
				nextPSEState[0] = (InputGateState[SW_W] == gateActive
						&& InputGateDir[SW_W] == SW_E && OutputGateState[SW_E]
						== gateActive && OutputGateDir[SW_E] == SW_W) ? PSE_ON
						: st;
			}
			else if (outport == SW_E)
			{
				// do nothing
			}
		}
		else if (inport == SW_E)
		{
			if (outport == SW_N)
			{
				nextPSEState[2] = st;
			}
			else if (outport == SW_W)
			{
				nextPSEState[1] = (InputGateState[SW_N] == gateActive
						&& InputGateDir[SW_N] == SW_S && OutputGateState[SW_S]
						== gateActive && OutputGateDir[SW_S] == SW_N) ? PSE_ON
						: st;
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
				nextPSEState[0] = (InputGateState[SW_S] == gateActive
						&& InputGateDir[SW_S] == SW_N && OutputGateState[SW_N]
						== gateActive && OutputGateDir[SW_N] == SW_S) ? PSE_ON
						: st;
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
	else if(variant == 1)
	{   // 4x4 regular path (straigth path)
		if (inport == SW_N)
		{
			if (outport == SW_E)
			{
					nextPSEState[4] = st;
			}
			else if (outport == SW_S)
			{

			}
			else if (outport == SW_W)
			{
				nextPSEState[1] = (InputGateState[SW_E] == gateActive
						&& InputGateDir[SW_E] == SW_S && OutputGateState[SW_S]
						== gateActive && OutputGateDir[SW_S] == SW_E) ? PSE_ON
						: st;
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

			}
			else if (outport == SW_E)
			{
				nextPSEState[0] = (InputGateState[SW_W] == gateActive
					&& InputGateDir[SW_W] == SW_N && OutputGateState[SW_N]
					== gateActive && OutputGateDir[SW_N] == SW_W) ? PSE_ON
					: st;
			}
		}
		else if (inport == SW_E)
		{
			if (outport == SW_N)
			{
				nextPSEState[2] = st;
			}
			else if (outport == SW_W)
			{

			}
			else if (outport == SW_S)
			{
				nextPSEState[1] = (InputGateState[SW_N] == gateActive
					&& InputGateDir[SW_N] == SW_W && OutputGateState[SW_W]
					== gateActive && OutputGateDir[SW_W] == SW_N) ? PSE_ON
					: st;
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

			}
			else if (outport == SW_N)
			{
				nextPSEState[0] = (InputGateState[SW_S] == gateActive
					&& InputGateDir[SW_S] == SW_E && OutputGateState[SW_E]
					== gateActive && OutputGateDir[SW_E] == SW_S) ? PSE_ON
					: st;
			}
		}
		else
		{
			opp_error("unknown input direction specified");
		}
	}
	else
	{
		opp_error("Photonic4x4Arbiter: switch variant PSEsetup not implemented");
	}
}
