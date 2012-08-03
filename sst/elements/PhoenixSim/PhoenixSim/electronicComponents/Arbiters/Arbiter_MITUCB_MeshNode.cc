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

#include "Arbiter_MITUCB_MeshNode.h"

Arbiter_MITUCB_MeshNode::Arbiter_MITUCB_MeshNode() {

	routingMatrix[0] = pair<int,int>(3,0);
	routingMatrix[1] = pair<int,int>(1,0);
	routingMatrix[2] = pair<int,int>(2,0);
	routingMatrix[3] = pair<int,int>(0,0);
	routingMatrix[4] = pair<int,int>(3,1);
	routingMatrix[5] = pair<int,int>(1,1);
	routingMatrix[6] = pair<int,int>(2,1);
	routingMatrix[7] = pair<int,int>(0,1);
	routingMatrix[8] = pair<int,int>(3,2);
	routingMatrix[9] = pair<int,int>(1,2);
	routingMatrix[10] = pair<int,int>(2,2);
	routingMatrix[11] = pair<int,int>(0,2);
	routingMatrix[12] = pair<int,int>(3,3);
	routingMatrix[13] = pair<int,int>(1,3);
	routingMatrix[14] = pair<int,int>(2,3);
	routingMatrix[15] = pair<int,int>(0,3);

	routingMatrixAlt[0] = pair<int,int>(3,1);
	routingMatrixAlt[1] = pair<int,int>(1,1);
	routingMatrixAlt[2] = pair<int,int>(2,1);
	routingMatrixAlt[3] = pair<int,int>(0,1);
	routingMatrixAlt[4] = pair<int,int>(3,0);
	routingMatrixAlt[5] = pair<int,int>(1,0);
	routingMatrixAlt[6] = pair<int,int>(2,0);
	routingMatrixAlt[7] = pair<int,int>(0,0);
	routingMatrixAlt[8] = pair<int,int>(3,3);
	routingMatrixAlt[9] = pair<int,int>(1,3);
	routingMatrixAlt[10] = pair<int,int>(2,3);
	routingMatrixAlt[11] = pair<int,int>(0,3);
	routingMatrixAlt[12] = pair<int,int>(3,2);
	routingMatrixAlt[13] = pair<int,int>(1,2);
	routingMatrixAlt[14] = pair<int,int>(2,2);
	routingMatrixAlt[15] = pair<int,int>(0,2);




}

Arbiter_MITUCB_MeshNode::~Arbiter_MITUCB_MeshNode() {

}



int Arbiter_MITUCB_MeshNode::route(ArbiterRequestMsg* rmsg)
{
	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[level];

	int group = myAddr->id[level]/16;
	int groupcol = group%4;
	bool topHalf = group < 8;
	bool normalTable = (group/4)%2 == 0;
	//std::cout<<"myid:"<<myId<<" group="<<group<<" groupcol="<<groupcol<<" "<<(topHalf?"top":"bottom")<<" "<<(normalTable?"norm":"alt")<<endl;
	// slight hack to get MIT network to work properly
	int thisX = (myAddr->id[level]%16)%4;
	int thisY = (myAddr->id[level]%16)/4;


	if(destId >= 256 && destId < 272) // Memory -> Core transfer
	{
		int destPort = destId - 256;
		pair<int,int> coord = normalTable?routingMatrix[destPort]:routingMatrixAlt[destPort];

		if(normalTable)
		{
			coord.second = (4 + coord.second + groupcol) % 4;
		}
		else
		{
			coord.second += 4;
			coord.second += (groupcol==1?-1:0);
			coord.second += (groupcol==2?2:0);
			coord.second += (groupcol==3?1:0);
			coord.second %= 4;
		}

		if(topHalf)
		{
			//coord.second = 3 - coord.second;
		}

		//std::cout<<"myX="<<thisX<<" myY="<<thisY<<" gotoX="<<coord.first<<" gotoY="<<coord.second<<endl;
		if(thisX < coord.first)
		{
			return SW_E;
		}
		else if(thisX > coord.first)
		{
			return SW_W;
		}
		else if(thisY < coord.second)
		{
			return SW_S;
		}
		else if(thisY > coord.second)
		{
			return SW_N;
		}
		else
		{
			return SW_Memory;
		}
	}
	else if(destId >= 0 && destId < 256) // Memory -> To Core Transfer
	{
		if(group == destId/16)
		{
			int gotoX = (destId%16)%4;
			int gotoY = (destId%16)/4;
			if(thisX < gotoX)
			{
				return SW_E;
			}
			else if(thisX > gotoX)
			{
				return SW_W;
			}
			else if(thisY < gotoY)
			{
				return SW_S;
			}
			else if(thisY > gotoY)
			{
				return SW_N;
			}
			else
			{
				return SW_Processor;
			}
		}
		else
		{
			std::cout<<"thisgroup "<<group<< "belongs in "<<destId/16<<endl;
			throw cRuntimeError("Wrong group");
		}
	}
}

