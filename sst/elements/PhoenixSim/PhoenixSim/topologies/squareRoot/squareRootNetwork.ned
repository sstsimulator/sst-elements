package topologies.squareRoot;

import topologies.squareRoot.SquareRootGrid;
import topologies.squareRoot.SquareRootGrid8x8;
import processingPlane.ProcessingPlane_ProcRoute;
import ioComponents.IOPlane_Photonic;

import electronicComponents.ElectronicChannel;
import electronicComponents.VirtualChannelControl;
import statistics.Statistics;

import electronicComponents.inoutConverter;
import chipComponents.LaserPlane;

network SquareRootNetwork4x4
{
    parameters:
        int numOfNodesX;
        int numOfNodesY;

		int numOfWavelengthChannels;
		
		bool useIOplane;
		
		 double coreSizeX;
       
         double pse1x2Width;
    	double pse2x2Width;
    	double crossWidth;

        @display("bgb=447,513;i=msg/package");
       
        
    submodules:
     	stat : Statistics 
     	{   
        }
        vc : VirtualChannelControl 
        {
        }
        
       // lsrPlane : LaserPlane {
        //    parameters:
       //         numOfNodesX = 4;
        //        numOfNodesY = 4;
                
        // 	gates:
        // 	    signal[16 * numOfWavelengthChannels];   
            
        //}
        
        prPlane[4]: ProcessingPlane_ProcRoute {
            parameters:
               	numNodesX = 2;
               	numNodesY = 2;
               	  networkPrefix = "0.0." + string(index) + ".";
              	numNetPorts = 2;
         
        }
        
        netPlane: SquareRootGrid {
            parameters:
				networkPrefix = "0.0.";
				numOfGrids = 1;
				gridID = -1;
				routerType = 30;
          
        
        }
       
        ioPlane[(useIOplane ? 1 : 0)]: IOPlane_Photonic {
            parameters:
               
                numOfPorts = numOfNodesX*2+(numOfNodesY-2)*2;   //io connections on periphery
            gates:
                elIn[numOfNodesX*2+(numOfNodesY-2)*2];
                elOut[numOfNodesX*2+(numOfNodesY-2)*2];
                phIn[numOfNodesX*2+(numOfNodesY-2)*2];
                phOut[numOfNodesX*2+(numOfNodesY-2)*2];
              

        }
conv[16] : inoutConverter {
            
        }

    connections allowunconnected:
        for i=0..3, for j=0..3 {
            prPlane[j].dataIn[i*2+1] <-- conv[i*4 + j].out;
            conv[i*4 + j].io_in <--> netPlane.PhGatewayOut[i + 4*j];
            prPlane[j].dataOut[i*2+1] --> conv[i*4 + j].in;
            conv[i*4 + j].io_out <--> netPlane.PhGatewayIn[i + 4*j];

            prPlane[j].dataIn[i*2] <-- netPlane.ElGatewayOut[i + 4*j];
            prPlane[j].dataOut[i*2] --> netPlane.ElGatewayIn[i + 4*j];

        	
        	
        	prPlane[j].laserOut[i] <--> netPlane.PhSource[i + 4*j];
        }
        
        
        
        
       // for i=0..16*numOfWavelengthChannels-1 {
       //  	lsrPlane.signal[i] <--> prPlane.laserIn[i];   
       // }
 
     
        netPlane.PhIOin[0] <--> ioPlane[0].phOut[0] if (useIOplane);
        netPlane.PhIOout[0] <--> ioPlane[0].phIn[0] if (useIOplane);
        netPlane.ElIOout[0] --> ioPlane[0].elIn[0] if (useIOplane);
        netPlane.ElIOin[0] <-- ioPlane[0].elOut[0] if (useIOplane);
      
        netPlane.PhIOin[1] <--> ioPlane[0].phOut[1] if (useIOplane);
        netPlane.PhIOout[1] <--> ioPlane[0].phIn[1] if (useIOplane);
        netPlane.ElIOout[1] --> ioPlane[0].elIn[1] if (useIOplane);
        netPlane.ElIOin[1] <-- ioPlane[0].elOut[1] if (useIOplane);
        
        netPlane.PhIOin[3] <--> ioPlane[0].phOut[4] if (useIOplane);
        netPlane.PhIOout[3] <--> ioPlane[0].phIn[4] if (useIOplane);
        netPlane.ElIOout[3] --> ioPlane[0].elIn[4] if (useIOplane);
        netPlane.ElIOin[3] <-- ioPlane[0].elOut[4] if (useIOplane);
        
        netPlane.PhIOin[4] <--> ioPlane[0].phOut[2] if (useIOplane);
        netPlane.PhIOout[4] <--> ioPlane[0].phIn[2] if (useIOplane);
        netPlane.ElIOout[4] --> ioPlane[0].elIn[2] if (useIOplane);
        netPlane.ElIOin[4] <-- ioPlane[0].elOut[2] if (useIOplane);
       
        netPlane.PhIOin[5] <--> ioPlane[0].phOut[3] if (useIOplane);
        netPlane.PhIOout[5] <--> ioPlane[0].phIn[3] if (useIOplane);
        netPlane.ElIOout[5] --> ioPlane[0].elIn[3] if (useIOplane);
        netPlane.ElIOin[5] <-- ioPlane[0].elOut[3] if (useIOplane);
        
        netPlane.PhIOin[6] <--> ioPlane[0].phOut[5] if (useIOplane);
        netPlane.PhIOout[6] <--> ioPlane[0].phIn[5] if (useIOplane);
        netPlane.ElIOout[6] --> ioPlane[0].elIn[5] if (useIOplane);
        netPlane.ElIOin[6] <-- ioPlane[0].elOut[5] if (useIOplane);
        
        netPlane.PhIOin[12] <--> ioPlane[0].phOut[6] if (useIOplane);
        netPlane.PhIOout[12] <--> ioPlane[0].phIn[6] if (useIOplane);
        netPlane.ElIOout[12] --> ioPlane[0].elIn[6] if (useIOplane);
        netPlane.ElIOin[12] <-- ioPlane[0].elOut[6] if (useIOplane);
        
        netPlane.PhIOin[9] <--> ioPlane[0].phOut[7] if (useIOplane);
        netPlane.PhIOout[9] <--> ioPlane[0].phIn[7] if (useIOplane);
        netPlane.ElIOout[9] --> ioPlane[0].elIn[7] if (useIOplane);
        netPlane.ElIOin[9] <-- ioPlane[0].elOut[7] if (useIOplane);
        
        netPlane.PhIOin[15] <--> ioPlane[0].phOut[8] if (useIOplane);
        netPlane.PhIOout[15] <--> ioPlane[0].phIn[8] if (useIOplane);
        netPlane.ElIOout[15] --> ioPlane[0].elIn[8] if (useIOplane);
        netPlane.ElIOin[15] <-- ioPlane[0].elOut[8] if (useIOplane);
        
        netPlane.PhIOin[14] <--> ioPlane[0].phOut[9] if (useIOplane);
        netPlane.PhIOout[14] <--> ioPlane[0].phIn[9] if (useIOplane);
        netPlane.ElIOout[14] --> ioPlane[0].elIn[9] if (useIOplane);
        netPlane.ElIOin[14] <-- ioPlane[0].elOut[9] if (useIOplane);
        
        netPlane.PhIOin[11] <--> ioPlane[0].phOut[10] if (useIOplane);
        netPlane.PhIOout[11] <--> ioPlane[0].phIn[10] if (useIOplane);
        netPlane.ElIOout[11] --> ioPlane[0].elIn[10] if (useIOplane);
        netPlane.ElIOin[11] <-- ioPlane[0].elOut[10] if (useIOplane);
        
        netPlane.PhIOin[10] <--> ioPlane[0].phOut[11] if (useIOplane);
        netPlane.PhIOout[10] <--> ioPlane[0].phIn[11] if (useIOplane);
        netPlane.ElIOout[10] --> ioPlane[0].elIn[11] if (useIOplane);
        netPlane.ElIOin[10] <-- ioPlane[0].elOut[11] if (useIOplane);

        

        


}

