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

#ifndef _ARBITER_BT_GWS_H_
#define _ARBITER_BT_GWS_H_

#include "PhotonicArbiter.h"

class Arbiter_BT_GWS : public PhotonicArbiter {
public:
	Arbiter_BT_GWS();
	virtual ~Arbiter_BT_GWS();

protected:
	virtual int route(ArbiterRequestMsg* rmsg);
	virtual void PSEsetup(int inport, int outport, PSE_STATE st);

	virtual bool isDimensionChange(int inport, int outport);

	enum PORTS {
		 G_InjectionN = 0,
		 G_InjectionS,
		 G_Ejection,
		 G_Gateway
	};
};

#endif /* ARBITER_BT_GW_H_ */
