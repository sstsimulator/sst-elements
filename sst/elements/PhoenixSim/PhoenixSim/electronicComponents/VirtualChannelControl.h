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

#ifndef VIRTUALCHANNELCONTROL_H_
#define VIRTUALCHANNELCONTROL_H_

#include <omnetpp.h>

#include "VC_Control.h"

using namespace std;



class VirtualChannelControl : public cSimpleModule {
public:
	VirtualChannelControl();
	virtual ~VirtualChannelControl();

	virtual void initialize();
	virtual void finish();

	static VC_Control* control;

	static int HIGH_PRIORITY_CHANNEL;

	enum CntrlTypes{
		VC_CNTRL_RND = 0,
		VC_CNTRL_PHPRI = 1,
		VC_CNTRL_RW =  2     //split them for DRAM reads/writes
	};
};

#endif /* VIRTUALCHANNELCONTROL_H_ */
