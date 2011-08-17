
package topologies.electronicMesh_Circuit;

//import topologies.electronicMesh.ElectronicMeshNode;
import electronicComponents.ElectronicRouter;
import electronicComponents.ElectronicChannel;
import electronicComponents.RouterCrossbar;

import electronicComponents.inoutConverter;
import electronicComponents.mux;

import electronicComponents.Parallelizer;
import electronicComponents.Serializer;


module ElectronicMeshCCPlane
{
    parameters:
        int numOfNodesX;
        int numOfNodesY;
        
        double O_frequency_cntrl;
        double O_frequency_data;
        
        double clockRate_io;
        double O_frequency_proc;
        
        int ioChannelWidth;
        
        int processorConcentration;
        int numOfWavelengthChannels;
        
        string networkPrefix;
        @display("i=OptNet_planes/el_plane_t");
    gates:
        input CntrlelectronicGatewayIn[numOfNodesX*numOfNodesY*processorConcentration];
        output CntrlelectronicGatewayOut[numOfNodesX*numOfNodesY*processorConcentration];
         input DataelectronicGatewayIn[numOfNodesX*numOfNodesY*processorConcentration];
        output DataelectronicGatewayOut[numOfNodesX*numOfNodesY*processorConcentration];
        
        input cIOin[];
        output cIOout[];
         input dIOin[];
        output dIOout[];

    types:
        channel ElectronicDataChannel extends electronicComponents.ElectronicChannel  //connections to the middle
        {
            electronicChannelWidth=numOfWavelengthChannels;
            isDataChannel = true;
            lowswing = true;
        }

    submodules:
        node[numOfNodesX*numOfNodesY]: ElectronicRouter {
            parameters:
            	numPorts = 4+processorConcentration;
        		numPSEports = 4+processorConcentration;
        		type = 90;
                id = networkPrefix + string(index) + ".";
                level = "NET";
                          
                numX = numOfNodesX;
                numY = numOfNodesY;
                
                switchVariant = 1;
                
                
                
            
        }
        
        xbar[numOfNodesX*numOfNodesY] : RouterCrossbar {
            parameters:
                numPorts = 4+processorConcentration;
                autounblock = false;
                clockRate = O_frequency_data;
                electronicChannelWidth=numOfWavelengthChannels;
                isDataPlane = true;
        }
        
        conv[numOfNodesX*numOfNodesY] : inoutConverter {
            
        }
        
        m[numOfNodesX*numOfNodesY] : mux {
            gates:
                in[4+processorConcentration];
        }

		serializer[numOfNodesX*numOfNodesY*processorConcentration] : Parallelizer {
   
                rateIn = O_frequency_proc;
                rateOut = O_frequency_data;
                
                numWiresOut = numOfWavelengthChannels;
                numWiresIn = numOfWavelengthChannels * O_frequency_data / O_frequency_proc;
                gates:
                    out[1];
		}
		
		deserializer[numOfNodesX*numOfNodesY*processorConcentration] : Serializer {
                
                rateIn = O_frequency_data;
                rateOut = O_frequency_proc;
                
                numWiresIn = numOfWavelengthChannels;
                numWiresOut = numOfWavelengthChannels * O_frequency_data / O_frequency_proc;
                
                gates:
                    in[1];
		}
		
		
		IOserializer[sizeof(this.cIOout)] : Parallelizer {
                rateIn = O_frequency_data;
                rateOut = clockRate_io;
                
                numWiresOut = ioChannelWidth;
                numWiresIn = ioChannelWidth * clockRate_io / O_frequency_data;
                
                 gates:
                    out[1];
		}
		
		IOdeserializer[sizeof(this.cIOin)] : Serializer {
                
                rateIn = O_frequency_data;
                rateOut = O_frequency_cntrl;
                
                numWiresIn = ioChannelWidth;
                numWiresOut = ioChannelWidth * clockRate_io / O_frequency_data;
                
                   gates:
                    in[1];
		}
	

    connections allowunconnected:
        for i=0..numOfNodesX-1, for j=0..numOfNodesY-2 {
            node[i+j*numOfNodesX].portIn[2] <-- ElectronicChannel <-- node[i+(j+1)*numOfNodesX].portOut[0];
            node[i+j*numOfNodesX].portOut[2] --> ElectronicChannel --> node[i+(j+1)*numOfNodesX].portIn[0];
            
            xbar[i+j*numOfNodesX].in[2] <-- ElectronicDataChannel <-- xbar[i+(j+1)*numOfNodesX].out[0];
            xbar[i+j*numOfNodesX].out[2] --> ElectronicDataChannel --> xbar[i+(j+1)*numOfNodesX].in[0];
           
        }

        for i=0..numOfNodesX-2, for j=0..numOfNodesY-1 {
            node[i+j*numOfNodesX].portIn[1] <-- ElectronicChannel <-- node[(i+1)+j*numOfNodesX].portOut[3];
            node[i+j*numOfNodesX].portOut[1] --> ElectronicChannel --> node[(i+1)+j*numOfNodesX].portIn[3];
            
            xbar[i+j*numOfNodesX].in[1] <-- ElectronicDataChannel <-- xbar[(i+1)+j*numOfNodesX].out[3];
            xbar[i+j*numOfNodesX].out[1] --> ElectronicDataChannel --> xbar[(i+1)+j*numOfNodesX].in[3];
            
        }

