
package topologies.electronicMesh;

import processingPlane.ProcessingPlane;
import processingPlane.ProcessingPlane_ProcRoute;
import topologies.electronicMesh.ElectronicMeshPlane;

import statistics.Statistics;

import ioComponents.IOPlane_Electronic;

import electronicComponents.VirtualChannelControl;

network ElectronicMeshNetwork
{
    parameters:
        int numOfNodesX;
        int numOfNodesY;
        
        int processorConcentration;
        
        bool useIOplane;

        @display("bgb=400,489");
    submodules:
		 stat : Statistics {
            
        }
        vc : VirtualChannelControl {
         	   
        }
        prPlane: ProcessingPlane_ProcRoute{
            parameters:
            		numNodesX = numOfNodesX;
            		numNodesY = numOfNodesY;
            		networkPrefix = "";         
             		numNetPorts = 1;
                    
                    numOfWavelengthChannels = 0;

                @display("p=214,298");
         
        }
        elPlane: ElectronicMeshPlane {
            parameters:
				
				networkPrefix = "";
				
                @display("p=182,257");
            gates:
               
                IOin[numOfNodesX*2+(numOfNodesY-2)*2];
                IOout[numOfNodesX*2+(numOfNodesY-2)*2];
            
        }
        
    
        
        
        ioPlane[useIOplane ? 1 : 0]: IOPlane_Electronic {
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
            elPlane.IOout[i] --> ioPlane[0].elIn[i] if useIOplane;
            elPlane.IOin[i] <-- ioPlane[0].elOut[i] if useIOplane;
            
        }

}




//this version of the network is for concentration by just adding in more ports to the router,
//  as opposed to an electronic switch behind the NIF
network ElectronicMeshNetwork_ConcBuffer
{
    parameters:
        int numOfNodesX;
        int numOfNodesY;
        
        int processorConcentration;
        
        bool useIOplane;

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
        elPlane: ElectronicMeshPlane_ConcBuffer {
            parameters:
				
				networkPrefix = "";
				
                @display("p=182,257");
            gates:
               
                IOin[numOfNodesX*2+(numOfNodesY-2)*2];
                IOout[numOfNodesX*2+(numOfNodesY-2)*2];
            
        }
        
    
        
        
        ioPlane[useIOplane ? 1 : 0]: IOPlane_Electronic {
            parameters:
             
                numOfPorts = numOfNodesX*2+(numOfNodesY-2)*2;   //io connections on periphery
            gates:
                elIn[numOfNodesX*2+(numOfNodesY-2)*2];
                elOut[numOfNodesX*2+(numOfNodesY-2)*2];
               
            
        }

    connections allowunconnected:
        for i=0..numOfNodesX-1, for j=0..numOfNodesY-1, for k=0..processorConcentration-1 {
            prPlane.dataIn[k+i*processorConcentration+j*numOfNodesX*processorConcentration] <-- elPlane.electronicGatewayOut[k+i*processorConcentration+j*numOfNodesX*processorConcentration];
            prPlane.dataOut[k+i*processorConcentration+j*numOfNodesX*processorConcentration] --> elPlane.electronicGatewayIn[k+i*processorConcentration+j*numOfNodesX*processorConcentration];

           
        }


        //going off chip
        for i=0..numOfNodesX*2+(numOfNodesY-2)*2-1 {
            elPlane.IOout[i] --> ioPlane[0].elIn[i] if useIOplane;
            elPlane.IOin[i] <-- ioPlane[0].elOut[i] if useIOplane;
            
        }

}


