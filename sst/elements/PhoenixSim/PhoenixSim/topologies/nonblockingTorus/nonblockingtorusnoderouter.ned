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

package topologies.nonblockingTorus;

import photonic.switches.NonBlockingSwitch4x4;
import electronicComponents.ElectronicRouter;
//
// TODO auto-generated module
//
module NonblockingTorusNodeRouter
{
    parameters:
        int nodeId;
        int numX;
        
        int type;
    gates:
		inout photonicInN;
        inout photonicInS;
        inout photonicInE;
        inout photonicInW;

        inout photonicOutN;
        inout photonicOutS;
        inout photonicOutE;
        inout photonicOutW;
        
        input electronicInN;
        input electronicInS;
        input electronicInE;
        input electronicInW;

        output electronicOutN;
        output electronicOutS;
        output electronicOutE;
        output electronicOutW;
        
       
    submodules:
    	Nonblocking4x4Control : ElectronicRouter 
    	{
         	parameters:
         	    type = type;
            	numPorts = 4;
        		numPSEports = 6;
                id = string(nodeId) + "."; 
                level = "NET";		
                numX = numX;
                numY = 2;
  
			
    	}
		Nonblocking4x4Switch : NonBlockingSwitch4x4
		{
		}
		
	connections:
	    
        Nonblocking4x4Switch.photonicInN <--> photonicInN;
        Nonblocking4x4Switch.photonicInS <--> photonicInS;
        Nonblocking4x4Switch.photonicInE <--> photonicInE;
        Nonblocking4x4Switch.photonicInW <--> photonicInW;
        
        Nonblocking4x4Switch.photonicOutN <--> photonicOutN;
        Nonblocking4x4Switch.photonicOutS <--> photonicOutS;
        Nonblocking4x4Switch.photonicOutE <--> photonicOutE;
        Nonblocking4x4Switch.photonicOutW <--> photonicOutW;
        
        Nonblocking4x4Switch.controlPSE2x2NE <-- Nonblocking4x4Control.toPSE[0];
        Nonblocking4x4Switch.controlPSE2x2SW <-- Nonblocking4x4Control.toPSE[1];
        Nonblocking4x4Switch.controlPSE1x2N <-- Nonblocking4x4Control.toPSE[2];
        Nonblocking4x4Switch.controlPSE1x2S <-- Nonblocking4x4Control.toPSE[3];
        Nonblocking4x4Switch.controlPSE1x2E <-- Nonblocking4x4Control.toPSE[4];
        Nonblocking4x4Switch.controlPSE1x2W <-- Nonblocking4x4Control.toPSE[5];
        
        Nonblocking4x4Control.portIn[0] <-- electronicInN;
        Nonblocking4x4Control.portIn[1] <-- electronicInS;
        Nonblocking4x4Control.portIn[2] <-- electronicInE;
        Nonblocking4x4Control.portIn[3] <-- electronicInW;

        Nonblocking4x4Control.portOut[0] --> electronicOutN;
        Nonblocking4x4Control.portOut[1] --> electronicOutS;
        Nonblocking4x4Control.portOut[2] --> electronicOutE;
        Nonblocking4x4Control.portOut[3] --> electronicOutW;
        
       
}
