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
import topologies.blockingTorus.EjectionSwitch*;
import electronicComponents.ElectronicRouter;
//
// TODO auto-generated module
//
module BlockingTorusEjectionRouter
{
    parameters:
        int nodeId;
         int numX;
        int numY;
        
       
 
        
        string networkPrefix;
        
       	int type;
    gates:
	    inout photonicInN;
        inout photonicInS;

        inout photonicOutN;
        inout photonicOutS;
        inout photonicOutGateway;
        
        input electronicInN;
        input electronicInS;
        input electronicInGateway;

        output electronicOutN;
        output electronicOutS;
        output electronicOutGateway;
        
 
        
	submodules:
    	EjectionControl : ElectronicRouter 
    	{
         	parameters:
         	    type = type;
            	numPorts = 3;
        		numPSEports = 2;
                 id = networkPrefix + string(nodeId) + ".";
                 level = "NET";
             
                
                 numX = numX;
                 numY = numY;
  
			
    	}
		EjectionSwitch : EjectionSwitch
		{
		}
		
		connections allowunconnected:
		EjectionSwitch.photonicInN <--> photonicInN;
        EjectionSwitch.photonicInS <--> photonicInS;
        
        EjectionSwitch.photonicOutN <--> photonicOutN;
        EjectionSwitch.photonicOutS <--> photonicOutS;
        EjectionSwitch.photonicOutGateway <--> photonicOutGateway;
        
        EjectionSwitch.controlPSE1x2_A <-- EjectionControl.toPSE[0];
        EjectionSwitch.controlPSE1x2_B <-- EjectionControl.toPSE[1];
        
        EjectionControl.portIn[0] <-- electronicInN;
        EjectionControl.portIn[1] <-- electronicInS;
        EjectionControl.portIn[2] <-- electronicInGateway;

        EjectionControl.portOut[0] --> electronicOutN;
        EjectionControl.portOut[1] --> electronicOutS;
        EjectionControl.portOut[2] --> electronicOutGateway;
        
     
		    
}


module BlockingTorusEjectionRouterRight
{
    parameters:
        int nodeId;
         int numX;
        int numY;
      
        
        string networkPrefix;
        
        int type;
    gates:
	    inout photonicInN;
        inout photonicInS;

        inout photonicOutN;
        inout photonicOutS;
        inout photonicOutGateway;
        
        input electronicInN;
        input electronicInS;
        input electronicInGateway;

        output electronicOutN;
        output electronicOutS;
        output electronicOutGateway;
    
        
	submodules:
    	EjectionControl : ElectronicRouter 
    	{
         	parameters:
         	    type = type;
            	numPorts = 3;
        		numPSEports = 2;
                  id = networkPrefix + string(nodeId) + ".";
                 level = "NET";
               
    			numX = numX;
                numY = numY;
			
    	}
		EjectionSwitch : EjectionSwitchRight
		{
		}
		
		connections allowunconnected:
		EjectionSwitch.photonicInN <--> photonicInN;
        EjectionSwitch.photonicInS <--> photonicInS;
        
        EjectionSwitch.photonicOutN <--> photonicOutN;
        EjectionSwitch.photonicOutS <--> photonicOutS;
        EjectionSwitch.photonicOutGateway <--> photonicOutGateway;
        
        EjectionSwitch.controlPSE1x2_A <-- EjectionControl.toPSE[0];
        EjectionSwitch.controlPSE1x2_B <-- EjectionControl.toPSE[1];
        
        EjectionControl.portIn[0] <-- electronicInN;
        EjectionControl.portIn[1] <-- electronicInS;
        EjectionControl.portIn[2] <-- electronicInGateway;

        EjectionControl.portOut[0] --> electronicOutN;
        EjectionControl.portOut[1] --> electronicOutS;
        EjectionControl.portOut[2] --> electronicOutGateway;
        
      
		    
}