        for i=0..numOfNodesX-1, for j=0..numOfNodesY-1, for k=0..processorConcentration-1 {
            node[i+j*numOfNodesX].portIn[4+k]  <-- ElectronicChannel {power = false;} <-- CntrlelectronicGatewayIn[k+i*processorConcentration+j*numOfNodesX*processorConcentration];
            node[i+j*numOfNodesX].portOut[4+k]  --> ElectronicChannel {power = false;} --> CntrlelectronicGatewayOut[k+i*processorConcentration+j*numOfNodesX*processorConcentration];
            
            xbar[i+j*numOfNodesX].in[4+k]  <-- ElectronicDataChannel{isThroughChannel=true; power=false;} <-- serializer[k+i*processorConcentration+j*numOfNodesX*processorConcentration].out[0];
            serializer[k+i*processorConcentration+j*numOfNodesX*processorConcentration].in <-- DataelectronicGatewayIn[k+i*processorConcentration+j*numOfNodesX*processorConcentration];
           
            xbar[i+j*numOfNodesX].out[4+k]  --> ElectronicDataChannel{isThroughChannel=true; power=false;} --> deserializer[k+i*processorConcentration+j*numOfNodesX*processorConcentration].in[0];
            deserializer[k+i*processorConcentration+j*numOfNodesX*processorConcentration].out --> DataelectronicGatewayOut[k+i*processorConcentration+j*numOfNodesX*processorConcentration];
        }
        
        for i=0..numOfNodesX-1, for j=0..numOfNodesY-1{
        
            m[i+j*numOfNodesX].out --> conv[i+j*numOfNodesX].in;
            conv[i+j*numOfNodesX].io_out <--> xbar[i+j*numOfNodesX].cntrl;
        }
        
        for i=0..numOfNodesX-1, for j=0..numOfNodesY-1, for k=0..4+processorConcentration-1 {
            node[i+j*numOfNodesX].toPSE[k] --> m[i+j*numOfNodesX].in[k];
            
          

        }

        //going off chip
        for i=0..numOfNodesX-1 {
            cIOin[i] --> node[i].portIn[0];
            cIOout[i] <-- node[i].portOut[0];
            
            dIOin[i]  --> IOdeserializer[i].in[0];
            IOdeserializer[i].out --> xbar[i].in[0];
            dIOout[i]  <-- IOserializer[i].out[0];
            IOserializer[i].in <-- xbar[i].out[0];
          
            

            cIOin[i+numOfNodesX] --> node[i+(numOfNodesY-1)*numOfNodesX].portIn[2];
            cIOout[i+numOfNodesX] <-- node[i+(numOfNodesY-1)*numOfNodesX].portOut[2];
            
            dIOin[i+numOfNodesX]  --> IOdeserializer[i+numOfNodesX].in[0];
            IOdeserializer[i+numOfNodesX].out --> xbar[i+(numOfNodesY-1)*numOfNodesX].in[2];
            dIOout[i+numOfNodesX]  <-- IOserializer[i+numOfNodesX].out[0];
            IOserializer[i+numOfNodesX].in <-- xbar[i+(numOfNodesY-1)*numOfNodesX].out[2];
            
        }

        for j=1..numOfNodesY-2 {
            cIOin[(j-1)+numOfNodesX*2] --> node[j*numOfNodesX].portIn[3];
            cIOout[(j-1)+numOfNodesX*2] <-- node[j*numOfNodesX].portOut[3];
            
            dIOin[(j-1)+numOfNodesX*2]  --> IOdeserializer[(j-1)+numOfNodesX*2].in[0];
            IOdeserializer[(j-1)+numOfNodesX*2].out -->  xbar[j*numOfNodesX].in[3];
            dIOout[(j-1)+numOfNodesX*2]  <-- IOserializer[(j-1)+numOfNodesX*2].out[0];
            IOserializer[(j-1)+numOfNodesX*2].in <--  xbar[j*numOfNodesX].out[3];
           

            cIOin[(j-1)+numOfNodesX*2+numOfNodesY-2] --> node[j*numOfNodesX + numOfNodesX-1].portIn[1];
            cIOout[(j-1)+numOfNodesX*2+numOfNodesY-2] <-- node[j*numOfNodesX + numOfNodesX-1].portOut[1];
            
            dIOin[(j-1)+numOfNodesX*2+numOfNodesY-2]  --> IOdeserializer[(j-1)+numOfNodesX*2+numOfNodesY-2].in[0];
            IOdeserializer[(j-1)+numOfNodesX*2+numOfNodesY-2].out -->  xbar[j*numOfNodesX + numOfNodesX-1].in[1];
            dIOout[(j-1)+numOfNodesX*2+numOfNodesY-2]   <-- IOserializer[(j-1)+numOfNodesX*2+numOfNodesY-2].out[0];
            IOserializer[(j-1)+numOfNodesX*2+numOfNodesY-2].in <-- xbar[j*numOfNodesX + numOfNodesX-1].out[1];
        }
}



