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

import electronicComponents.ElectronicRouter;
//
// TODO auto-generated module
//
module SiNRouter
{
    parameters:
        int nodeId;
        int numOfNodesX;
        int numOfNodesY;
        int type;
        
        double SiN_CrosspointSizeX;
        double SiN_CrosspointSizeY;
    gates:
        inout photonicHorizontalIn[numOfNodesX];
        inout photonicHorizontalOut[numOfNodesX];
        inout photonicVerticalIn[numOfNodesY];
        inout photonicVerticalOut[numOfNodesY];
        
        input electronicHorizontalIn[numOfNodesX - 1];
        output electronicHorizontalOut[numOfNodesX - 1];
        input electronicVerticalIn[numOfNodesY - 1];
        output electronicVerticalOut[numOfNodesY - 1];
        
        input electronicGatewayIn;
        output electronicGatewayOut;
        
        
       
    submodules:
    	control : ElectronicRouter 
    	{
         	parameters:
         	    type = type;
            	numPorts = numOfNodesX + numOfNodesY - 2 + 1;
        		numPSEports = numOfNodesX*numOfNodesY;
                id = string(nodeId) + "."; 
                level = "NET";		
                numX = numOfNodesX;
                numY = numOfNodesY;	
    	}
    	
    	
		switch : SiNSwitch
		{
		    parameters:
		    	switchSizeX = numOfNodesY;
        		switchSizeY = numOfNodesX;
        		SiN_CrosspointSizeX = SiN_CrosspointSizeX;
        		SiN_CrosspointSizeY = SiN_CrosspointSizeY;
		}
	
		
	connections:
	    //photonic ports
	    for i = 0 .. numOfNodesX - 1
	    {
	    	switch.horizontalIn[i] <--> photonicHorizontalIn[i];
	    	switch.horizontalOut[i] <--> photonicHorizontalOut[i];
	    }
	    
	    for i = 0 .. numOfNodesY - 1
	    {
	        switch.verticalIn[i] <--> photonicVerticalIn[i];
	        switch.verticalOut[i] <--> photonicVerticalOut[i];
	    }
	    
	    //control ports
	    for i = 0 .. numOfNodesX * numOfNodesY - 1
	    {
	        switch.controlPort[i] <-- control.toPSE[i];
	    }
	    
	    //electronic ports
	    for i = 0 .. numOfNodesX - 2
	    {
    	    control.portIn[i] <-- electronicHorizontalIn[i];
    	    control.portOut[i] --> electronicHorizontalOut[i];
	    }
	    
	    for i = 0 .. numOfNodesY - 2
	    {
	        control.portIn[numOfNodesX - 1 + i] <-- electronicVerticalIn[i];
	        control.portOut[numOfNodesX - 1 + i] --> electronicVerticalOut[i];
	    }
	    
	    electronicGatewayIn --> control.portIn[numOfNodesX + numOfNodesY - 2];
	    electronicGatewayOut <-- control.portOut[numOfNodesX + numOfNodesY - 2];
}
