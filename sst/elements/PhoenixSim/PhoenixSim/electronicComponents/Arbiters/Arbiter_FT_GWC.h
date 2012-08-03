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

#ifndef ARBITER_FT_GWC_H_
#define ARBITER_FT_GWC_H_

#include "PhotonicNoUturnArbiter.h"

class Arbiter_FT_GWC : public PhotonicNoUturnArbiter {
public:
	Arbiter_FT_GWC();
	virtual ~Arbiter_FT_GWC();

	virtual int route(ArbiterRequestMsg* rmsg);
	virtual void PSEsetup(int inport, int outport, PSE_STATE st);

	virtual int getUpPort(ArbiterRequestMsg* rmsg, int lev);
	virtual int getDownPort(ArbiterRequestMsg* rmsg, int lev);


	enum PORTS {
		SW_N = 0,
		SW_E = 1,
		SW_S = 2,
		SW_W = 3
	};


};

#endif /* ARBITER_GWC_H_ */
