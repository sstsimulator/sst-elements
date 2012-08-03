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

package topologies.photonicTDM;

import processingPlane.ProcessingPlane_ProcRoute;
import ioComponents.IOPlane_Photonic;

import statistics.Statistics;
import electronicComponents.VirtualChannelControl;

import electronicComponents.inoutConverter;

import photonic.devices.*;

import ioComponents.DRAM_LRL.*;

import topologies.Gateway;

import photonic.PhotonicSwitch;
//
// TODO auto-generated module
//
network PhotonicMeshTDMNetwork
{
        parameters:
        int numOfNodesX;
        int numOfNodesY;

		bool useIOplane;

		bool enhancedTDM;
		
		double IL_Facet_Vertical;
		
		string gateway = (useIOplane) ? "Gateway" : "Gateway_NoMem";
		
        @display("bgb=447,513");
    submodules:
        stat : Statistics {
            
        }
        
        vc : VirtualChannelControl {
        }
        
        prPlane: ProcessingPlane_ProcRoute
        {
            parameters:
        		numNodesX = numOfNodesX;
           		numNodesY = numOfNodesY;
           	  	networkPrefix = "";
           	  	numNetPorts = 7;
           	  	autounblock = !enhancedTDM;
           	 
              
        }

        phPlane: PhotonicMeshTDMPlane 
        {
            parameters:
                tdmSwitch = (enhancedTDM) ? "EtdmSwitchMesh" : "tdmSwitchMesh";
        }
        
        //ioPlane[(useIOplane ? 1 : 0)]: IOPlane_Photonic {
        //    parameters:
        //        numOfPorts = numOfNodesX*2+(numOfNodesY-2)*2;   //io connections on periphery
          
        //}
        
        dimms[(useIOplane ? numOfNodesX * numOfNodesY : 0)] : DRAM_OCM {
            
        }
        
        
        gateway[numOfNodesX * numOfNodesY] : <gateway> like PhotonicSwitch {
            
        }
        
     
       

 		conv[numOfNodesX*numOfNodesY] : inoutConverter {
            
        }
        
        fIn[(useIOplane ? numOfNodesX * numOfNodesY : 0)]: Facet {
            IL_Facet = IL_Facet_Vertical;
            @display("p=215,244");
        }
        fOut[(useIOplane ? numOfNodesX * numOfNodesY : 0)]: Facet {
             IL_Facet = IL_Facet_Vertical;
            @display("p=215,88");
        }
       

    connections allowunconnected:
        for i=0..numOfNodesX*numOfNodesY-1 {
            prPlane.dataIn[7*i] <-- conv[i].out;
            conv[i].io_in <--> gateway[i].out[0];
            
            prPlane.dataOut[7*i] --> conv[i].in;
            conv[i].io_out <--> gateway[i].in[0];
            
           	gateway[i].out[1] <--> phPlane.photonicIn[i];
           	gateway[i].in[1] <--> phPlane.photonicOut[i];
           	
           	dimms[i].dataIn <--> fOut[i].photonicOut if useIOplane;
           	fOut[i].photonicIn <--> gateway[i].out[2] if useIOplane;
           	dimms[i].dataOut <--> fIn[i].photonicIn if useIOplane;
           	fIn[i].photonicOut <-->  gateway[i].in[2] if useIOplane;
           	
           	gateway[i].cntrl[0] <-- prPlane.dataOut[7*i + 1] if useIOplane;
           	gateway[i].cntrl[1] <-- prPlane.dataOut[7*i + 2] if useIOplane;
           	gateway[i].cntrl[2] <-- prPlane.dataOut[7*i + 3] if useIOplane;
           	gateway[i].cntrl[3] <-- prPlane.dataOut[7*i + 4] if useIOplane;
           	gateway[i].cntrl[4] <-- prPlane.dataOut[7*i + 5] if useIOplane;
           	gateway[i].cntrl[5] <-- prPlane.dataOut[7*i + 6] if useIOplane;
           	
         
        }


//  		//going off chip
//        for i=0..numOfNodesX-1 {
//            //--top row
//            phPlane.photonicMemoryIn[i] <--> ioPlane[0].phOut[i] if (useIOplane);
//            phPlane.photonicMemoryOut[i] <--> ioPlane[0].phIn[i] if (useIOplane);
//			
//			phPlane.electronicMemoryIn[i] <-- ioPlane[0].elOut[i] if (useIOplane);
//			phPlane.electronicMemoryOut[i] --> ioPlane[0].elIn[i] if (useIOplane);
//        }
//
//        for i=0..numOfNodesX-1 {
//            //--bottom row
//            phPlane.photonicMemoryIn[i+numOfNodesX*(numOfNodesY-1)] <--> ioPlane[0].phOut[i+numOfNodesX] if (useIOplane);
//            phPlane.photonicMemoryOut[i+numOfNodesX*(numOfNodesY-1)] <--> ioPlane[0].phIn[i+numOfNodesX] if (useIOplane);
//
//			phPlane.electronicMemoryIn[i+numOfNodesX*(numOfNodesY-1)] <-- ioPlane[0].elOut[i+numOfNodesX] if (useIOplane);
//			phPlane.electronicMemoryOut[i+numOfNodesX*(numOfNodesY-1)] --> ioPlane[0].elIn[i+numOfNodesX] if (useIOplane);
//        }
//
//        for j=1..numOfNodesY-2 {
//            //--left side
//            phPlane.photonicMemoryIn[numOfNodesX*j] <--> ioPlane[0].phOut[(j-1)+numOfNodesX*2] if (useIOplane);
//            phPlane.photonicMemoryOut[numOfNodesX*j] <--> ioPlane[0].phIn[(j-1)+numOfNodesX*2] if (useIOplane);
//
//			phPlane.electronicMemoryIn[numOfNodesX*j] <-- ioPlane[0].elOut[(j-1)+numOfNodesX*2] if (useIOplane);
//			phPlane.electronicMemoryOut[numOfNodesX*j] --> ioPlane[0].elIn[(j-1)+numOfNodesX*2] if (useIOplane);
//        }
//
//        for j=1..numOfNodesY-2 {
//            //--right side
//            phPlane.photonicMemoryIn[(numOfNodesX-1)+numOfNodesX*j] <--> ioPlane[0].phOut[(((j-1)+numOfNodesX*2)+numOfNodesY)-2] if (useIOplane);
//            phPlane.photonicMemoryOut[(numOfNodesX-1)+numOfNodesX*j] <--> ioPlane[0].phIn[(((j-1)+numOfNodesX*2)+numOfNodesY)-2] if (useIOplane);
//
//			phPlane.electronicMemoryIn[(numOfNodesX-1)+numOfNodesX*j] <-- ioPlane[0].elOut[(((j-1)+numOfNodesX*2)+numOfNodesY)-2] if (useIOplane);
//			phPlane.electronicMemoryOut[(numOfNodesX-1)+numOfNodesX*j] --> ioPlane[0].elIn[(((j-1)+numOfNodesX*2)+numOfNodesY)-2] if (useIOplane);
//		}
}
