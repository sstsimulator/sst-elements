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

package topologies.photonicMesh;

import processingPlane.ProcessingPlane_ProcRoute;
import ioComponents.IOPlane_Photonic;

import statistics.Statistics;
import electronicComponents.VirtualChannelControl;

import electronicComponents.inoutConverter;
//
// TODO auto-generated module
//
network PhotonicMeshNetwork
{
        parameters:
        int numOfNodesX;
        int numOfNodesY;

		bool useThermalModel;
		bool useIOplane;



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
              numNetPorts = 2;
        }

        phPlane: PhotonicMeshPlane 
        {
        }
        
        ioPlane[(useIOplane ? 1 : 0)]: IOPlane_Photonic {
            parameters:
                numOfPorts = numOfNodesX*2+(numOfNodesY-2)*2;   //io connections on periphery
          
        }

 		conv[numOfNodesX*numOfNodesY] : inoutConverter {
            
        }
		
		thermal[(useThermalModel?1:0)]: ThermalModel
        {
        }

    connections allowunconnected:
        for i=0..numOfNodesX*numOfNodesY-1 {
            prPlane.dataIn[i*2+1] <-- conv[i].out;
            conv[i].io_in <--> phPlane.photonicGatewayOut[i];
            prPlane.dataOut[i*2+1] --> conv[i].in;
            conv[i].io_out <--> phPlane.photonicGatewayIn[i];
            
            prPlane.dataIn[i*2] <-- phPlane.electronicGatewayOut[i];
            prPlane.dataOut[i*2] --> phPlane.electronicGatewayIn[i];
        }


  		//going off chip
        for i=0..numOfNodesX-1 {
            //--top row
            phPlane.photonicMemoryIn[i] <--> ioPlane[0].phOut[i] if (useIOplane);
            phPlane.photonicMemoryOut[i] <--> ioPlane[0].phIn[i] if (useIOplane);
			
			phPlane.electronicMemoryIn[i] <-- ioPlane[0].elOut[i] if (useIOplane);
			phPlane.electronicMemoryOut[i] --> ioPlane[0].elIn[i] if (useIOplane);
        }

        for i=0..numOfNodesX-1 {
            //--bottom row
            phPlane.photonicMemoryIn[i+numOfNodesX*(numOfNodesY-1)] <--> ioPlane[0].phOut[i+numOfNodesX] if (useIOplane);
            phPlane.photonicMemoryOut[i+numOfNodesX*(numOfNodesY-1)] <--> ioPlane[0].phIn[i+numOfNodesX] if (useIOplane);

			phPlane.electronicMemoryIn[i+numOfNodesX*(numOfNodesY-1)] <-- ioPlane[0].elOut[i+numOfNodesX] if (useIOplane);
			phPlane.electronicMemoryOut[i+numOfNodesX*(numOfNodesY-1)] --> ioPlane[0].elIn[i+numOfNodesX] if (useIOplane);
        }

        for j=1..numOfNodesY-2 {
            //--left side
            phPlane.photonicMemoryIn[numOfNodesX*j] <--> ioPlane[0].phOut[(j-1)+numOfNodesX*2] if (useIOplane);
            phPlane.photonicMemoryOut[numOfNodesX*j] <--> ioPlane[0].phIn[(j-1)+numOfNodesX*2] if (useIOplane);

			phPlane.electronicMemoryIn[numOfNodesX*j] <-- ioPlane[0].elOut[(j-1)+numOfNodesX*2] if (useIOplane);
			phPlane.electronicMemoryOut[numOfNodesX*j] --> ioPlane[0].elIn[(j-1)+numOfNodesX*2] if (useIOplane);
        }

        for j=1..numOfNodesY-2 {
            //--right side
            phPlane.photonicMemoryIn[(numOfNodesX-1)+numOfNodesX*j] <--> ioPlane[0].phOut[(((j-1)+numOfNodesX*2)+numOfNodesY)-2] if (useIOplane);
            phPlane.photonicMemoryOut[(numOfNodesX-1)+numOfNodesX*j] <--> ioPlane[0].phIn[(((j-1)+numOfNodesX*2)+numOfNodesY)-2] if (useIOplane);

			phPlane.electronicMemoryIn[(numOfNodesX-1)+numOfNodesX*j] <-- ioPlane[0].elOut[(((j-1)+numOfNodesX*2)+numOfNodesY)-2] if (useIOplane);
			phPlane.electronicMemoryOut[(numOfNodesX-1)+numOfNodesX*j] --> ioPlane[0].elIn[(((j-1)+numOfNodesX*2)+numOfNodesY)-2] if (useIOplane);
		}
}

