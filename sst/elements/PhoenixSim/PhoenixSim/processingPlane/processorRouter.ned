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

package processingPlane;

import electronicComponents.RouterCrossbar;
import electronicComponents.RouterArbiter;
import electronicComponents.mux;
import electronicComponents.RouterStat;
import electronicComponents.inoutConverter;
import electronicComponents.demux;

simple ProcessorRouterArbiter
{
    parameters:
        string id;
        
        string level;
        string networkProfile;
        //int numOfRoutingLevels;
        
        int processorConcentration;
        string addressTranslator;
        
        int routerMaxGrants;
        
        int numOfNodesX;
        int numOfNodesY;
        
       
    	
    	
        int numX;
        int numY;
		int type;
		int numPorts;
		int numPSE;
		
		int switchVariant;
		
		
		
		int routerVirtualChannels;
		int electronicChannelWidth;
		//double packetTimeout;
		double clockRate;
		int routerBufferSize;

        @display("i=block/cogwheel");
    gates:
        inout req[numPorts];

        inout XbarCntrl;
		output pseCntrl[numPSE];
		
        input ackIn[numPorts];
         
}

module ProcessorRouter
{
    parameters:
        string id;
         bool autounblock;
        double clockRate;
        
        string level;
        int numOfNodesX;
        int numOfNodesY;
        
        int processorConcentration;
        
        double O_frequency_cntrl;
		double O_frequency_data;
		double O_frequency_proc;
		
		int electronicChannelWidth;
		int numOfWavelengthChannels;
		
		
		int numX;
		int numY;

        @display("i=abstract/router");
    gates:
        input portIn[processorConcentration + 1];
        inout req[processorConcentration + 1];
        output portOut[processorConcentration + 1];

        


    submodules:

        xbar: RouterCrossbar {
            parameters:
				numPorts = processorConcentration + 1;
                @display("p=133,152");
                
                clockRate = clockRate;
                isProcPlane = true;
                 autounblock = autounblock;
                 electronicChannelWidth = (O_frequency_data / O_frequency_proc) * numOfWavelengthChannels;
          
        }

        arb: ProcessorRouterArbiter {
			 parameters:
			     id = id;
			     type = 5;
			     
			     level = level;
			     
			     numPorts = processorConcentration + 1;
			     numPSE = 0;
			     
			     numX = numX;
			     numY = numY;
			     
			      clockRate = clockRate;
			      
		
			      
			      switchVariant = 0;
			      
			     @display("p=94,53");
        }

		stat : RouterStat {
		   parameters:
		        clockRate = clockRate;
		       numPorts = processorConcentration + 1;   
		}
		
	
		

    connections allowunconnected:
        for i=0..processorConcentration {
    
            portIn[i] --> xbar.in[i];
            xbar.out[i] --> portOut[i];
            

            arb.req[i] <--> req[i];
            
        }

        arb.XbarCntrl <--> xbar.cntrl;

}
