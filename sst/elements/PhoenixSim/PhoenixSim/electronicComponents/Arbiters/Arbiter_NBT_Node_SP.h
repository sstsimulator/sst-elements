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

#ifndef ARBITER_NBT_NODE_SP_H_
#define ARBITER_NBT_NODE_SP_H_

#include "Arbiter_NBT_Node.h"

class Arbiter_NBT_Node_SP : public Arbiter_NBT_Node
{
public:
	Arbiter_NBT_Node_SP();
	virtual ~Arbiter_NBT_Node_SP();
protected:
	virtual void PSEsetup(int inport, int outport, PSE_STATE st);



	enum PORTS
	{
		SW_N = 0,
		SW_E,
		SW_S,
		SW_W
	};
};

#endif /* ARBITER_NBT_4X4_SP_H_ */
