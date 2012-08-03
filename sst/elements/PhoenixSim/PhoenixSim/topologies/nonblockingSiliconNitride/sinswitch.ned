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

package topologies.nonblockingSiliconNitride;

import photonic.devices.PSE1x2;

//
// TODO auto-generated module
//
module SiNSwitch
{
    parameters:
        double switchSizeX;
        double switchSizeY;
        double SiN_CrosspointSizeX;
        double SiN_CrosspointSizeY;
    gates:
        inout horizontalIn[switchSizeY];
        inout horizontalOut[switchSizeY];
        inout verticalIn[switchSizeX];
        inout verticalOut[switchSizeX];
        
        input controlPort[switchSizeX*switchSizeY];
	submodules:
	    point[switchSizeX*switchSizeY]: PSE1x2
	    {
	        parameters:
	            PropagationLoss;
        		PassByRingLoss;
        		PassThroughRingLoss;
        		CrossingLoss;
        		Crosstalk_Cross;
        
        		RingOn_ER_1x2;
        		RingOff_ER_1x2;
        		ThroughDelay_1x2;
				DropDelay_1x2;
				RingStaticDissipation;
				RingDynamicOffOn;
				RingDynamicOnOff;

				//ringDiameter = default(50); // Assumes a 50 um circular ring
	    }
	connections:
	    for i = 0 .. switchSizeX - 1
	    {
	        verticalIn[i] <--> point[(switchSizeY-1)*switchSizeX + i].photonicVertIn;
	        verticalOut[i] <--> point[i].photonicVertOut;
	    }
	    
	    for i = 0 .. switchSizeY - 1
	    {
	        horizontalIn[i] <--> point[switchSizeX * i].photonicHorizIn;
	        horizontalOut[i] <--> point[switchSizeX * i + switchSizeX - 1].photonicHorizOut;
	    }
	    
	    for i = 0 .. switchSizeX * switchSizeY - 1
	    {
	        point[i].photonicHorizOut <--> point[i+1].photonicHorizIn if i%switchSizeX != switchSizeX - 1;
	        point[i].photonicVertOut <--> point[i - switchSizeX].photonicVertIn if int(i/switchSizeX) > 0;
	        
	        controlPort[i] --> point[i].fromRouter;
	    }
}