network SquareRootNetwork8x8
{
    parameters:
        int numOfNodesX;
        int numOfNodesY;

		int numOfWavelengthChannels;
		
		bool useIOplane;
		
		  double coreSizeX;
       
         double pse1x2Width;
    	double pse2x2Width;
    	double crossWidth;

		
        @display("bgb=447,513;i=msg/package");
       
    
        
    submodules:
         stat : Statistics 
     	{   
        }
        vc : VirtualChannelControl 
        {
        }
        
       prPlane[16]: ProcessingPlane_ProcRoute {
            parameters:
               	numNodesX = 2;
               	numNodesY = 2;
               	  networkPrefix = "0." + string(index >> 2) + "." + string(index % 4) + ".";
               	numNetPorts = 2;
         
        }
        
        netPlane: SquareRootGrid8x8 {
            parameters:
                networkPrefix = "0.";
			
				numOfEights = 1;
				gridID = -1;
				routerType = 31;
				
           		
        }
       
        ioPlane[(useIOplane ? 1 : 0)]: IOPlane_Photonic {
            parameters:
               
                numOfPorts = 28;   //io connections on periphery
          
        }
		conv[64] : inoutConverter {
            
        }

    connections allowunconnected:
        for i=0..15, for j=0..3 {
            prPlane[i].dataIn[j*2+1] <-- conv[i*4 + j].out;
            conv[i*4 + j].io_in <--> netPlane.PhGatewayOut[i*4 + j];
            prPlane[i].dataOut[j*2+1] --> conv[i*4 + j].in;
            conv[i*4 + j].io_out <--> netPlane.PhGatewayIn[i*4 + j];

            prPlane[i].dataIn[j*2] <-- netPlane.ElGatewayOut[i*4 + j];
            prPlane[i].dataOut[j*2] --> netPlane.ElGatewayIn[i*4 + j];

        
        	
        	prPlane[i].laserOut[j] <--> netPlane.PhSource[i*4 + j];
        }
        
     
        
    
    

  		//going off chip
       // for i=0..27 {
     
        //    netPlane.PhIOin[i] <--> ioPlane[0].phOut[i];
         //   netPlane.PhIOout[i] <--> ioPlane[0].phIn[i];

        //    netPlane.ElIOout[i] --> ioPlane[0].elIn[i];
        //    netPlane.ElIOin[i] <-- ioPlane[0].elOut[i];

         
        //}
        
         netPlane.PhIOin[0] <--> ioPlane[0].phOut[0] if (useIOplane);
        netPlane.PhIOout[0] <--> ioPlane[0].phIn[0] if (useIOplane);
        netPlane.ElIOout[0] --> ioPlane[0].elIn[0] if (useIOplane);
        netPlane.ElIOin[0] <-- ioPlane[0].elOut[0] if (useIOplane);
      
        netPlane.PhIOin[1] <--> ioPlane[0].phOut[1] if (useIOplane);
        netPlane.PhIOout[1] <--> ioPlane[0].phIn[1] if (useIOplane);
        netPlane.ElIOout[1] --> ioPlane[0].elIn[1] if (useIOplane);
        netPlane.ElIOin[1] <-- ioPlane[0].elOut[1] if (useIOplane);
        
        netPlane.PhIOin[4] <--> ioPlane[0].phOut[2] if (useIOplane);
        netPlane.PhIOout[4] <--> ioPlane[0].phIn[2] if (useIOplane);
        netPlane.ElIOout[4] --> ioPlane[0].elIn[2] if (useIOplane);
        netPlane.ElIOin[4] <-- ioPlane[0].elOut[2] if (useIOplane);
        
        netPlane.PhIOin[5] <--> ioPlane[0].phOut[3] if (useIOplane);
        netPlane.PhIOout[5] <--> ioPlane[0].phIn[3] if (useIOplane);
        netPlane.ElIOout[5] --> ioPlane[0].elIn[3] if (useIOplane);
        netPlane.ElIOin[5] <-- ioPlane[0].elOut[3] if (useIOplane);
       
        netPlane.PhIOin[16] <--> ioPlane[0].phOut[4] if (useIOplane);
        netPlane.PhIOout[16] <--> ioPlane[0].phIn[4] if (useIOplane);
        netPlane.ElIOout[16] --> ioPlane[0].elIn[4] if (useIOplane);
        netPlane.ElIOin[16] <-- ioPlane[0].elOut[4] if (useIOplane);
        
        netPlane.PhIOin[17] <--> ioPlane[0].phOut[5] if (useIOplane);
        netPlane.PhIOout[17] <--> ioPlane[0].phIn[5] if (useIOplane);
        netPlane.ElIOout[17] --> ioPlane[0].elIn[5] if (useIOplane);
        netPlane.ElIOin[17] <-- ioPlane[0].elOut[5] if (useIOplane);
        
        netPlane.PhIOin[20] <--> ioPlane[0].phOut[6] if (useIOplane);
        netPlane.PhIOout[20] <--> ioPlane[0].phIn[6] if (useIOplane);
        netPlane.ElIOout[20] --> ioPlane[0].elIn[6] if (useIOplane);
        netPlane.ElIOin[20] <-- ioPlane[0].elOut[6] if (useIOplane);
        
        netPlane.PhIOin[21] <--> ioPlane[0].phOut[7] if (useIOplane);
        netPlane.PhIOout[21] <--> ioPlane[0].phIn[7] if (useIOplane);
        netPlane.ElIOout[21] --> ioPlane[0].elIn[7] if (useIOplane);
        netPlane.ElIOin[21] <-- ioPlane[0].elOut[7] if (useIOplane);
        
        netPlane.PhIOin[3] <--> ioPlane[0].phOut[8] if (useIOplane);
        netPlane.PhIOout[3] <--> ioPlane[0].phIn[8] if (useIOplane);
        netPlane.ElIOout[3] --> ioPlane[0].elIn[8] if (useIOplane);
        netPlane.ElIOin[3] <-- ioPlane[0].elOut[8] if (useIOplane);
        
        netPlane.PhIOin[22] <--> ioPlane[0].phOut[9] if (useIOplane);
        netPlane.PhIOout[22] <--> ioPlane[0].phIn[9] if (useIOplane);
        netPlane.ElIOout[22] --> ioPlane[0].elIn[9] if (useIOplane);
        netPlane.ElIOin[22] <-- ioPlane[0].elOut[9] if (useIOplane);
        
        netPlane.PhIOin[12] <--> ioPlane[0].phOut[10] if (useIOplane);
        netPlane.PhIOout[12] <--> ioPlane[0].phIn[10] if (useIOplane);
        netPlane.ElIOout[12] --> ioPlane[0].elIn[10] if (useIOplane);
        netPlane.ElIOin[12] <-- ioPlane[0].elOut[10] if (useIOplane);
        
        netPlane.PhIOin[25] <--> ioPlane[0].phOut[11] if (useIOplane);
        netPlane.PhIOout[25] <--> ioPlane[0].phIn[11] if (useIOplane);
        netPlane.ElIOout[25] --> ioPlane[0].elIn[11] if (useIOplane);
        netPlane.ElIOin[25] <-- ioPlane[0].elOut[11] if (useIOplane);
        
         netPlane.PhIOin[15] <--> ioPlane[0].phOut[12] if (useIOplane);
        netPlane.PhIOout[15] <--> ioPlane[0].phIn[12] if (useIOplane);
        netPlane.ElIOout[15] --> ioPlane[0].elIn[12] if (useIOplane);
        netPlane.ElIOin[15] <-- ioPlane[0].elOut[12] if (useIOplane);
      
        netPlane.PhIOin[26] <--> ioPlane[0].phOut[13] if (useIOplane);
        netPlane.PhIOout[26] <--> ioPlane[0].phIn[13] if (useIOplane);
        netPlane.ElIOout[26] --> ioPlane[0].elIn[13] if (useIOplane);
        netPlane.ElIOin[26] <-- ioPlane[0].elOut[13] if (useIOplane);
        
        netPlane.PhIOin[48] <--> ioPlane[0].phOut[14] if (useIOplane);
        netPlane.PhIOout[48] <--> ioPlane[0].phIn[14] if (useIOplane);
        netPlane.ElIOout[48] --> ioPlane[0].elIn[14] if (useIOplane);
        netPlane.ElIOin[48] <-- ioPlane[0].elOut[14] if (useIOplane);
        
        netPlane.PhIOin[37] <--> ioPlane[0].phOut[15] if (useIOplane);
        netPlane.PhIOout[37] <--> ioPlane[0].phIn[15] if (useIOplane);
        netPlane.ElIOout[37] --> ioPlane[0].elIn[15] if (useIOplane);
        netPlane.ElIOin[37] <-- ioPlane[0].elOut[15] if (useIOplane);
       
        netPlane.PhIOin[51] <--> ioPlane[0].phOut[16] if (useIOplane);
        netPlane.PhIOout[51] <--> ioPlane[0].phIn[16] if (useIOplane);
        netPlane.ElIOout[51] --> ioPlane[0].elIn[16] if (useIOplane);
        netPlane.ElIOin[51] <-- ioPlane[0].elOut[16] if (useIOplane);
        
        netPlane.PhIOin[38] <--> ioPlane[0].phOut[17] if (useIOplane);
        netPlane.PhIOout[38] <--> ioPlane[0].phIn[17] if (useIOplane);
        netPlane.ElIOout[38] --> ioPlane[0].elIn[17] if (useIOplane);
        netPlane.ElIOin[38] <-- ioPlane[0].elOut[17] if (useIOplane);
        
        netPlane.PhIOin[60] <--> ioPlane[0].phOut[18] if (useIOplane);
        netPlane.PhIOout[60] <--> ioPlane[0].phIn[18] if (useIOplane);
        netPlane.ElIOout[60] --> ioPlane[0].elIn[18] if (useIOplane);
        netPlane.ElIOin[60] <-- ioPlane[0].elOut[18] if (useIOplane);
        
        netPlane.PhIOin[41] <--> ioPlane[0].phOut[19] if (useIOplane);
        netPlane.PhIOout[41] <--> ioPlane[0].phIn[19] if (useIOplane);
        netPlane.ElIOout[41] --> ioPlane[0].elIn[19] if (useIOplane);
        netPlane.ElIOin[41] <-- ioPlane[0].elOut[19] if (useIOplane);
        
        netPlane.PhIOin[63] <--> ioPlane[0].phOut[20] if (useIOplane);
        netPlane.PhIOout[63] <--> ioPlane[0].phIn[20] if (useIOplane);
        netPlane.ElIOout[63] --> ioPlane[0].elIn[20] if (useIOplane);
        netPlane.ElIOin[63] <-- ioPlane[0].elOut[20] if (useIOplane);
        
        netPlane.PhIOin[62] <--> ioPlane[0].phOut[21] if (useIOplane);
        netPlane.PhIOout[62] <--> ioPlane[0].phIn[21] if (useIOplane);
        netPlane.ElIOout[62] --> ioPlane[0].elIn[21] if (useIOplane);
        netPlane.ElIOin[62] <-- ioPlane[0].elOut[21] if (useIOplane);
        
        netPlane.PhIOin[59] <--> ioPlane[0].phOut[22] if (useIOplane);
        netPlane.PhIOout[59] <--> ioPlane[0].phIn[22] if (useIOplane);
        netPlane.ElIOout[59] --> ioPlane[0].elIn[22] if (useIOplane);
        netPlane.ElIOin[59] <-- ioPlane[0].elOut[22] if (useIOplane);
        
        netPlane.PhIOin[58] <--> ioPlane[0].phOut[23] if (useIOplane);
        netPlane.PhIOout[58] <--> ioPlane[0].phIn[23] if (useIOplane);
        netPlane.ElIOout[58] --> ioPlane[0].elIn[23] if (useIOplane);
        netPlane.ElIOin[58] <-- ioPlane[0].elOut[23] if (useIOplane);
        
        netPlane.PhIOin[47] <--> ioPlane[0].phOut[24] if (useIOplane);
        netPlane.PhIOout[47] <--> ioPlane[0].phIn[24] if (useIOplane);
        netPlane.ElIOout[47] --> ioPlane[0].elIn[24] if (useIOplane);
        netPlane.ElIOin[47] <-- ioPlane[0].elOut[24] if (useIOplane);
        
        netPlane.PhIOin[46] <--> ioPlane[0].phOut[25] if (useIOplane);
        netPlane.PhIOout[46] <--> ioPlane[0].phIn[25] if (useIOplane);
        netPlane.ElIOout[46] --> ioPlane[0].elIn[25] if (useIOplane);
        netPlane.ElIOin[46] <-- ioPlane[0].elOut[25] if (useIOplane);
        
        netPlane.PhIOin[43] <--> ioPlane[0].phOut[26] if (useIOplane);
        netPlane.PhIOout[43] <--> ioPlane[0].phIn[26] if (useIOplane);
        netPlane.ElIOout[43] --> ioPlane[0].elIn[26] if (useIOplane);
        netPlane.ElIOin[43] <-- ioPlane[0].elOut[26] if (useIOplane);
        
        netPlane.PhIOin[42] <--> ioPlane[0].phOut[27] if (useIOplane);
        netPlane.PhIOout[42] <--> ioPlane[0].phIn[27] if (useIOplane);
        netPlane.ElIOout[42] --> ioPlane[0].elIn[27] if (useIOplane);
        netPlane.ElIOin[42] <-- ioPlane[0].elOut[27] if (useIOplane);
        
     

        


}


