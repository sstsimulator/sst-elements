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

package topologies.switches.machzehndercrossbar;

import photonic.switches.machzehnder.MZCrossbarSwitch;
import electronicComponents.ElectronicRouter;
//
// TODO auto-generated module
//
module MZCrossbarSwitchRouter
{
    parameters:
        int numOfNodesX;
    gates:
		inout photonicIn[numOfNodesX];
        inout photonicOut[numOfNodesX];
        
        input electronicIn[numOfNodesX];
        output electronicOut[numOfNodesX];
        submodules:
    	swc : ElectronicRouter 
    	{
         	parameters:
         	    type = 1000;
            	numPorts = numOfNodesX;
        		numPSEports = numOfNodesX*numOfNodesX*2;
                id = "-1.";// gateway ID != node ID, perform translation	
                level = "NET";			
                numX = numOfNodesX;
                numY = numOfNodesX;
                switchVariant = 1;
			gates:
			    portIn[numOfNodesX];
			    portOut[numOfNodesX];
			    toPSE[2*numOfNodesX*numOfNodesX];
    	}
		sw : MZCrossbarSwitch
		{
		    parameters:
		        numInputs = numOfNodesX;
		        numOutputs = numOfNodesX;
		        waveguideSpacing = 0;
		}
		
		connections:
		    	    for i = 0..numOfNodesX-1
	    {
	        photonicIn[i] <--> sw.in[i];
	        photonicOut[i] <--> sw.out[i];
	        
	        electronicIn[i] --> swc.portIn[i];
	        electronicOut[i] <-- swc.portOut[i];
	        
	    }
            
        for i = 0 .. numOfNodesX*numOfNodesX*2 - 1
        {
            swc.toPSE[i] --> sw.control[i];
        }
}
