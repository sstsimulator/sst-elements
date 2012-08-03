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

#ifndef ARBITER_MITUCB_MESHNODE_H_
#define ARBITER_MITUCB_MESHNODE_H_

#include "ElectronicArbiter.h"
#include <map>

class Arbiter_MITUCB_MeshNode : public ElectronicArbiter
{
public:
	Arbiter_MITUCB_MeshNode();
	virtual ~Arbiter_MITUCB_MeshNode();


protected:
	virtual int route(ArbiterRequestMsg* rmsg);

	map<int, pair<int,int> > routingMatrix,routingMatrixAlt;


	enum PORTS
	{
		SW_N = 0,
		SW_S,
		SW_E,
		SW_W,
		SW_Processor,
		SW_Memory
	};
};

#endif
