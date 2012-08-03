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

#include "VirtualChannelControl.h"

Define_Module(VirtualChannelControl);

VC_Control* VirtualChannelControl::control = NULL;
int VirtualChannelControl::HIGH_PRIORITY_CHANNEL = 0;


VirtualChannelControl::VirtualChannelControl() {
	// TODO Auto-generated constructor stub

}

VirtualChannelControl::~VirtualChannelControl() {
	// TODO Auto-generated destructor stub
}



void VirtualChannelControl::initialize(){
	int type = par("virtualChannelControl");
	int numVC = par("routerVirtualChannels");

	switch(type){
	case VC_CNTRL_RND:
		control = new VC_Control_Random(numVC);
		break;
	case VC_CNTRL_PHPRI:
		control = new VC_Control_PhotonicPriority(numVC);
		break;
	case VC_CNTRL_RW:
			control = new VC_Control_DRAMreadWrite(numVC);
			break;
	default:
		opp_error("unknown virtual channel controller");

	}
}

void VirtualChannelControl::finish(){

	if(control != NULL)
		delete control;
}
