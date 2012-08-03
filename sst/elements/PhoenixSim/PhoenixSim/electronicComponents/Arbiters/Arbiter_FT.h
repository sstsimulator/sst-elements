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

#ifndef ARBITER_FT_H_
#define ARBITER_FT_H_

#include "PhotonicNoUturnArbiter.h"

class Arbiter_FT: public PhotonicNoUturnArbiter {
public:
	Arbiter_FT();
	virtual ~Arbiter_FT();

	virtual int route(ArbiterRequestMsg* rmsg);
	virtual void PSEsetup(int inport, int outport, PSE_STATE st);
	virtual bool isDimensionChange(int in, int out);

	virtual int getUpPort(ArbiterRequestMsg* rmsg, int lev);
	virtual int getDownPort(ArbiterRequestMsg* rmsg, int lev);


	static bool seeded;


	enum PORTS {
		FT_UP0,
		FT_UP1,
		FT_DN0,
		FT_DN1
	};

	enum PSE {
		PSE_output=0,
		PSE_input
	};

};

#endif /* ARBITER_FT_H_ */