network SquareRootNetwork16x16
{
    parameters:
        int numOfNodesX;
        int numOfNodesY;

		int numOfWavelengthChannels;
		
		bool useIOplane;
		
		  double coreSizeX;
       
         double pse1x2Width;
    	double pse2x2Width;
    	double crossWidth;

		
        @display("bgb=447,513;i=msg/package");
       
      
    
        
    submodules:
         stat : Statistics 
     	{   
        }
        vc : VirtualChannelControl 
        {
        }
        
       prPlane[64]: ProcessingPlane_ProcRoute {
            parameters:
               	numNodesX = 2;
               	numNodesY = 2;
               	  networkPrefix = string((index >> 4) % 4) + "." + string((index >> 2) % 4) + "." + string(index % 4) + ".";
               numNetPorts = 2;
         
        }
        
        netPlane: SquareRootGrid16x16 {
            parameters:
                networkPrefix = "";
				
				numOfSixteens = 1;
				gridID = -1;
				routerType = 32;
				
           		
        }
       
        ioPlane[(useIOplane ? 1 : 0)]: IOPlane_Photonic {
            parameters:
               
                numOfPorts = 28;   //io connections on periphery
          
        }
conv[256] : inoutConverter {
            
        }

    connections allowunconnected:
        for i=0..63, for j=0..3 {
            prPlane[i].dataIn[j*2+1] <-- conv[i*4 + j].out;
            conv[i*4 + j].io_in <--> netPlane.PhGatewayOut[i*4 + j];
            prPlane[i].dataOut[j*2+1] --> conv[i*4 + j].in;
            conv[i*4 + j].io_out <--> netPlane.PhGatewayIn[i*4 + j];

            prPlane[i].dataIn[j*2] <-- netPlane.ElGatewayOut[i*4 + j];
            prPlane[i].dataOut[j*2] --> netPlane.ElGatewayIn[i*4 + j];

        
        	
        	prPlane[i].laserOut[j] <--> netPlane.PhSource[i*4 + j];
        }

        
        	
        
        
}


