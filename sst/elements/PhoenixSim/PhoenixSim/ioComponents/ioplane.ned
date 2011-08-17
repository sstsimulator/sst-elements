package ioComponents;

import electronicComponents.ElectronicIOpad;
import ioComponents.MemNode_Electronic;

import photonic.devices.Facet;

import photonic.devices.Line;

import ioComponents.DRAM_LRL.DRAM_OCM;
import ioComponents.DRAM_LRL.DRAM_OCM_E;



module IOPlane_Electronic
{
    parameters:
        int numOfPorts;

    gates:
        input elIn[numOfPorts];
        output elOut[numOfPorts];
        


    submodules:

        dram[numOfPorts]: MemNode_Electronic {
            parameters:
                dramID = index;
              

                @display("p=267,157");
        }
        
       
    connections allowunconnected:
        for i=0..numOfPorts-1 {
          

         
            dram[i].elIn <-- ElectronicIOpad <-- elIn[i]; 
            dram[i].elOut --> ElectronicIOpad --> elOut[i];

	
			

        }



}


module IOPlane_Photonic
{
    parameters:
        int numOfPorts;
  		int DRAM_DIMMsPerGW;
	
		double IL_Facet_Lateral;

    gates:
        input elIn[numOfPorts];
        output elOut[numOfPorts];
        inout phIn[numOfPorts];
        inout phOut[numOfPorts];
        
    


    submodules:


        fIn[DRAM_DIMMsPerGW * numOfPorts]: Facet {
			parameters:
			    IL_Facet = IL_Facet_Lateral;
            @display("p=60,56");
        }
        fOut[DRAM_DIMMsPerGW * numOfPorts]: Facet {
			parameters:
			    IL_Facet = IL_Facet_Lateral;
            @display("p=528,56");
        }

        
       	memGW[numOfPorts] : MemoryGateway_Photonic {
       	    id = index;
       	}
       	
       	dimms[DRAM_DIMMsPerGW * numOfPorts] : DRAM_OCM {
       	    
       	}
         
      	
    connections allowunconnected:
        for i=0..numOfPorts-1 {
            memGW[i].elIn <-- elIn[i];
            memGW[i].elOut --> elOut[i];
            memGW[i].ph_in_net <--> phIn[i];
            memGW[i].ph_out_net <--> phOut[i];
        }
        
        for i=0..numOfPorts-1, for j=0..DRAM_DIMMsPerGW-1 {
         	memGW[i].ph_in_dimm[j] <--> fIn[i*DRAM_DIMMsPerGW + j].photonicOut;
         	memGW[i].ph_out_dimm[j] <--> fOut[i*DRAM_DIMMsPerGW + j].photonicIn;
         	
         	fIn[i*DRAM_DIMMsPerGW + j].photonicIn <--> dimms[i*DRAM_DIMMsPerGW + j].dataOut;
         	fOut[i*DRAM_DIMMsPerGW + j].photonicOut <--> dimms[i*DRAM_DIMMsPerGW+j].dataIn;   
            
        }


}


module IOPlane_ElectronicCC
{
    parameters:
        int numOfPorts;
  		int DRAM_DIMMsPerGW;
	
		double IL_Facet_Lateral;
		
	

    gates:
        input cIn[numOfPorts];
        output cOut[numOfPorts];
        input dataIn[numOfPorts];
        output dataOut[numOfPorts];
        
    


    submodules:


       
        
       	memGW[numOfPorts] : MemoryGateway_ElectronicCC  {
       	    id = index;
       	}
       	
       	dimms[DRAM_DIMMsPerGW * numOfPorts] : DRAM_OCM_E {
       	    
       	}
         
      
         
      	
    connections allowunconnected:
        for i=0..numOfPorts-1 {
            memGW[i].cIn <-- cIn[i];
            memGW[i].cOut --> cOut[i];
            memGW[i].data_in_net <-- dataIn[i];
            memGW[i].data_out_net --> dataOut[i];
        }
        
        for i=0..numOfPorts-1, for j=0..DRAM_DIMMsPerGW-1 {
         	memGW[i].data_in_dimm[j] <-- ElectronicIOpad{lowSwing = true;} <-- dimms[i*DRAM_DIMMsPerGW + j].dataOut;
         	memGW[i].data_out_dimm[j] --> ElectronicIOpad{lowSwing = true;} --> dimms[i*DRAM_DIMMsPerGW+j].dataIn;   
         	
        
        }


}