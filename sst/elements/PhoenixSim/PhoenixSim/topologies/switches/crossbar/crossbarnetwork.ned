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

package topologies.switches.crossbar;


import topologies.Gateway_NoMem;
import photonic.switches.CrossbarSwitch;
import electronicComponents.ElectronicRouter;
//
// TODO auto-generated module
//
module CrossbarNetwork
{
    parameters:
        int numOfNodesX;
    gates:
        inout photonicGatewayIn[numOfNodesX];
        inout photonicGatewayOut[numOfNodesX];
        input electronicGatewayIn[numOfNodesX];
        output electronicGatewayOut[numOfNodesX];
    submodules:
        gateway[numOfNodesX]: Gateway_NoMem 
        {
        }
        
        switch: CrossbarSwitch
        {
            parameters:
    			numPorts = numOfNodesX;
    			numPSE = numOfNodesX*numOfNodesX;
    			waveguideSpacing = 0;  //maximal compactness
        }
        
        control : ElectronicRouter {
         	parameters:
         	    type = 1000;
         	  
            	numPorts = numOfNodesX;
        		numPSEports = numOfNodesX*numOfNodesX;
        		
                id = string(0) + ".";
                level = "NET";
                
                numX = numOfNodesX;
                numY = numOfNodesX;
                
                switchVariant = 0;
               
        }
       
	connections allowunconnected:
	    
	    for i = 0..numOfNodesX*numOfNodesX - 1
	    {
	        switch.cntrl[i] <-- control.toPSE[i];
	    }
	    
	    for i = 0..numOfNodesX-1
	    {
	        gateway[i].in[1] <--> switch.out[i];
	     	gateway[i].out[1] <--> switch.in[i];
	     	
	     	gateway[i].in[0] <--> photonicGatewayIn[i];
	     	gateway[i].out[0] <--> photonicGatewayOut[i];
	     	
	     	control.portIn[i] <-- electronicGatewayIn[i];
	     	control.portOut[i] --> electronicGatewayOut[i];
	    }
	    
        

}
