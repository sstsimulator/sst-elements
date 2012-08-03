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

#ifndef ARBITER_NBT_GATEWAY_H_
#define ARBITER_NBT_GATEWAY_H_

#include "PhotonicArbiter.h"

class Arbiter_NBT_Gateway : public PhotonicArbiter
{
public:
	Arbiter_NBT_Gateway();
	virtual ~Arbiter_NBT_Gateway();

	virtual void init(string id, int level, int x, int y, int vc, int ports, int numpse, int buff_size, string n);

protected:
	virtual int route(ArbiterRequestMsg* rmsg);
	virtual void PSEsetup(int inport, int outport, PSE_STATE st);

	int nodeRow;
	bool swIsN;

	enum PORTS
	{
		  NBMGR_InjectionN = 0,
		  NBMGR_InjectionS,
		  NBMGR_Gateway
	};
};

#endif /* ARBITER_NBT_GW_H_ */
