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

#ifndef ARBITER_OM4X4_H_
#define ARBITER_OM4X4_H_

#include "PhotonicArbiter.h"

class Arbiter_OM4x4: public PhotonicArbiter {
public:
	Arbiter_OM4x4();
	virtual ~Arbiter_OM4x4();

	virtual int route(ArbiterRequestMsg* rmsg);
	virtual void PSEsetup(int inport, int outport, PSE_STATE st);

	//assuming seashell switch
	//ports are numbered 0-3
	//pse's are PSE_row_col
	enum PSE {
		PSE_0_1 = 1,
		PSE_0_2 = 2,
		PSE_0_3 = 3,
		PSE_1_0 = 4,
		PSE_1_2 = 6,
		PSE_1_3 = 7,
		PSE_2_0 = 8,
		PSE_2_1 = 9,
		PSE_2_3 = 11,
		PSE_3_0 = 12,
		PSE_3_1 = 13,
		PSE_3_2 = 14,
	};
};

#endif /* ARBITER_OM4X4_H_ */
