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

#ifndef _ARBITER_XB_GATEWAY_H_
#define _ARBITER_XB_GATEWAY_H_

#include "PhotonicArbiter.h"

class Arbiter_XB_Gateway : public PhotonicArbiter {
public:
	Arbiter_XB_Gateway();
	virtual ~Arbiter_XB_Gateway();

	virtual int route(ArbiterRequestMsg* rmsg);
	virtual void PSEsetup(int inport, int outport, PSE_STATE st);

	enum PORTS {
		GWC_Node=0,
		GWC_Net,
		GWC_Mem
	};

	enum PSE {
		PSE_output=0,
		PSE_input
	};
};

#endif /* ARBITER_GWC_H_ */
