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

import topologies.blockingTorus.InjectionSwitch;
import electronicComponents.ElectronicRouter;
//
// TODO auto-generated module
//
module BlockingTorusInjectionRouter
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
    	InjectionControl : ElectronicRouter 
    	{
         	parameters:
         	    type = type;
            	numPorts = 4;
        		numPSEports = 4;
                  id = networkPrefix + string(nodeId) + ".";
                 level = "NET";
                 
                
                  numX = numX;
                numY = numY;
  
		
    	}
		InjectionSwitch : InjectionSwitch
		{
		}
	
	connections:
	    
        InjectionSwitch.photonicInN <--> photonicInN;
        InjectionSwitch.photonicInS <--> photonicInS;
        InjectionSwitch.photonicInE <--> photonicInE;
        InjectionSwitch.photonicInW <--> photonicInW;
        
        InjectionSwitch.photonicOutN <--> photonicOutN;
        InjectionSwitch.photonicOutS <--> photonicOutS;
        InjectionSwitch.photonicOutE <--> photonicOutE;
        InjectionSwitch.photonicOutW <--> photonicOutW;
        
        InjectionSwitch.controlPSE1x2_NE <-- InjectionControl.toPSE[0];
        InjectionSwitch.controlPSE1x2_NW <-- InjectionControl.toPSE[1];
        InjectionSwitch.controlPSE1x2_SE <-- InjectionControl.toPSE[2];
        InjectionSwitch.controlPSE1x2_SW <-- InjectionControl.toPSE[3];
        
        InjectionControl.portIn[0] <-- electronicInN;
        InjectionControl.portIn[1] <-- electronicInS;
        InjectionControl.portIn[2] <-- electronicInE;
        InjectionControl.portIn[3] <-- electronicInW;

        InjectionControl.portOut[0] --> electronicOutN;
        InjectionControl.portOut[1] --> electronicOutS;
        InjectionControl.portOut[2] --> electronicOutE;
        InjectionControl.portOut[3] --> electronicOutW;
        
     
}
