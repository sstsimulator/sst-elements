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

import topologies.nonblockingTorus.NonblockingTorusGatewaySwitch;
import electronicComponents.ElectronicRouter;

//
// TODO auto-generated module
//
module NonblockingTorusGatewayRouter
{
    parameters:
        int nodeId;
        int numX;
        
        int type;
    gates:
		inout photonicInEjection;
        inout photonicInGateway;
        
        inout photonicOutInjectionN;
        inout photonicOutInjectionS;
        inout photonicOutGateway;
        

        
        input electronicInInjectionN;
        input electronicInInjectionS;
        input electronicInGateway;
        
        output electronicOutInjectionN;
        output electronicOutInjectionS;
        output electronicOutGateway;
        
     
    submodules:
    	GatewayControl : ElectronicRouter 
    	{
         	parameters:
         	    type = type;
            	numPorts = 3;
        		numPSEports = 1;
                id = string(floor(nodeId/numX)*2+((nodeId%numX)<(numX>>1)?0:1)) + ".";// gateway ID != node ID, perform translation
                level = "NET";				
                numX = numX;
                numY = 2;
			
    	}
		GatewaySwitch : NonblockingTorusGatewaySwitch
		{
		}
		
	connections:
	    
        photonicInEjection <--> GatewaySwitch.photonicInEjection;
        photonicInGateway <--> GatewaySwitch.photonicInGateway;
        
        photonicOutInjectionN <--> GatewaySwitch.photonicOutInjectionN;
        photonicOutInjectionS <--> GatewaySwitch.photonicOutInjectionS;
        photonicOutGateway <--> GatewaySwitch.photonicOutGateway;
        
        GatewaySwitch.controlPSE1x2 <-- GatewayControl.toPSE[0];
        
        
        electronicInInjectionN --> GatewayControl.portIn[0];
        electronicInInjectionS --> GatewayControl.portIn[1];
        electronicInGateway --> GatewayControl.portIn[2];
        
        electronicOutInjectionN <-- GatewayControl.portOut[0];
        electronicOutInjectionS <-- GatewayControl.portOut[1];
        electronicOutGateway <-- GatewayControl.portOut[2];
        
	
}
