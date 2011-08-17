
package processingPlane;

import processingPlane.ProcessorNode;
import processingPlane.interfaces.NIF;

module ProcessingPlane
{
    parameters:
        int numNetPorts;
        
        int numNodesX;
        int numNodesY;
     
        
        string networkPrefix;
      	int processorConcentration;
      	
      	string NIF_type;
        
 		double O_frequency_cntrl;
        
        int numOfWavelengthChannels;
        
        @display("i=planes/pe_plane");
    gates:
        input dataIn[numNodesX*numNodesY*processorConcentration*numNetPorts];
        output dataOut[numNodesX*numNodesY*processorConcentration*numNetPorts];
       
        
        inout laserIn[numNodesX*numNodesY * numOfWavelengthChannels*processorConcentration];
        inout laserOut[numNodesX*numNodesY*processorConcentration];

     
    submodules:
        proc[numNodesX*numNodesY*processorConcentration]: Processor{
            parameters:
                id = networkPrefix + string(int(index / processorConcentration)) + ".0." + string(index % processorConcentration) + ".";
             	
          		clockRate = O_frequency_cntrl;
                @display("p=100,100,m,$numNodesX,100,100;i=components/cellnode");
               
        }
        
        
        nic[numNodesX*numNodesY*processorConcentration]: <NIF_type> like NIF {
           id = index;
          numNetPorts = numNetPorts;
                @display("p=80,80,m,$numNodesX,100,100");
        }
        
        
    connections allowunconnected:
        for j=0..numNetPorts-1, for i=0..numNodesX*numNodesY*processorConcentration -1 {
            dataIn[i*numNetPorts + j] --> { @display("ls=,0"); } --> nic[i].portIn[j];
            dataOut[i*numNetPorts + j] <-- { @display("ls=,0"); } <-- nic[i].portOut[j];
          

        }
        for i=0..numNodesX*numNodesY*processorConcentration -1 {
       		proc[i].nicReq <--> nic[i].procReq;
     		proc[i].toNic --> nic[i].fromProc;
     		proc[i].fromNic <-- nic[i].toProc;
           
        }
        
     
}



module ProcessingPlane_ProcRoute
{
    parameters:
        int numNetPorts;
        
        int numNodesX;
        int numNodesY;
     
      	bool autounblock = default(false);
        string networkPrefix;
        
        int processorConcentration;
        
 		double O_frequency_cntrl;
        
        int numOfWavelengthChannels;
        
        @display("i=planes/pe_plane");
    gates:
        input dataIn[numNodesX*numNodesY*numNetPorts];
        output dataOut[numNodesX*numNodesY*numNetPorts];
     
        
        inout laserIn[numNodesX*numNodesY * numOfWavelengthChannels];
        inout laserOut[numNodesX*numNodesY];

     
    submodules:
        tx[numNodesX*numNodesY]: ProcessorNode {
            parameters:
                id = networkPrefix + string(index) + ".";
               numNetPorts = numNetPorts;
          		clockRate = O_frequency_cntrl;
          		autounblock = autounblock;
          		number = index;
            

                @display("p=100,100,m,$numNodesX,100,100;i=components/cellnode");
               

        }
    connections allowunconnected:
        for j=0..numNetPorts-1, for i=0..numNodesX*numNodesY -1 {
            dataIn[i*numNetPorts + j] --> { @display("ls=,0"); } --> tx[i].portIn[j];
           dataOut[i*numNetPorts + j] <-- { @display("ls=,0"); } <-- tx[i].portOut[j];
          

        }
        for i=0..numNodesX*numNodesY -1 {
            laserOut[i] <--> tx[i].laserOut;
           
        }
        
       for i=0..numNodesX*numNodesY -1, for j=0..numOfWavelengthChannels-1 {
			laserIn[i*numOfWavelengthChannels + j] <--> tx[i].laserIn[j];           
       }
}