import addon.ThermalModel;
import photonic.PhotoCAD.Pmesh_16;
import electronicComponents.Serializer;
import electronicComponents.Parallelizer;
import electronicComponents.ElectronicRouter;
import electronicComponents.ElectronicChannel;

network PhotonicMeshNetwork_PhotoCAD
{
        parameters:
        int numOfNodesX;
        int numOfNodesY;

		bool useIOplane;
		
		bool useWDM;
        int numOfWavelengthChannels;
		double O_frequency_data;
		double O_frequency_proc;
        @display("bgb=447,513");
    submodules:
        stat : Statistics {
            
        }
        
        thermal : ThermalModel {
            
        }
        
        vc : VirtualChannelControl {
        }
        
        prPlane: ProcessingPlane_ProcRoute 
        {
            parameters:
        		numNodesX = numOfNodesX;
           		numNodesY = numOfNodesY;
           	  	networkPrefix = "";
              numNetPorts = 2;
        }

        phPlane: Pmesh_16
        {
        }
        
        ioPlane[(useIOplane ? 1 : 0)]: IOPlane_Photonic {
            parameters:
                numOfPorts = numOfNodesX*2+(numOfNodesY-2)*2;   //io connections on periphery
          
        }

 		conv[numOfNodesX*numOfNodesY] : inoutConverter {
            
        }
        
         mModulator[16] : Parallelizer {
            parameters:
                //numOut = (useWDM?numOfWavelengthChannels:1);
              
                dispValueX = (useWDM?numOfWavelengthChannels:1)*80-10;
                dispValueY = (useWDM?numOfWavelengthChannels:1)*40 + 160;
                
                rateIn = O_frequency_proc;
                rateOut = O_frequency_data;
                
                numWiresOut = numOfWavelengthChannels;
                numWiresIn = numOfWavelengthChannels * O_frequency_data / O_frequency_proc;
                @display("p=$dispValueY,50;b=$dispValueX,10,,,,1");
            gates:
                out[64];
        }
        mDetector[16] : Serializer {
            parameters:
                dispValueX = (useWDM?numOfWavelengthChannels:1)*80-10;
                dispValueY = (useWDM?numOfWavelengthChannels:1)*40 + 160;
                
                
				rateIn = O_frequency_data;
                rateOut = O_frequency_proc;
                
                numWiresIn = numOfWavelengthChannels;
                numWiresOut = numOfWavelengthChannels * O_frequency_data / O_frequency_proc;
                
                @display("p=$dispValueY,250;b=$dispValueX,10,,,,1");
            gates:
                in[64];
        }
        
        elRoutes[16] : ElectronicRouter {
            parameters:
         	    type = 80;
         	  
            	numPorts = 5;
        		numPSEports = 15;
        		
            
                numX = numOfNodesX;
                numY = numOfNodesY;
                
                switchVariant = 3;
                
                
                id = string(index) + ".";
                
               
                level = "NET";
               
        }

    connections allowunconnected:
       //serDes

		for i = 0..15{
		 	mModulator[i].in <-- prPlane.dataOut[i*2 + 1];   
		    mDetector[i].out --> prPlane.dataIn[i*2 + 1];
		    
		    elRoutes[i].portIn[4] <-- prPlane.dataOut[i*2];
		    elRoutes[i].portOut[4] --> prPlane.dataIn[i*2];
		    
		}

		for i = 0..15, for j = 0..63 {
		 
		 	 mModulator[i].out[j] --> phPlane.cntrl_modData[j + ((i%4)*4 + int(i/4)) * 64];   
		 	 
		 	 mDetector[i].in[j] <-- phPlane.cntrl_detData[j + ((i%4)*4 + int(i/4)) * 64];
		 	
		}
		
		
		//electronic routers
         for i=0..numOfNodesX-1, for j=0..numOfNodesY-2 {
            
            elRoutes[i+j*numOfNodesX].portIn[2] <-- ElectronicChannel <-- elRoutes[i+(j+1)*numOfNodesX].portOut[0];
            elRoutes[i+j*numOfNodesX].portOut[2]  --> ElectronicChannel --> elRoutes[i+(j+1)*numOfNodesX].portIn[0];
           
        }

        for i=0..numOfNodesX-2, for j=0..numOfNodesY-1 {
          
            elRoutes[i+j*numOfNodesX].portIn[1] <-- ElectronicChannel <-- elRoutes[(i+1)+j*numOfNodesX].portOut[3];
            elRoutes[i+j*numOfNodesX].portOut[1]  --> ElectronicChannel --> elRoutes[(i+1)+j*numOfNodesX].portIn[3];
            
        }
  		
  		
  		//switch control
  		for i=0..15, for j=0..14 {
  		 	elRoutes[i].toPSE[j] --> phPlane.cntrl_ringCntrl[(((i%4)*4 + int(i/4))*15) + j];   
  		}
}