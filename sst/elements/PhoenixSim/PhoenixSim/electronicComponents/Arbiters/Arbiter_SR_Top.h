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

#ifndef ARBITER_SR_TOP_H_
#define ARBITER_SR_TOP_H_

#include "Arbiter_SR.h"

class Arbiter_SR_Top: public Arbiter_SR {
public:
	Arbiter_SR_Top();
	virtual ~Arbiter_SR_Top();

	virtual void init(string id, int level, int numX, int numY, int vc,
			int ports, int numpse, int buff, string n);

protected:
	virtual int route(ArbiterRequestMsg* rmsg);


private:

	map<int, int> routeMiddle;

};

#endif /* ARBITER_SR_QUAD_H_ */
