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

#include "Arbiter_MITUCB_MemoryNode.h"

Arbiter_MITUCB_MemoryNode::Arbiter_MITUCB_MemoryNode() {

	routingTableFiber[0] = 1;
	routingTableFiber[1] = 0;
	routingTableFiber[2] = 1;
	routingTableFiber[3] = 0;
	routingTableFiber[4] = 0;
	routingTableFiber[5] = 1;
	routingTableFiber[6] = 0;
	routingTableFiber[7] = 1;
	routingTableFiber[8] = 2;
	routingTableFiber[9] = 3;
	routingTableFiber[10] = 2;
	routingTableFiber[11] = 3;
	routingTableFiber[12] = 3;
	routingTableFiber[13] = 2;
	routingTableFiber[14] = 3;
	routingTableFiber[15] = 2;

}

Arbiter_MITUCB_MemoryNode::~Arbiter_MITUCB_MemoryNode() {

}



int Arbiter_MITUCB_MemoryNode::route(ArbiterRequestMsg* rmsg)
{
	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[level];

	// Ports
	// 0..15 accesspoints
	// 16 to memory node
	int destPort = destId - 256;
	int srcPort = this->myAddr->id[level];
	//std::cout<<"destPort:"<<destPort<<" srcPort:"<<srcPort<<endl;
	if(destPort == myAddr->id[level])
	{
		return 16;
	}
	else if(destId >= 0 && destId < 256)
	{
		int group = destId/16;

		int lambdaId = ((group%4)+(srcPort/4))%4;

		int fiberId = routingTableFiber[group];
		//std::cout<<"group "<<group<<" picking fiber:"<<lambdaId<<" lambda:"<<fiberId<<endl;
		if(srcPort == 0 || srcPort == 1 || srcPort == 6 || srcPort == 7 || srcPort == 8 || srcPort == 9 || srcPort == 14 || srcPort == 15)
		{
		}
		else
		{
			fiberId += (fiberId%2==0?1:-1);
		}
		//std::cout<<"picking fiber:"<<fiberId<<" lambda:"<<lambdaId<<endl;
		return lambdaId + fiberId*4;
	}
	else
	{
		std::cout<<"exiting: "<<destId<<" myid"<<this->myAddr->id[level]<<endl;;
		opp_error("haltmemory");
	}
}

