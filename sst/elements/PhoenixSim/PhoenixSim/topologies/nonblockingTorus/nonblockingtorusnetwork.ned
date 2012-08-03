package topologies.nonblockingTorus;

import topologies.nonblockingTorus.NonblockingTorusPhotonicPlane;
import processingPlane.ProcessingPlane_ProcRoute;
import ioComponents.IOPlane_Photonic;
import electronicComponents.VirtualChannelControl;

import electronicComponents.inoutConverter;
import statistics.Statistics;

network NonblockingTorusNetwork
{
    parameters:
        int numOfNodesX;
        //double numOfMemBanks;
        
        bool useIOplane;
        
    submodules:
		stat : Statistics 
		{    
        }
        
        vc : VirtualChannelControl 
        {
        }
        
        phPlane: NonblockingTorusPhotonicPlane {
            parameters:
                @display("p=100,175;i=planes/ph_plane.gif");
           
                
            
        }
        
        prPlane: ProcessingPlane_ProcRoute 
        {
            parameters:
          		networkPrefix = "";
                numNodesX = numOfNodesX;
                numNodesY = 2;
                numNetPorts = 2;

           
        }
        ioPlane[(useIOplane ? 1 : 0)]: IOPlane_Photonic 
        {
            parameters:
                
                numOfPorts = numOfNodesX*2;
           
        }
        conv[numOfNodesX*2] : inoutConverter {
            
        }
    connections allowunconnected:
        for i=0..2*numOfNodesX-1 {
            prPlane.dataOut[i*2+1] --> conv[i].in;
            conv[i].io_out <--> { @display("ls=,0"); } <--> phPlane.electronicGatewayIn[i];
            prPlane.dataIn[i*2+1] <-- conv[i].out;
            conv[i].io_in <--> { @display("ls=,0"); } <--> phPlane.electronicGatewayOut[i];

            prPlane.dataOut[i*2] --> { @display("ls=,0"); } --> phPlane.electronicControlInProc[i];
            prPlane.dataIn[i*2] <-- { @display("ls=,0"); } <-- phPlane.electronicControlOutProc[i];

          

			ioPlane[0].phOut[i] <--> { @display("ls=,0"); } <--> phPlane.photonicMemoryIn[i] if (useIOplane);
			ioPlane[0].phIn[i] <--> { @display("ls=,0"); } <--> phPlane.photonicMemoryOut[i] if (useIOplane);

			ioPlane[0].elOut[i] --> { @display("ls=,0"); } --> phPlane.electronicControlInMemory[i] if (useIOplane);
			ioPlane[0].elIn[i] <-- { @display("ls=,0"); } <-- phPlane.electronicControlOutMemory[i] if (useIOplane);
			
        }
        
}
