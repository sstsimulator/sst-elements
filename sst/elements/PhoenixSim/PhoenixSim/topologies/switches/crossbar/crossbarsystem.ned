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

import processingPlane.ProcessingPlane_ProcRoute;

import statistics.Statistics;
import electronicComponents.VirtualChannelControl;
import electronicComponents.inoutConverter;
//
// TODO auto-generated module
//
network CrossbarSystem
{
    parameters:
        int numOfNodesX;
    submodules:
    	stat : Statistics 
    	{
            
        }
        
        vc : VirtualChannelControl {
        }
        
        prPlane: ProcessingPlane_ProcRoute
        {
            parameters:
        		numNodesX = numOfNodesX;
           		numNodesY = 1;
           	  	networkPrefix = "";
           	  	numNetPorts = 2;
              
        }

        phPlane: CrossbarNetwork 
        {
            parameters:
            	numOfNodesX = numOfNodesX;
        }
        
 		conv[numOfNodesX] : inoutConverter {
            
        }
    connections allowunconnected:
        for i = 0..numOfNodesX-1
        {
            prPlane.dataIn[i*2] <-- phPlane.electronicGatewayOut[i];
            prPlane.dataOut[i*2] --> phPlane.electronicGatewayIn[i];
            
            prPlane.dataIn[i*2+1] <-- conv[i].out;
            conv[i].io_in <--> phPlane.photonicGatewayOut[i];
            prPlane.dataOut[i*2+1] --> conv[i].in;
            conv[i].io_out <--> phPlane.photonicGatewayIn[i];
        }
}
