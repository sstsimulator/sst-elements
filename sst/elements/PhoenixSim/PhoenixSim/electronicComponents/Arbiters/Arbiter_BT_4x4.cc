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

#include "Arbiter_BT_4x4.h"

Arbiter_BT_4x4::Arbiter_BT_4x4() {


}

Arbiter_BT_4x4::~Arbiter_BT_4x4() {

}

void Arbiter_BT_4x4::init(string id, int level, int numX, int numY, int vc, int ports,
		int numpse, int buff, string n)
{
	PhotonicArbiter::init(id, level, numX, numY, vc, ports, numpse, buff,  n);

	isConnected = myX == myY || myX == numX - 1 - myY;

	GWisN = id[level] >= numX*(numX>>1);

}


bool Arbiter_BT_4x4::isDimensionChange(int inport, int outport){
	return inport / 2 != outport / 2;
}

//returns the port num to go to
int Arbiter_BT_4x4::route(ArbiterRequestMsg* rmsg)
{
	NetworkAddress* addr = (NetworkAddress*)rmsg->getDest();
	int destId = addr->id[level];
	int destx;
	int desty;
	int inport = rmsg->getPortIn();

	//if(isConc){
	//	destx = (destId % (numX * 2)) / 2;
	//	desty = (destId / (numY * 2)) / 2;
	//}else{
		destx = destId % numX;
		desty = destId / numY;
	//}

	int hopsup, hopsdown;

		//X-Y routing

		if(destx != 0 && destx != numX-1)
		{
			if(destx%2==0)
			{
				destx -= 1;
			}
			else
			{
				destx += 1;
			}
		}
		if(inport == SW_N)
		{
			return SW_S;
		}
		if(inport == SW_S)
		{
			return SW_N;
		}

		if(destx == this->myX)
		{
			//correct column to reach ejection switch
			if(((this->myY)%2 == 0 && desty%2 == 1) || ((this->myY)%2 == 1 && desty%2 == 0))
			{
				// if the intermediate switch is on the same side of the loop as the ejection point
				if(desty < (this->myY))
				{
					return SW_N;
				}
				else
				{
					return SW_S;
				}
			}
			else
			{
				if((this->myY)%2==0)
				{
					// nodes == ejection switches
					//minimize: num of nodes to edge from injection point + num of nodes on other side from edge to intermediate turn point
					hopsup = ((this->myY)>>1) + (desty>>1);
					hopsdown = ((numY - (this->myY))>>1) + ((numY - desty - 1)>>1);
				}
				else
				{
					hopsup = (((this->myY) + 1)>>1) + (desty>>1);
					hopsdown = ((numY - (this->myY))>>1) + ((numY - desty - 1)>>1);
				}
				if(hopsup < hopsdown)
				{
					return SW_N;
				}
				else
				{
					return SW_S;
				}
			}
		}
		else
		{
			//not correct column yet so just foward on
			if(inport == SW_E)
			{
				return SW_W;
			}
			else
			{
				return SW_E;
			}
		}

}

void Arbiter_BT_4x4::PSEsetup(int inport, int outport, PSE_STATE st)
{
	if(inport == outport)
	{
		opp_error("no u turns allowed in this nonblocking switch");
	}

	if(inport == SW_N)
	{
		if(outport == SW_E)
		{
			nextPSEState[4] = st;
		}
		else if(outport == SW_S)
		{
									//   vvv need to check this incase another path is using the same PSE
			nextPSEState[1] = st;
			if(InputGateState[2] == gateActive && InputGateDir[2] == SW_W && OutputGateState[3] == gateActive && OutputGateDir[3] == SW_E)
			// if E->W connection is set, make sure to leave it on
			{
				nextPSEState[1] = PSE_ON;
			}
		}
		else if(outport == SW_W)
		{
			// do nothing
		}
	}
	else if(inport == SW_S)
	{
		if(outport == SW_W)
		{
			nextPSEState[5] = st;
		}
		else if(outport == SW_N)
		{
			nextPSEState[0] = st;
			if(InputGateState[3] == gateActive && InputGateDir[3] == SW_E && OutputGateState[2] == gateActive && OutputGateDir[2] == SW_W)
			{
				nextPSEState[0] = PSE_ON;
			}
		}
		else if(outport == SW_E)
		{
			// do nothing
		}
	}
	else if(inport == SW_E)
	{

		if(outport == SW_N)
		{
			nextPSEState[2] = st;
		}
		else if(outport == SW_W)
		{
			nextPSEState[1] = st;
			if(InputGateState[0] == gateActive && InputGateDir[0] == SW_S && OutputGateState[1] == gateActive && OutputGateDir[1] == SW_N)
			{
				nextPSEState[1] = PSE_ON;
			}
		}
		else if(outport == SW_S)
		{
			// do nothing
		}
	}
	else if(inport == SW_W)
	{
		if(outport == SW_S)
		{
			nextPSEState[3] = st;
		}
		else if(outport == SW_E)
		{
			nextPSEState[0] = st;
			if(InputGateState[1] == gateActive && InputGateDir[1] == SW_N && OutputGateState[0] == gateActive && OutputGateDir[0] == SW_S)
			{
				nextPSEState[0] = PSE_ON;
			}
		}
		else if(outport == SW_N)
		{
			// do nothing
		}
	}
	else
	{
		opp_error("unknown input direction specified");
	}
}
