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

#ifndef ARBITER_BT_4X4_H_
#define ARBITER_BT_4X4_H_

#include "PhotonicNoUturnArbiter.h"

class Arbiter_BT_4x4 : public PhotonicNoUturnArbiter {
public:
	Arbiter_BT_4x4();
	virtual ~Arbiter_BT_4x4();

	virtual void init(string id, int level, int x, int y, int vc, int ports, int numpse, int buff, string n);

protected:
	virtual int route(ArbiterRequestMsg* rmsg);
	void PSEsetup(int inport, int outport, PSE_STATE st);

	virtual bool isDimensionChange(int inport, int outport);

	bool isConnected;
	bool GWisN;



	enum PORTS {
		SW_N = 0,
		SW_S,
		SW_E,
		SW_W
	};
};

#endif /* ARBITER_NBT_4X4_H_ */
