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

#ifndef ARBITER_SR8X8_H_
#define ARBITER_SR8X8_H_

#include "Photonic4x4Arbiter.h"

class Arbiter_SR8x8: public Photonic4x4Arbiter {
public:
	Arbiter_SR8x8();
	virtual ~Arbiter_SR8x8();

	virtual void init(string id, int level, int numX, int numY, int vc, int ports, int numpse, int buff, string n) ;

protected:
	virtual int route(ArbiterRequestMsg* rmsg);


	virtual int getUpPort(ArbiterRequestMsg* rmsg, int lev);
	virtual int getDownPort(ArbiterRequestMsg* rmsg, int lev);

private:

	bool isMiddle;

	int myGrid;

	map<int, int> oldIDsQuad;
	map<int, int> oldIDsUnit;

	int **gatewayAccess;
	int ***routeGrid;
	int **routeMiddle;
	int **routeQuad;

	int ***routeExpressDirection;
	int ***routeLevelUp;
	int ***routeInterGridExpress;
	int ***routeLevelTwo;

};

#endif /* ARBITER_SR8X8_H_ */
