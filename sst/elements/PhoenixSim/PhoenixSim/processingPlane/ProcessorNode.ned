package processingPlane;

import processingPlane.Processor;
import processingPlane.ProcessorRouter;
import processingPlane.interfaces.NIF;



module ProcessorNode
{
    parameters:
        int numNetPorts;
        
        string id;
        
        bool autounblock;
        
        int number;
        
        string NIF_type;
        double clockRate;
     
        int processorConcentration;
		 int numOfWavelengthChannels;

    gates:
        input portIn[numNetPorts];
        output portOut[numNetPorts];
        
        inout laserIn[numOfWavelengthChannels];
        inout laserOut;

    submodules:

        proc[processorConcentration]: Processor {
             id = id + "0." + string(index) + ".";
            clockRate = clockRate;
          	isNetworkSideConcentration = false;
           
            @display("p=63,53");
        }
        

        nic: <NIF_type> like NIF {
           id = number;
          numNetPorts = numNetPorts;
            @display("p=158,114");
        }
        
        procRoute[(processorConcentration > 1) ? 1 : 0]:  ProcessorRouter {
            parameters:
                level = "PROC";
                id = id + "0.";
                
                numX = 1;
                numY = 1;
                
                clockRate = clockRate;
                
               autounblock = autounblock;
        }

    

    connections allowunconnected:

     	for i=0..processorConcentration-1 {
        	proc[i].nicReq <--> procRoute[0].req[i] if (processorConcentration > 1);
        	proc[i].toNic --> procRoute[0].portIn[i] if (processorConcentration > 1);
        	proc[i].fromNic <-- procRoute[0].portOut[i] if (processorConcentration > 1);
     	}
       
     	nic.procReq <--> procRoute[0].req[processorConcentration] if (processorConcentration > 1);
     	nic.toProc --> procRoute[0].portIn[processorConcentration] if (processorConcentration > 1);
     	nic.fromProc <-- procRoute[0].portOut[processorConcentration] if (processorConcentration > 1);
     	
     	proc[0].nicReq <--> nic.procReq  if (processorConcentration == 1);
     	proc[0].toNic --> nic.fromProc  if (processorConcentration == 1);
     	proc[0].fromNic <-- nic.toProc  if (processorConcentration == 1);
     	

		for i=0..numNetPorts-1 {
        	nic.portIn[i] <-- portIn[i];
        	nic.portOut[i] --> portOut[i];
       
		}


}

