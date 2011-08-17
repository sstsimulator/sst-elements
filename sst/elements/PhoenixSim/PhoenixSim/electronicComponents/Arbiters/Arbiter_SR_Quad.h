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

#ifndef ARBITER_SR_QUAD_H_
#define ARBITER_SR_QUAD_H_

#include "Arbiter_SR.h"

class Arbiter_SR_Quad: public Arbiter_SR {
public:
	Arbiter_SR_Quad();
	virtual ~Arbiter_SR_Quad();

	virtual void init(string id, int level, int numX, int numY, int vc,
			int ports, int numpse, int buff, string n);

protected:
	virtual int route(ArbiterRequestMsg* rmsg);

	virtual int getUpPort(ArbiterRequestMsg* rmsg, int lev);
	virtual int getDownPort(ArbiterRequestMsg* rmsg, int lev);




private:

	int myEight;
	int myGrid;
	int myQuad;
	int myUnit;

	int gatewayAccess;
	map<int, int> routeQuad;
	map<int, int> quadExpressPort;

	map<int, int> gridExpressQuad;
	int gridExpressPort;

	int toMiddle;

};

#endif /* ARBITER_SR_QUAD_H_ */
