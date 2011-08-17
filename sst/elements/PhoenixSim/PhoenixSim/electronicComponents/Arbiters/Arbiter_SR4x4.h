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

#ifndef ARBITER_SR4x4_H_
#define ARBITER_SR4x4_H_

#include "Photonic4x4Arbiter.h"

class Arbiter_SR4x4: public Photonic4x4Arbiter {
public:
	Arbiter_SR4x4();
	virtual ~Arbiter_SR4x4();

	virtual void init(string id, int level, int x, int y, int vc, int ports,
			int numpse, int buff_size, string n);

protected:
	virtual int route(ArbiterRequestMsg* rmsg);

	virtual int getUpPort(ArbiterRequestMsg* rmsg, int lev);
	virtual int getDownPort(ArbiterRequestMsg* rmsg, int lev);

private:

	bool isMiddle;

	int **gatewayAccess;
	int ***routeGrid;
	int **routeMiddle;
	int **routeQuad;

};

#endif /* ARBITER_SR_H_ */
