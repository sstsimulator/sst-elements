
package topologies.CMesh;

import processingPlane.ProcessingPlane;
import topologies.CMesh.CMeshPlane;

import statistics.Statistics;

import ioComponents.IOPlane_Electronic;

import electronicComponents.VirtualChannelControl;

network CMeshNetwork
{
    parameters:
        int numOfNodesX;
        int numOfNodesY;
        
        int processorConcentration;
        
        

        @display("bgb=400,489");
    submodules:
		 stat : Statistics {
            
        }
        vc : VirtualChannelControl {
         	   
        }
        prPlane: ProcessingPlane{
            parameters:
            		numNodesX = numOfNodesX;
            		numNodesY = numOfNodesY;
            		networkPrefix = "";
             
                  
               		numNetPorts = 1;
                    
                    numOfWavelengthChannels = 0;

                @display("p=214,298");
         
        }
        elPlane: CMeshPlane {
            parameters:
				
				networkPrefix = "";
				
                @display("p=182,257");
            gates:
                electronicGatewayIn[numOfNodesX*numOfNodesY];
                electronicGatewayOut[numOfNodesX*numOfNodesY];
                IOin[numOfNodesX*2+(numOfNodesY-2)*2];
                IOout[numOfNodesX*2+(numOfNodesY-2)*2];
            
        }
        ioPlane: IOPlane_Electronic {
            parameters:
             
                numOfPorts = numOfNodesX*2+(numOfNodesY-2)*2;   //io connections on periphery
            gates:
                elIn[numOfNodesX*2+(numOfNodesY-2)*2];
                elOut[numOfNodesX*2+(numOfNodesY-2)*2];
               
            
        }





    connections allowunconnected:
        for i=0..numOfNodesX-1, for j=0..numOfNodesY-1 {
            prPlane.dataIn[i+j*numOfNodesX] <-- elPlane.electronicGatewayOut[i+j*numOfNodesX];
            prPlane.dataOut[i+j*numOfNodesX] --> elPlane.electronicGatewayIn[i+j*numOfNodesX];

           
        }


        //going off chip
        for i=0..numOfNodesX*2+(numOfNodesY-2)*2-1 {
            elPlane.IOout[i] --> ioPlane.elIn[i];
            elPlane.IOin[i] <-- ioPlane.elOut[i];
            
        }

}


