
package topologies.electronicMesh_Circuit;

import processingPlane.ProcessingPlane;
import processingPlane.ProcessingPlane_ProcRoute;
import topologies.electronicMesh.ElectronicMeshPlane;

import statistics.Statistics;

import ioComponents.IOPlane_ElectronicCC;

import electronicComponents.VirtualChannelControl;

network ElectronicMeshCCNetwork
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
             		numNetPorts = 2;
                    
                    numOfWavelengthChannels = 0;

                @display("p=214,298");
         
        }
        netPlane: ElectronicMeshCCPlane {
            parameters:
				
				networkPrefix = "";
				
                @display("p=182,257");
            gates:
              
                cIOin[numOfNodesX*2+(numOfNodesY-2)*2];
                cIOout[numOfNodesX*2+(numOfNodesY-2)*2];
                 dIOin[numOfNodesX*2+(numOfNodesY-2)*2];
                dIOout[numOfNodesX*2+(numOfNodesY-2)*2];
            
        }
        
      
        
        
        ioPlane[useIOplane ? 1 : 0]: IOPlane_ElectronicCC {
            parameters:
             
                numOfPorts = numOfNodesX*2+(numOfNodesY-2)*2;   //io connections on periphery
         
               
            
        }





    connections allowunconnected:
        for i=0..numOfNodesX-1, for j=0..numOfNodesY-1, for k=0..processorConcentration-1 {
            prPlane.dataIn[2*(k+i*processorConcentration+j*numOfNodesX*processorConcentration)] <-- netPlane.CntrlelectronicGatewayOut[k+i*processorConcentration+j*numOfNodesX*processorConcentration];
            prPlane.dataOut[2*(k+i*processorConcentration+j*numOfNodesX*processorConcentration)] --> netPlane.CntrlelectronicGatewayIn[k+i*processorConcentration+j*numOfNodesX*processorConcentration];

  			prPlane.dataIn[2*(k+i*processorConcentration+j*numOfNodesX*processorConcentration)+1] <-- netPlane.DataelectronicGatewayOut[k+i*processorConcentration+j*numOfNodesX*processorConcentration];
            prPlane.dataOut[2*(k+i*processorConcentration+j*numOfNodesX*processorConcentration)+1] --> netPlane.DataelectronicGatewayIn[k+i*processorConcentration+j*numOfNodesX*processorConcentration];
            
            
           
        }


        //going off chip
        for i=0..numOfNodesX*2+(numOfNodesY-2)*2-1 {
            netPlane.cIOout[i] --> ioPlane[0].cIn[i] if useIOplane;
            netPlane.cIOin[i] <-- ioPlane[0].cOut[i] if useIOplane;
            
            netPlane.dIOout[i] --> ioPlane[0].dataIn[i] if useIOplane;
            netPlane.dIOin[i] <-- ioPlane[0].dataOut[i] if useIOplane;
            
        }

}

