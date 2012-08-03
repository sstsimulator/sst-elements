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

package topologies.blockingTorus;

import topologies.blockingTorus.GatewaySwitch*;
import electronicComponents.ElectronicRouter;


module BlockingTorusGatewayRouter
{
    parameters:
        int nodeId;
        
        int numX;
        int numY;
        

        
        string networkPrefix;
        
    gates:
	    inout photonicInInjectionN;
        inout photonicInInjectionS;
        inout photonicInEjection;
		inout photonicInGateway;
		
        inout photonicOutInjectionN;
        inout photonicOutInjectionS;
        inout photonicOutGateway;
        
        input electronicInInjectionN;
        input electronicInInjectionS;
        input electronicInEjection;
        input electronicInGateway;

        output electronicOutInjectionN;
        output electronicOutInjectionS;
        output electronicOutEjection;
        output electronicOutGateway;
  
        
	submodules:
    	GatewayControl : ElectronicRouter 
    	{
         	parameters:
         	    type = 12;
            	numPorts = 4;
        		numPSEports = 2;
                  id = networkPrefix + string(nodeId) + ".";
                 level = "NET";
               
                 
                numX = numX;
                numY = numY;
  
			
    	}
		GatewaySwitch : GatewaySwitch
		{
		}
		
	connections allowunconnected:
	    GatewaySwitch.photonicInInjectionN <--> photonicInInjectionN;
        GatewaySwitch.photonicInInjectionS <--> photonicInInjectionS;
        GatewaySwitch.photonicInEjection <--> photonicInEjection;
        GatewaySwitch.photonicInGateway <--> photonicInGateway;
        
        GatewaySwitch.photonicOutInjectionN <--> photonicOutInjectionN;
        GatewaySwitch.photonicOutInjectionS <--> photonicOutInjectionS;
        GatewaySwitch.photonicOutGateway <--> photonicOutGateway;
        
        GatewaySwitch.controlPSE1x2_A <-- GatewayControl.toPSE[0];
        GatewaySwitch.controlPSE1x2_B <-- GatewayControl.toPSE[1];
        
        GatewayControl.portIn[0] <-- electronicInInjectionN;
        GatewayControl.portIn[1] <-- electronicInInjectionS;
        GatewayControl.portIn[2] <-- electronicInEjection;
        GatewayControl.portIn[3] <-- electronicInGateway;

        GatewayControl.portOut[0] --> electronicOutInjectionN;
        GatewayControl.portOut[1] --> electronicOutInjectionS;
        GatewayControl.portOut[2] --> electronicOutEjection;
        GatewayControl.portOut[3] --> electronicOutGateway;
        
     
}


module BlockingTorusGatewayRouterRight
{
    parameters:
        int nodeId;
         int numX;
        int numY;
        
      
      
       	string networkPrefix;
       	
    gates:
	    inout photonicInInjectionN;
        inout photonicInInjectionS;
        inout photonicInEjection;
		inout photonicInGateway;
		
        inout photonicOutInjectionN;
        inout photonicOutInjectionS;
        inout photonicOutGateway;
        
        input electronicInInjectionN;
        input electronicInInjectionS;
        input electronicInEjection;
        input electronicInGateway;

        output electronicOutInjectionN;
        output electronicOutInjectionS;
        output electronicOutEjection;
        output electronicOutGateway;
   
        
	submodules:
    	GatewayControl : ElectronicRouter 
    	{
         	parameters:
         	    type = 12;
            	numPorts = 4;
        		numPSEports = 2;
                 id = networkPrefix + string(nodeId) + ".";
                 level = "NET";
              
      			numX = numX;
                numY = numY;
			
    	}
		GatewaySwitch : GatewaySwitchRight
		{
		}
		
	connections allowunconnected:
	    GatewaySwitch.photonicInInjectionN <--> photonicInInjectionN;
        GatewaySwitch.photonicInInjectionS <--> photonicInInjectionS;
        GatewaySwitch.photonicInEjection <--> photonicInEjection;
        GatewaySwitch.photonicInGateway <--> photonicInGateway;
        
        GatewaySwitch.photonicOutInjectionN <--> photonicOutInjectionN;
        GatewaySwitch.photonicOutInjectionS <--> photonicOutInjectionS;
        GatewaySwitch.photonicOutGateway <--> photonicOutGateway;
        
        GatewaySwitch.controlPSE1x2_A <-- GatewayControl.toPSE[0];
        GatewaySwitch.controlPSE1x2_B <-- GatewayControl.toPSE[1];
        
        GatewayControl.portIn[0] <-- electronicInInjectionN;
        GatewayControl.portIn[1] <-- electronicInInjectionS;
        GatewayControl.portIn[2] <-- electronicInEjection;
        GatewayControl.portIn[3] <-- electronicInGateway;

        GatewayControl.portOut[0] --> electronicOutInjectionN;
        GatewayControl.portOut[1] --> electronicOutInjectionS;
        GatewayControl.portOut[2] --> electronicOutEjection;
        GatewayControl.portOut[3] --> electronicOutGateway;
        
      
}
