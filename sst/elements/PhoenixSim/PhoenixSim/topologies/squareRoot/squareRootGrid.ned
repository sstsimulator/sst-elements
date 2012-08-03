package topologies.squareRoot;

import chipComponents.HybridRouter;

import electronicComponents.ElectronicChannel;


import photonic.devices.LineBidirectional;
import photonic.devices.BendBidirectional;
import photonic.devices.CrossBidirectional;


import photonic.devices.CrossDouble;
import photonic.devices.Line;

import topologies.Gateway_NoMem;



module SquareRootQuadrant
{
    parameters:
        string networkPrefix;
        
        int quadId;
        
    	int routerType;
    	int numOfGrids;
    	
    	double coreSizeX;
    	double pse1x2Width;
    	double pse2x2Width;
    	double crossWidth;
    	
    	double layoutDim1;
    	double layoutDim2;
    	double layoutDim3;
    	
    	bool useIOplane;
    	
    	
    
     gates:
        input ElGatewayIn[4];
        input ElIOin[4];
        output ElGatewayOut[4];
        output ElIOout[4];
        
        inout PhGatewayIn[4];
        inout PhIOin[4];
        inout PhGatewayOut[4];
        inout PhIOout[4];
        
        inout PhSource[4];

        input levelUpIn[4];
        output levelUpOut[4];
        inout phlevelUpIn[4];
        inout phlevelUpOut[4];
    
    
     submodules:
        node[4]: HybridRouter {
            parameters:
                id = networkPrefix + string(quadId) + "." + string(index) + ".";
			
				optSwitch = "NonBlockingSwitch4x4_P";
				
				
				elRouter = 33;
				
				level = "NET1";
				
				numX = sqrt(numOfGrids*16);
				numY = sqrt(numOfGrids*16);
				
				

        }
        
        gw[useIOplane ? 4 : 0]: HybridRouter {
            parameters:
                id = networkPrefix + string(quadId) + "." + string(index) + ".";
			
				optSwitch = "Gateway";
				
				
				elRouter = 1;
				
				level = "NET1";
				
				numX = sqrt(numOfGrids*16);
				numY = sqrt(numOfGrids*16);
				
				

        }
        
        gw_nomem[useIOplane ? 0 : 4]: Gateway_NoMem {
          
        }
        
        lines0[4] : LineBidirectional {
             parameters:
                 Length_Line = coreSizeX - layoutDim1;
        }
        
        
     connections allowunconnected:
         
         for i=0..3 {
  
           	gw[i].phIn[0] <--> PhGatewayIn[i] if (useIOplane);
           	gw[i].phOut[0] <--> PhGatewayOut[i] if (useIOplane);
           	
           	gw_nomem[i].in[0] <--> PhGatewayIn[i] if (!useIOplane);
           	gw_nomem[i].out[0] <--> PhGatewayOut[i] if (!useIOplane);
           	
           	//gw[i].source <--> PhSource[i];
           	
           	
           	gw[i].elIn[0] <-- ElectronicChannel{power = false;} <-- ElGatewayIn[i] if (useIOplane);
            gw[i].elOut[0] --> ElectronicChannel{power = false;} --> ElGatewayOut[i] if (useIOplane);
            
            
            
            gw[i].elIn[2] <-- ElIOin[i] if useIOplane;
            gw[i].elOut[2] --> ElIOout[i] if useIOplane;
            
            gw[i].phIn[2] <--> PhIOin[i] if useIOplane;
            gw[i].phOut[2] <--> PhIOout[i] if useIOplane;
            
        }
        
    
    
    	node[0].elIn[1] <-- ElectronicChannel <-- node[1].elOut[3];
        node[0].elOut[1] --> ElectronicChannel --> node[1].elIn[3];
      
        node[0].elIn[2] <-- ElectronicChannel <-- node[3].elOut[0];
        node[0].elOut[2] --> ElectronicChannel --> node[3].elIn[0];
        
        node[1].elIn[2] <-- ElectronicChannel <-- node[2].elOut[0];
        node[1].elOut[2] --> ElectronicChannel --> node[2].elIn[0];
      
        node[2].elIn[3] <-- ElectronicChannel <-- node[3].elOut[1];
        node[2].elOut[3] --> ElectronicChannel --> node[3].elIn[1];
        
        node[0].phIn[1] <--> lines0[0].photonicInA;
        lines0[0].photonicOutA <--> node[1].phOut[3];
        node[0].phOut[1] <--> lines0[0].photonicInB;
        lines0[0].photonicOutB <--> node[1].phIn[3];
        
        node[0].phIn[2] <--> lines0[1].photonicInA;
        lines0[1].photonicOutA <--> node[3].phOut[0];
        node[0].phOut[2] <--> lines0[1].photonicInB;
        lines0[1].photonicOutB <--> node[3].phIn[0];
        
        node[1].phIn[2] <--> lines0[2].photonicInA;
        lines0[2].photonicOutA <--> node[2].phOut[0];
        node[1].phOut[2] <--> lines0[2].photonicInB;
        lines0[2].photonicOutB <--> node[2].phIn[0];
        
        node[2].phIn[3] <--> lines0[3].photonicInA;
        lines0[3].photonicOutA <--> node[3].phOut[1];
        node[2].phOut[3] <--> lines0[3].photonicInB;
        lines0[3].photonicOutB <--> node[3].phIn[1];
      
      
        
        //level ups
        node[0].elIn[(quadId % 2 == 0) ? 0 : 3]  <-- levelUpIn[0];
        node[0].elOut[(quadId % 2 == 0) ? 0 : 3]  --> levelUpOut[0];
        
        node[1].elIn[(quadId % 2 == 0) ? 1 : 0]  <-- levelUpIn[1];
        node[1].elOut[(quadId % 2 == 0) ? 1 : 0]  --> levelUpOut[1];
        
        node[2].elIn[(quadId % 2 == 0) ? 2 : 1]  <-- levelUpIn[2];
        node[2].elOut[(quadId % 2 == 0) ? 2 : 1]  --> levelUpOut[2];
        
        node[3].elIn[(quadId % 2 == 0) ? 3 : 2]  <-- levelUpIn[3];
        node[3].elOut[(quadId % 2 == 0) ? 3 : 2]  --> levelUpOut[3];
       
      	node[0].phIn[(quadId % 2 == 0) ? 0 : 3]  <--> phlevelUpIn[0];
        node[0].phOut[(quadId % 2 == 0) ? 0 : 3]  <--> phlevelUpOut[0];
        
        node[1].phIn[(quadId % 2 == 0) ? 1 : 0]  <--> phlevelUpIn[1];
        node[1].phOut[(quadId % 2 == 0) ? 1 : 0]  <--> phlevelUpOut[1];
        
        node[2].phIn[(quadId % 2 == 0) ? 2 : 1]  <--> phlevelUpIn[2];
        node[2].phOut[(quadId % 2 == 0) ? 2 : 1]  <--> phlevelUpOut[2];
        
        node[3].phIn[(quadId % 2 == 0) ? 3 : 2]  <--> phlevelUpIn[3];
        node[3].phOut[(quadId % 2 == 0) ? 3 : 2]  <--> phlevelUpOut[3];
        
        
        
        
        //gateways  -- when using memory
        node[0].elIn[(quadId % 2 == 0) ? 3 : 0]  <-- ElectronicChannel <-- gw[0].elOut[1] if (useIOplane);
        node[0].elOut[(quadId % 2 == 0) ? 3 : 0] --> ElectronicChannel --> gw[0].elIn[1] if (useIOplane);
        
        node[1].elIn[(quadId % 2 == 0) ? 0 : 1]  <-- ElectronicChannel <-- gw[1].elOut[1] if (useIOplane);
        node[1].elOut[(quadId % 2 == 0) ? 0 : 1] --> ElectronicChannel --> gw[1].elIn[1] if (useIOplane);
        
        node[2].elIn[(quadId % 2 == 0) ? 1 : 2]  <-- ElectronicChannel <-- gw[2].elOut[1] if (useIOplane);
        node[2].elOut[(quadId % 2 == 0) ? 1 : 2] --> ElectronicChannel --> gw[2].elIn[1] if (useIOplane);
        
        node[3].elIn[(quadId % 2 == 0) ? 2 : 3]  <-- ElectronicChannel <-- gw[3].elOut[1] if (useIOplane);
        node[3].elOut[(quadId % 2 == 0) ? 2 : 3] --> ElectronicChannel --> gw[3].elIn[1] if (useIOplane);
        
        node[0].phIn[(quadId % 2 == 0) ? 3 : 0]  <--> gw[0].phOut[1] if (useIOplane);
        node[0].phOut[(quadId % 2 == 0) ? 3 : 0] <--> gw[0].phIn[1] if (useIOplane);
        
        node[1].phIn[(quadId % 2 == 0) ? 0 : 1]  <--> gw[1].phOut[1] if (useIOplane);
        node[1].phOut[(quadId % 2 == 0) ? 0 : 1] <--> gw[1].phIn[1] if (useIOplane);
        
        node[2].phIn[(quadId % 2 == 0) ? 1 : 2]  <--> gw[2].phOut[1] if (useIOplane);
        node[2].phOut[(quadId % 2 == 0) ? 1 : 2] <--> gw[2].phIn[1] if (useIOplane);
        
        node[3].phIn[(quadId % 2 == 0) ? 2 : 3]  <--> gw[3].phOut[1] if (useIOplane);
        node[3].phOut[(quadId % 2 == 0) ? 2 : 3] <--> gw[3].phIn[1] if (useIOplane);
        
        
        //gateways  -- when not using memory
        node[0].elIn[(quadId % 2 == 0) ? 3 : 0]  <-- ElectronicChannel <-- ElGatewayIn[0] if (!useIOplane);
        node[0].elOut[(quadId % 2 == 0) ? 3 : 0] --> ElectronicChannel --> ElGatewayOut[0] if (!useIOplane);
        
        node[1].elIn[(quadId % 2 == 0) ? 0 : 1]  <-- ElectronicChannel <-- ElGatewayIn[1] if (!useIOplane);
        node[1].elOut[(quadId % 2 == 0) ? 0 : 1] --> ElectronicChannel --> ElGatewayOut[1] if (!useIOplane);
        
        node[2].elIn[(quadId % 2 == 0) ? 1 : 2]  <-- ElectronicChannel <-- ElGatewayIn[2] if (!useIOplane);
        node[2].elOut[(quadId % 2 == 0) ? 1 : 2] --> ElectronicChannel --> ElGatewayOut[2] if (!useIOplane);
        
        node[3].elIn[(quadId % 2 == 0) ? 2 : 3]  <-- ElectronicChannel <-- ElGatewayIn[3] if (!useIOplane);
        node[3].elOut[(quadId % 2 == 0) ? 2 : 3] --> ElectronicChannel --> ElGatewayOut[3] if (!useIOplane);
        
        
        node[0].phIn[(quadId % 2 == 0) ? 3 : 0]  <--> gw_nomem[0].out[1] if (!useIOplane);
        node[0].phOut[(quadId % 2 == 0) ? 3 : 0] <--> gw_nomem[0].in[1] if (!useIOplane);
        
        node[1].phIn[(quadId % 2 == 0) ? 0 : 1]  <--> gw_nomem[1].out[1] if (!useIOplane);
        node[1].phOut[(quadId % 2 == 0) ? 0 : 1] <--> gw_nomem[1].in[1] if (!useIOplane);
        
        node[2].phIn[(quadId % 2 == 0) ? 1 : 2]  <--> gw_nomem[2].out[1] if (!useIOplane);
        node[2].phOut[(quadId % 2 == 0) ? 1 : 2] <--> gw_nomem[2].in[1] if (!useIOplane);
        
        node[3].phIn[(quadId % 2 == 0) ? 2 : 3]  <--> gw_nomem[3].out[1] if (!useIOplane);
        node[3].phOut[(quadId % 2 == 0) ? 2 : 3] <--> gw_nomem[3].in[1] if (!useIOplane);
        
      
}



module SquareRootGrid
{
    
    parameters:
        
 		string networkPrefix;
 		int routerType;
 		int numOfGrids;
 		
 		int gridID;
 		
 		bool useIOplane;
 		
 		double coreSizeX;
 		double PathSeparation;
 	
 	
    	double pse1x2Width;
    	double pse2x2Width;
    	double crossWidth;
    	
    	double layoutDim1;
    	double layoutDim2;
    	double layoutDim3;
    	
 	 gates:
        input ElGatewayIn[16];
        input ElIOin[16];
        output ElGatewayOut[16];
        output ElIOout[16];
        
        inout PhGatewayIn[16];
        inout PhIOin[16];
        inout PhGatewayOut[16];
        inout PhIOout[16];
        
        inout PhSource[16];

        input levelUpIn[(gridID >= 0) ? 4 : 0];
        output levelUpOut[(gridID >= 0) ? 4 : 0];
        inout phlevelUpIn[(gridID >= 0) ? 4 : 0];
        inout phlevelUpOut[(gridID >= 0) ? 4 : 0];
       
       
     types:
        channel ElectronicChannel_2_2 extends electronicComponents.ElectronicChannel  //connections to the middle
        {
            spaceLengths = 2;
            routerLengths = 2;
        }

        channel ElectronicChannel_3_4 extends electronicComponents.ElectronicChannel   //N and S express channels
        {
            spaceLengths = 3;
            routerLengths = 4;
        }

        channel ElectronicChannel_1_2 extends electronicComponents.ElectronicChannel   //N and S express channels
        {
            spaceLengths = 1;
            routerLengths = 2;
        }
        
     submodules:
         
         quad[4] : SquareRootQuadrant {
             
              networkPrefix = networkPrefix;
        
        	  quadId = index;
        
    		  routerType = routerType;
    		  numOfGrids = numOfGrids;
         }
         
         middle[4]: HybridRouter {
            parameters:
                id = networkPrefix + string(index + 4) + ".";
				
				optSwitch = "NonBlockingSwitch4x4_P";
				
				elRouter = (numOfGrids > 1) ? 35 : 34;
				
				numX = sqrt(numOfGrids*16);
				numY = sqrt(numOfGrids*16);
				
				level = "NET2"; 
        }
        
        //middle lines
        lines0[4] : LineBidirectional {
             parameters:
                 Length_Line = coreSizeX - layoutDim1;
        }
        
        
         
        //quadrant express lines
        //N and S express
        linesNSx[2] : LineBidirectional {
             parameters:
                 Length_Line = coreSizeX * 3;  
        }
        bendsNSx[4] : BendBidirectional{
            
                 
        }
        //E and W express
        linesEWx[2] : LineBidirectional {
            parameters:
                 Length_Line = coreSizeX; 
        }
        bendsEWx[4] : BendBidirectional{
            
        }
        
        //lines into middle / level up
        linesHorizNSup[4] : LineBidirectional { 
             parameters:
                 Length_Line = coreSizeX;
        }
        linesVertNSup[4] : LineBidirectional { 
             parameters:
                 Length_Line = coreSizeX;
        }
        bendsNSup[4] : BendBidirectional {
            
        }
        
      
        linesVertEWup[4] : LineBidirectional { 
             parameters:
                 Length_Line = coreSizeX / 2;
        }
        bendsEWup[4] : BendBidirectional {
            
        }
        
        cross[4] : CrossBidirectional {   
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
        }
        
        Xperimeter[8] : CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
        }
        
        ExitNS[4] : LineBidirectional {
              parameters:
                 Length_Line = layoutDim1;
        }
        
        ExitEW[4] : LineBidirectional {
              parameters:
                 Length_Line = (coreSizeX - layoutDim1) + layoutDim1 + layoutDim1 / 2;
        }
        
        
         
         
     connections allowunconnected:
         
         for i=0..15 {
         	ElGatewayIn[i] --> quad[i / 4].ElGatewayIn[i % 4];
        	ElGatewayOut[i] <-- quad[i / 4].ElGatewayOut[i % 4];
         	
         	ElIOin[i] --> quad[i / 4].ElIOin[i % 4] if useIOplane;
        	ElIOout[i] <-- quad[i / 4].ElIOout[i % 4] if useIOplane;
        	
       		PhGatewayIn[i] <--> quad[i / 4].PhGatewayIn[i % 4];
        	PhGatewayOut[i] <--> quad[i / 4].PhGatewayOut[i % 4];
         	
         	PhIOin[i] <--> quad[i / 4].PhIOin[i % 4] if useIOplane;
        	PhIOout[i] <--> quad[i / 4].PhIOout[i % 4] if useIOplane;
        	
        	PhSource[i] <--> quad[i / 4].PhSource[i % 4];
             
         }
         
        //connect express lanes 
        
        //W
        quad[0].phlevelUpIn[3] <--> bendsEWx[0].photonicInnerBendOut;
        bendsEWx[0].photonicInnerBendIn <--> linesEWx[0].photonicOutA;
        
        linesEWx[0].photonicInA <--> bendsEWx[3].photonicInnerBendOut if (gridID == 0 || gridID == 3 || gridID < 0);
        linesEWx[0].photonicInA <--> Xperimeter[6].photonicOutN if (gridID == 1 || gridID == 2);
        Xperimeter[6].photonicInS <--> Xperimeter[7].photonicOutN if (gridID == 1 || gridID == 2);
        Xperimeter[7].photonicInS <--> bendsEWx[3].photonicInnerBendOut if (gridID == 1 || gridID == 2);
        
        bendsEWx[3].photonicInnerBendIn <--> quad[3].phlevelUpOut[0];
        
        
        quad[0].phlevelUpOut[3] <--> bendsEWx[0].photonicOuterBendIn;
        bendsEWx[0].photonicOuterBendOut <--> linesEWx[0].photonicInB;
         
        linesEWx[0].photonicOutB <--> bendsEWx[3].photonicOuterBendIn if (gridID == 0 || gridID == 3 || gridID < 0);
        linesEWx[0].photonicOutB <--> Xperimeter[6].photonicInN if (gridID == 1 || gridID == 2);
        Xperimeter[6].photonicOutS <--> Xperimeter[7].photonicInN if (gridID == 1 || gridID == 2);
        Xperimeter[7].photonicOutS <--> bendsEWx[3].photonicOuterBendIn if (gridID == 1 || gridID == 2);
        
        bendsEWx[3].photonicOuterBendOut <--> quad[3].phlevelUpIn[0];
        
        
        //E
        quad[1].phlevelUpIn[2] <--> bendsEWx[1].photonicInnerBendOut;
        bendsEWx[1].photonicInnerBendIn <--> linesEWx[1].photonicOutA;
        linesEWx[1].photonicInA <--> bendsEWx[2].photonicInnerBendOut if (gridID == 1 || gridID == 2 || gridID < 0);
       
        linesEWx[1].photonicInA <--> Xperimeter[2].photonicOutN if (gridID == 0 || gridID == 3);
        Xperimeter[2].photonicInS <--> Xperimeter[3].photonicOutN if (gridID == 0 || gridID == 3);
        Xperimeter[3].photonicInS <--> bendsEWx[2].photonicInnerBendOut if (gridID == 0 || gridID == 3);
        
        bendsEWx[2].photonicInnerBendIn <--> quad[2].phlevelUpOut[1];
        
        
        quad[1].phlevelUpOut[2] <--> bendsEWx[1].photonicOuterBendIn;
        bendsEWx[1].photonicOuterBendOut <--> linesEWx[1].photonicInB;
      
        linesEWx[1].photonicOutB <--> bendsEWx[2].photonicOuterBendIn if (gridID == 1 || gridID == 2 || gridID < 0);
        linesEWx[1].photonicOutB <--> Xperimeter[2].photonicInN if (gridID == 0 || gridID == 3);
        Xperimeter[2].photonicOutS <--> Xperimeter[3].photonicInN if (gridID == 0 || gridID == 3);
        Xperimeter[3].photonicOutS <--> bendsEWx[2].photonicOuterBendIn if (gridID == 0 || gridID == 3);
        
        bendsEWx[2].photonicOuterBendOut <--> quad[2].phlevelUpIn[1];
        
        
        //N
        quad[0].phlevelUpIn[0] <--> bendsNSx[0].photonicInnerBendOut;
        bendsNSx[0].photonicInnerBendIn <--> linesNSx[0].photonicOutA;
       
        linesNSx[0].photonicInA <--> bendsNSx[3].photonicInnerBendOut if (gridID == 0 || gridID == 1 || gridID < 0);
        linesNSx[0].photonicInA <--> Xperimeter[0].photonicOutN if (gridID == 3 || gridID == 2);
        Xperimeter[0].photonicInS <--> Xperimeter[1].photonicOutN if (gridID == 3 || gridID == 2);
        Xperimeter[1].photonicInS <--> bendsNSx[3].photonicInnerBendOut if (gridID == 3 || gridID == 2);
        
        bendsNSx[3].photonicInnerBendIn <--> quad[1].phlevelUpOut[1];
        
        
        quad[0].phlevelUpOut[0] <--> bendsNSx[0].photonicOuterBendIn;
        bendsNSx[0].photonicOuterBendOut <--> linesNSx[0].photonicInB;
        
        linesNSx[0].photonicOutB <--> bendsNSx[3].photonicOuterBendIn if (gridID == 0 || gridID == 1 || gridID < 0);
        linesNSx[0].photonicOutB <--> Xperimeter[0].photonicInN if (gridID == 3 || gridID == 2);
        Xperimeter[0].photonicOutS <--> Xperimeter[1].photonicInN if (gridID == 3 || gridID == 2);
        Xperimeter[1].photonicOutS <--> bendsNSx[3].photonicOuterBendIn if (gridID == 3 || gridID == 2);
        
        bendsNSx[3].photonicOuterBendOut <--> quad[1].phlevelUpIn[1];
        
        
        //S
        quad[2].phlevelUpIn[2] <--> bendsNSx[1].photonicInnerBendOut;
        bendsNSx[1].photonicInnerBendIn <--> linesNSx[1].photonicOutA;
       
        linesNSx[1].photonicInA <--> bendsNSx[2].photonicInnerBendOut if (gridID == 2 || gridID == 3 || gridID < 0);
        linesNSx[1].photonicInA <--> Xperimeter[4].photonicOutN if (gridID == 1 || gridID == 0);
        Xperimeter[4].photonicInS <--> Xperimeter[5].photonicOutN if (gridID == 1 || gridID == 0);
        Xperimeter[5].photonicInS <--> bendsNSx[2].photonicInnerBendOut if (gridID == 1 || gridID == 0);
        
        bendsNSx[2].photonicInnerBendIn <--> quad[3].phlevelUpOut[3];
        
   
        quad[2].phlevelUpOut[2] <--> bendsNSx[1].photonicOuterBendIn;
        bendsNSx[1].photonicOuterBendOut <--> linesNSx[1].photonicInB;
        
        linesNSx[1].photonicOutB <--> bendsNSx[2].photonicOuterBendIn if (gridID == 0 || gridID == 3 || gridID < 0);
        linesNSx[1].photonicOutB <--> Xperimeter[4].photonicInN if (gridID == 1 || gridID == 2);
        Xperimeter[4].photonicOutS <--> Xperimeter[5].photonicInN if (gridID == 1 || gridID == 2);
        Xperimeter[5].photonicOutS <--> bendsNSx[2].photonicOuterBendIn if (gridID == 1 || gridID == 2);
        
        bendsNSx[2].photonicOuterBendOut <--> quad[3].phlevelUpIn[3];
        
        
        quad[0].levelUpIn[3] <-- ElectronicChannel_3_4 <-- quad[3].levelUpOut[0];
        quad[0].levelUpOut[3] --> ElectronicChannel_3_4 --> quad[3].levelUpIn[0];
        
        quad[1].levelUpIn[2] <-- ElectronicChannel_3_4 <-- quad[2].levelUpOut[1];
        quad[1].levelUpOut[2] --> ElectronicChannel_3_4 --> quad[2].levelUpIn[1];
        
        quad[0].levelUpIn[0] <-- ElectronicChannel_3_4 <-- quad[1].levelUpOut[1];
        quad[0].levelUpOut[0] --> ElectronicChannel_3_4 --> quad[1].levelUpIn[1];
        
        quad[2].levelUpIn[2] <-- ElectronicChannel_3_4 <-- quad[3].levelUpOut[3];
        quad[2].levelUpOut[2] --> ElectronicChannel_3_4 --> quad[3].levelUpIn[3];
         
         
         
        //connect middles together
        middle[0].elIn[1] <-- ElectronicChannel <-- middle[1].elOut[3];
        middle[0].elOut[1] --> ElectronicChannel --> middle[1].elIn[3];
      
        middle[0].elIn[2] <-- ElectronicChannel <-- middle[3].elOut[0];
        middle[0].elOut[2] --> ElectronicChannel --> middle[3].elIn[0];
        
        middle[1].elIn[2] <-- ElectronicChannel <-- middle[2].elOut[0];
        middle[1].elOut[2] --> ElectronicChannel --> middle[2].elIn[0];
      
        middle[2].elIn[3] <-- ElectronicChannel <-- middle[3].elOut[1];
        middle[2].elOut[3] --> ElectronicChannel --> middle[3].elIn[1];
        
        middle[0].phIn[1] <--> lines0[0].photonicInA;
        lines0[0].photonicOutA <--> middle[1].phOut[3];
        middle[0].phOut[1] <--> lines0[0].photonicInB;
        lines0[0].photonicOutB <--> middle[1].phIn[3];
        
        middle[0].phIn[2] <--> lines0[1].photonicInA;
        lines0[1].photonicOutA <--> middle[3].phOut[0];
        middle[0].phOut[2] <--> lines0[1].photonicInB;
        lines0[1].photonicOutB <--> middle[3].phIn[0];
        
        middle[1].phIn[2] <--> lines0[2].photonicInA;
        lines0[2].photonicOutA <--> middle[2].phOut[0];
        middle[1].phOut[2] <--> lines0[2].photonicInB;
        lines0[2].photonicOutB <--> middle[2].phIn[0];
        
        middle[2].phIn[3] <--> lines0[3].photonicInA;
        lines0[3].photonicOutA <--> middle[3].phOut[1];
        middle[2].phOut[3] <--> lines0[3].photonicInB;
        lines0[3].photonicOutB <--> middle[3].phIn[1];
        
        
        //from quads to middle
    
        quad[0].levelUpIn[1] <-- ElectronicChannel_2_2 <-- middle[1].elOut[0] if (gridID != 2); 
        quad[0].levelUpOut[1] --> ElectronicChannel_2_2 --> middle[1].elIn[0] if (gridID != 2); 
        
        quad[0].phlevelUpIn[1] <--> linesHorizNSup[1].photonicOutA 				if (gridID != 2);
        linesHorizNSup[1].photonicInA <--> bendsNSup[1].photonicOuterBendOut 	if (gridID != 2);
        bendsNSup[1].photonicOuterBendIn <--> cross[0].photonicOutN 			if (gridID != 2);
       
        cross[0].photonicInS <--> linesVertNSup[1].photonicOutA;				
        linesVertNSup[1].photonicInA <--> middle[1].phOut[0];
        
        quad[0].phlevelUpOut[1] <--> linesHorizNSup[1].photonicInB 				if (gridID != 2);
        linesHorizNSup[1].photonicOutB <--> bendsNSup[1].photonicInnerBendIn 	if (gridID != 2);
        bendsNSup[1].photonicInnerBendOut <--> cross[0].photonicInN 			if (gridID != 2);
        
        cross[0].photonicOutS <--> linesVertNSup[1].photonicInB;
        linesVertNSup[1].photonicOutB <--> middle[1].phIn[0];
      
        
        quad[1].levelUpIn[0] <-- ElectronicChannel_2_2 <-- middle[0].elOut[0] if (gridID != 3); 
        quad[1].levelUpOut[0] --> ElectronicChannel_2_2 --> middle[0].elIn[0] if (gridID != 3); 
        
       	quad[1].phlevelUpIn[0] <--> linesHorizNSup[0].photonicOutA 				if (gridID != 3);
        linesHorizNSup[0].photonicInA <--> bendsNSup[0].photonicOuterBendOut 	if (gridID != 3);
        bendsNSup[0].photonicOuterBendIn <--> cross[0].photonicOutW 			if (gridID != 3);
        
        cross[0].photonicInE <--> linesVertNSup[0].photonicOutA;
        linesVertNSup[0].photonicInA <--> middle[0].phOut[0];
        
        quad[1].phlevelUpOut[0] <--> linesHorizNSup[0].photonicInB 				if (gridID != 3);
        linesHorizNSup[0].photonicOutB <--> bendsNSup[0].photonicInnerBendIn 	if (gridID != 3);
        bendsNSup[0].photonicInnerBendOut <--> cross[0].photonicInW 			if (gridID != 3);
        
        cross[0].photonicOutE <--> linesVertNSup[0].photonicInB;
        linesVertNSup[0].photonicOutB <--> middle[0].phIn[0];
        
        quad[2].levelUpIn[3] <-- ElectronicChannel_2_2 <-- middle[3].elOut[2] if (gridID != 0); 
        quad[2].levelUpOut[3] --> ElectronicChannel_2_2 --> middle[3].elIn[2] if (gridID != 0); 
        
       	quad[2].phlevelUpIn[3] <--> linesHorizNSup[3].photonicOutA 				if (gridID != 0);
        linesHorizNSup[3].photonicInA <--> bendsNSup[3].photonicOuterBendOut 	if (gridID != 0);
        bendsNSup[3].photonicOuterBendIn <--> cross[2].photonicOutW 			if (gridID != 0);
        
        cross[2].photonicInE <--> linesVertNSup[3].photonicOutA;
        linesVertNSup[3].photonicInA <--> middle[3].phOut[2];
        
        quad[2].phlevelUpOut[3] <--> linesHorizNSup[3].photonicInB 				if (gridID != 0);
        linesHorizNSup[3].photonicOutB <--> bendsNSup[3].photonicInnerBendIn 	if (gridID != 0);
        bendsNSup[3].photonicInnerBendOut <--> cross[2].photonicInW 			if (gridID != 0);
        
        cross[2].photonicOutE <--> linesVertNSup[3].photonicInB;
        linesVertNSup[3].photonicOutB <--> middle[3].phIn[2];
        
        quad[3].levelUpIn[2] <-- ElectronicChannel_2_2 <-- middle[2].elOut[2] if (gridID != 1); 
        quad[3].levelUpOut[2] --> ElectronicChannel_2_2 --> middle[2].elIn[2] if (gridID != 1); 
        
       	quad[3].phlevelUpIn[2] <--> linesHorizNSup[2].photonicOutA 				if (gridID != 1);
        linesHorizNSup[2].photonicInA <--> bendsNSup[2].photonicOuterBendOut 	if (gridID != 1);
        bendsNSup[2].photonicOuterBendIn <--> cross[2].photonicOutN 			if (gridID != 1);
        
        cross[2].photonicInS <--> linesVertNSup[2].photonicOutA;
        linesVertNSup[2].photonicInA <--> middle[2].phOut[2];
        
        quad[3].phlevelUpOut[2] <--> linesHorizNSup[2].photonicInB 				if (gridID != 1);
        linesHorizNSup[2].photonicOutB <--> bendsNSup[2].photonicInnerBendIn 	if (gridID != 1);
        bendsNSup[2].photonicInnerBendOut <--> cross[2].photonicInN 			if (gridID != 1);
        
        cross[2].photonicOutS <--> linesVertNSup[2].photonicInB;
        linesVertNSup[2].photonicOutB <--> middle[2].phIn[2];
        
        
        
        quad[0].levelUpIn[2] <-- ElectronicChannel_1_2 <-- middle[3].elOut[3] if (gridID != 1);  
        quad[0].levelUpOut[2] --> ElectronicChannel_1_2 --> middle[3].elIn[3] if (gridID != 1);  
        
        quad[0].phlevelUpIn[2] <-->  linesVertEWup[1].photonicOutA 			if (gridID != 1); 
        linesVertEWup[1].photonicInA <--> bendsEWup[3].photonicInnerBendOut if (gridID != 1); 
        bendsEWup[3].photonicInnerBendIn <--> cross[3].photonicOutN 		if (gridID != 1);
        
        cross[3].photonicInS <-->  middle[3].phOut[3]; 
       
        quad[0].phlevelUpOut[2] <--> linesVertEWup[1].photonicInB 			if (gridID != 1); 
        linesVertEWup[1].photonicOutB <--> bendsEWup[3].photonicOuterBendIn if (gridID != 1); 
        bendsEWup[3].photonicOuterBendOut <--> cross[3].photonicInN 		if (gridID != 1); 
         
        cross[3].photonicOutS <--> middle[3].phIn[3]; 
        
        
        quad[1].levelUpIn[3] <-- ElectronicChannel_1_2 <-- middle[2].elOut[1] if (gridID != 0);  
        quad[1].levelUpOut[3] --> ElectronicChannel_1_2 --> middle[2].elIn[1] if (gridID != 0);  
        
        quad[1].phlevelUpIn[3] <--> linesVertEWup[2].photonicOutA 			if (gridID != 0); 
        linesVertEWup[2].photonicInA <--> bendsEWup[2].photonicInnerBendOut if (gridID != 0); 
        bendsEWup[2].photonicInnerBendIn <--> cross[1].photonicOutW		if (gridID != 0); 
        
        cross[1].photonicInE <--> middle[2].phOut[1]; 
        
        quad[1].phlevelUpOut[3] <--> linesVertEWup[2].photonicInB 			if (gridID != 0); 
        linesVertEWup[2].photonicOutB <--> bendsEWup[2].photonicOuterBendIn if (gridID != 0); 
        bendsEWup[2].photonicOuterBendOut <--> cross[1].photonicInW 		if (gridID != 0);  
        
        cross[1].photonicOutE <--> middle[2].phIn[1]; 
        
        
        quad[2].levelUpIn[0] <-- ElectronicChannel_1_2 <-- middle[1].elOut[1] if (gridID != 3);  
        quad[2].levelUpOut[0] --> ElectronicChannel_1_2 --> middle[1].elIn[1] if (gridID != 3);  
        
        quad[2].phlevelUpIn[0] <--> linesVertEWup[3].photonicOutA 			if (gridID != 3); 
        linesVertEWup[3].photonicInA <--> bendsEWup[1].photonicInnerBendOut if (gridID != 3); 
        bendsEWup[1].photonicInnerBendIn <--> cross[1].photonicOutN 		if (gridID != 3);  
        
        cross[1].photonicInS <-->  middle[1].phOut[1]; 
       
        quad[2].phlevelUpOut[0] <--> linesVertEWup[3].photonicInB 			if (gridID != 3); 
        linesVertEWup[3].photonicOutB <--> bendsEWup[1].photonicOuterBendIn if (gridID != 3); 
        bendsEWup[1].photonicOuterBendOut <--> cross[1].photonicInN 		if (gridID != 3); 
        
        cross[1].photonicOutS <--> middle[1].phIn[1]; 
        
        
        quad[3].levelUpIn[1] <-- ElectronicChannel_1_2 <-- middle[0].elOut[3] if (gridID != 2);  
        quad[3].levelUpOut[1] --> ElectronicChannel_1_2 --> middle[0].elIn[3] if (gridID != 2);  
        
        quad[3].phlevelUpIn[1] <--> linesVertEWup[0].photonicOutA 			if (gridID != 2); 
        linesVertEWup[0].photonicInA <--> bendsEWup[0].photonicInnerBendOut if (gridID != 2); 
        bendsEWup[0].photonicInnerBendIn <--> cross[3].photonicOutE 		if (gridID != 2);  
        
        cross[3].photonicInW <--> middle[0].phOut[3]; 
        
        quad[3].phlevelUpOut[1] <--> linesVertEWup[0].photonicInB 			if (gridID != 2); 
        linesVertEWup[0].photonicOutB <--> bendsEWup[0].photonicOuterBendIn if (gridID != 2); 
        bendsEWup[0].photonicOuterBendOut <--> cross[3].photonicInE 		if (gridID != 2); 
        
        cross[3].photonicOutW <--> middle[0].phIn[3]; 
        
        
        
        //level ups
        cross[2].photonicInW <--> ExitNS[3].photonicOutA if (gridID == 0);
        ExitNS[3].photonicInA <--> Xperimeter[5].photonicOutW if (gridID == 0);
        Xperimeter[5].photonicInE <--> phlevelUpIn[3] if (gridID == 0);
        cross[2].photonicOutW <--> ExitNS[3].photonicInB if (gridID == 0);
        ExitNS[3].photonicOutB <--> Xperimeter[5].photonicInW if (gridID == 0);
        Xperimeter[5].photonicOutE <--> phlevelUpOut[3] if (gridID == 0);
        
        cross[1].photonicInW <--> ExitEW[1].photonicOutA if (gridID == 0);
        ExitEW[1].photonicInA <--> Xperimeter[3].photonicOutW if (gridID == 0);
        Xperimeter[3].photonicInE <--> phlevelUpIn[1] if (gridID == 0);
        cross[1].photonicOutW <--> ExitEW[1].photonicInB if (gridID == 0);
        ExitEW[1].photonicOutB <--> Xperimeter[3].photonicInW if (gridID == 0);
        Xperimeter[3].photonicOutE <--> phlevelUpOut[1] if (gridID == 0);
        
        quad[1].phlevelUpIn[3] <--> ExitEW[0].photonicOutA if(gridID == 0);
        ExitEW[0].photonicInA <--> Xperimeter[2].photonicOutW if(gridID == 0);
        Xperimeter[2].photonicInE <--> phlevelUpIn[0] if(gridID == 0);
        quad[1].phlevelUpOut[3]<--> ExitEW[0].photonicInB if(gridID == 0);
        ExitEW[0].photonicOutB <--> Xperimeter[2].photonicInW if(gridID == 0);
        Xperimeter[2].photonicOutE <--> phlevelUpOut[0] if(gridID == 0);
        
        quad[2].phlevelUpIn[3] <--> ExitNS[2].photonicOutA if(gridID == 0);
        ExitNS[2].photonicInA <--> Xperimeter[4].photonicOutW if(gridID == 0);
        Xperimeter[4].photonicInE <--> phlevelUpIn[2] if(gridID == 0);
        quad[2].phlevelUpOut[3]<--> ExitNS[2].photonicInB if(gridID == 0);
        ExitNS[2].photonicOutB <--> Xperimeter[4].photonicInW if(gridID == 0);
        Xperimeter[4].photonicOutE <--> phlevelUpOut[2] if(gridID == 0);
        
        
        
        cross[2].photonicInN <--> ExitNS[2].photonicOutA if (gridID == 1);
        ExitNS[2].photonicInA <--> Xperimeter[4].photonicOutW if (gridID == 1);
        Xperimeter[4].photonicInE <--> phlevelUpIn[3] if (gridID == 1);
        cross[2].photonicOutN <--> ExitNS[2].photonicInB if (gridID == 1);
        ExitNS[2].photonicOutB <--> Xperimeter[4].photonicInW if (gridID == 1);
        Xperimeter[4].photonicOutE <--> phlevelUpOut[3] if (gridID == 1);
        
        cross[3].photonicInN <--> ExitEW[2].photonicOutA if (gridID == 1);
        ExitEW[2].photonicInA <--> Xperimeter[6].photonicOutW if (gridID == 1);
        Xperimeter[6].photonicInE <--> phlevelUpIn[1] if (gridID == 1);
        cross[3].photonicOutN <--> ExitEW[2].photonicInB if (gridID == 1);
        ExitEW[2].photonicOutB <--> Xperimeter[6].photonicInW if (gridID == 1);
        Xperimeter[6].photonicOutE <--> phlevelUpOut[1] if (gridID == 1);
        
        
        quad[3].phlevelUpIn[2] <--> ExitNS[3].photonicOutA if(gridID == 1);
        ExitNS[3].photonicInA <--> Xperimeter[5].photonicOutW if(gridID ==1);
        Xperimeter[5].photonicInE <--> phlevelUpIn[2] if(gridID == 1);
        quad[3].phlevelUpOut[2]<--> ExitNS[3].photonicInB if(gridID == 1);
        ExitNS[3].photonicOutB <--> Xperimeter[5].photonicInW if(gridID == 1);
        Xperimeter[5].photonicOutE <--> phlevelUpOut[2] if(gridID == 1);
        
        quad[0].phlevelUpIn[2] <--> ExitEW[3].photonicOutA if(gridID == 1);
        ExitEW[3].photonicInA <--> Xperimeter[7].photonicOutW if(gridID == 1);
        Xperimeter[7].photonicInE <--> phlevelUpIn[0] if(gridID == 1);
        quad[0].phlevelUpOut[2]<--> ExitEW[3].photonicInB if(gridID == 1);
        ExitEW[3].photonicOutB <--> Xperimeter[7].photonicInW if(gridID == 1);
        Xperimeter[7].photonicOutE <--> phlevelUpOut[0] if(gridID == 1);
         
      
        
               
        cross[0].photonicInN <--> ExitNS[1].photonicOutA if (gridID == 2);
        ExitNS[1].photonicInA <--> Xperimeter[1].photonicOutW if (gridID == 2);
        Xperimeter[1].photonicInE <--> phlevelUpIn[0] if (gridID == 2);
        cross[0].photonicOutN <--> ExitNS[1].photonicInB if (gridID == 2);
        ExitNS[1].photonicOutB <--> Xperimeter[1].photonicInW if (gridID == 2);
        Xperimeter[1].photonicOutE <--> phlevelUpOut[0] if (gridID == 2);
        
        cross[3].photonicInE <--> ExitEW[3].photonicOutA if (gridID == 2);
        ExitEW[3].photonicInA <--> Xperimeter[7].photonicOutW if (gridID == 2);
        Xperimeter[7].photonicInE <--> phlevelUpIn[2] if (gridID == 2);
        cross[3].photonicOutE <--> ExitEW[3].photonicInB if (gridID == 2);
        ExitEW[3].photonicOutB <--> Xperimeter[7].photonicInW if (gridID == 2);
        Xperimeter[7].photonicOutE <--> phlevelUpOut[2] if (gridID == 2);
        
        quad[0].phlevelUpIn[1] <--> ExitNS[0].photonicOutA if(gridID == 2);
        ExitNS[0].photonicInA <--> Xperimeter[0].photonicOutW if(gridID ==2);
        Xperimeter[0].photonicInE <--> phlevelUpIn[1] if(gridID == 2);
        quad[0].phlevelUpOut[1]<--> ExitNS[0].photonicInB if(gridID == 2);
        ExitNS[0].photonicOutB <--> Xperimeter[0].photonicInW if(gridID == 2);
        Xperimeter[0].photonicOutE <--> phlevelUpOut[1] if(gridID == 2);
        
        quad[3].phlevelUpIn[1] <--> ExitEW[2].photonicOutA if(gridID == 2);
        ExitEW[2].photonicInA <--> Xperimeter[6].photonicOutW if(gridID == 2);
        Xperimeter[6].photonicInE <--> phlevelUpIn[3] if(gridID == 2);
        quad[3].phlevelUpOut[1]<--> ExitEW[2].photonicInB if(gridID == 2);
        ExitEW[2].photonicOutB <--> Xperimeter[6].photonicInW if(gridID == 2);
        Xperimeter[6].photonicOutE <--> phlevelUpOut[3] if(gridID == 2);
        

        
        cross[0].photonicInW <--> ExitNS[0].photonicOutA if (gridID == 3);
        ExitNS[0].photonicInA <--> Xperimeter[0].photonicOutW if (gridID == 3);
        Xperimeter[0].photonicInE <--> phlevelUpIn[0] if (gridID == 3);
        cross[0].photonicOutW <--> ExitNS[0].photonicInB if (gridID == 3);
        ExitNS[0].photonicOutB <--> Xperimeter[0].photonicInW if (gridID == 3);
        Xperimeter[0].photonicOutE <--> phlevelUpOut[0] if (gridID == 3);
        
        cross[1].photonicInN <--> ExitEW[0].photonicOutA if (gridID == 3);
        ExitEW[0].photonicInA <--> Xperimeter[2].photonicOutW if (gridID == 3);
        Xperimeter[2].photonicInE <--> phlevelUpIn[2] if (gridID == 3);
        cross[1].photonicOutN <--> ExitEW[0].photonicInB if (gridID == 3);
        ExitEW[0].photonicOutB <--> Xperimeter[2].photonicInW if (gridID == 3);
        Xperimeter[2].photonicOutE <--> phlevelUpOut[2] if (gridID == 3);
        
        quad[1].phlevelUpIn[0] <--> ExitNS[1].photonicOutA if(gridID == 3);
        ExitNS[1].photonicInA <--> Xperimeter[1].photonicOutW if(gridID ==3);
        Xperimeter[1].photonicInE <--> phlevelUpIn[1] if(gridID == 3);
        quad[1].phlevelUpOut[0]<--> ExitNS[1].photonicInB if(gridID == 3);
        ExitNS[1].photonicOutB <--> Xperimeter[1].photonicInW if(gridID == 3);
        Xperimeter[1].photonicOutE <--> phlevelUpOut[1] if(gridID == 3);
        
        quad[2].phlevelUpIn[0] <--> ExitEW[1].photonicOutA if(gridID == 3);
        ExitEW[1].photonicInA <--> Xperimeter[3].photonicOutW if(gridID == 3);
        Xperimeter[3].photonicInE <--> phlevelUpIn[3] if(gridID == 3);
        quad[2].phlevelUpOut[0]<--> ExitEW[1].photonicInB if(gridID == 3);
        ExitEW[1].photonicOutB <--> Xperimeter[3].photonicInW if(gridID == 3);
        Xperimeter[3].photonicOutE <--> phlevelUpOut[3] if(gridID == 3);
        
        
        
        middle[3].elIn[2] <-- levelUpIn[3] if (gridID == 0);
        middle[3].elOut[2] --> levelUpOut[3] if (gridID == 0);
        
        middle[2].elIn[1] <-- levelUpIn[1] if (gridID == 0);
        middle[2].elOut[1] --> levelUpOut[1] if (gridID == 0);
        
        quad[1].levelUpIn[3] <-- levelUpIn[0] if(gridID == 0);
        quad[1].levelUpOut[3] --> levelUpOut[0] if(gridID == 0);
        
        quad[2].levelUpIn[3] <-- levelUpIn[2] if(gridID == 0);
        quad[2].levelUpOut[3] --> levelUpOut[2] if(gridID == 0);
        
        
        middle[3].elIn[3] <-- levelUpIn[1] if (gridID == 1);
        middle[3].elOut[3] --> levelUpOut[1] if (gridID == 1);
        
        middle[2].elIn[2] <-- levelUpIn[3] if (gridID == 1);
        middle[2].elOut[2] --> levelUpOut[3] if (gridID == 1);
        
        quad[0].levelUpIn[2] <-- levelUpIn[0] if(gridID == 1);
        quad[0].levelUpOut[2] --> levelUpOut[0] if(gridID == 1);
        
        quad[3].levelUpIn[2] <-- levelUpIn[2] if(gridID == 1);
        quad[3].levelUpOut[2] --> levelUpOut[2] if(gridID == 1);
        
        
        middle[0].elIn[3] <-- levelUpIn[2] if (gridID == 2);
        middle[0].elOut[3] --> levelUpOut[2] if (gridID == 2);   
        
        middle[1].elIn[0] <-- levelUpIn[0] if (gridID == 2);
        middle[1].elOut[0] --> levelUpOut[0] if (gridID == 2);  
        
        quad[3].levelUpIn[1] <-- levelUpIn[3] if(gridID == 2);
        quad[3].levelUpOut[1] --> levelUpOut[3] if(gridID == 2);
        
        quad[0].levelUpIn[1] <-- levelUpIn[1] if(gridID == 2);
        quad[0].levelUpOut[1] --> levelUpOut[1] if(gridID == 2);
        
        middle[0].elIn[0] <-- levelUpIn[0] if (gridID == 3);
        middle[0].elOut[0] --> levelUpOut[0] if (gridID == 3);   
        
        middle[1].elIn[1] <-- levelUpIn[2] if (gridID == 3);
        middle[1].elOut[1] --> levelUpOut[2] if (gridID == 3);
        
        quad[1].levelUpIn[0] <-- levelUpIn[1] if(gridID == 3);
        quad[1].levelUpOut[0] --> levelUpOut[1] if(gridID == 3);
        
        quad[2].levelUpIn[0] <-- levelUpIn[3] if(gridID == 3);
        quad[2].levelUpOut[0] --> levelUpOut[3] if(gridID == 3);
        
}


//implements a 64-node square root grid
module SquareRootGrid8x8
{
    parameters:
  	
     

        int gridID;     //specifies where in the layout this grid is.  It matters for where the levelUp ports are placed.
       					//if just a 4x4 is being instantiated, specify id=0, then connect the level up ports together on the outside
       					//0-NW, 1-NE, 2-SE, 3-SW  (clockwise)

        int numOfEights;
        
        bool useIOplane;
        
        int routerType;
        
        string networkPrefix;
        
       double coreSizeX;
       
         double pse1x2Width;
    	double pse2x2Width;
    	double crossWidth;
    
    	double layoutDim1;
    	double layoutDim2;
    	double layoutDim3;
    	
    	double PathSeparation;
    	
    gates:
        input ElGatewayIn[64];
        input ElIOin[64];
        output ElGatewayOut[64];
        output ElIOout[64];
        
        inout PhGatewayIn[64];
        inout PhIOin[64];
        inout PhGatewayOut[64];
        inout PhIOout[64];
        
        inout PhSource[64];
        
        input levelUpIn[(gridID >= 0) ? 4 : 0];
        output levelUpOut[(gridID >= 0) ? 4 : 0];
        inout phlevelUpIn[(gridID >= 0) ? 4 : 0];
        inout phlevelUpOut[(gridID >= 0) ? 4 : 0];

     
        
     types:
        channel ElectronicChannel_3_5 extends electronicComponents.ElectronicChannel  
        {
            spaceLengths = 3;
            routerLengths = 5;
        }
        
   
        
    submodules:
        grid[4]: SquareRootGrid {
            parameters:
              networkPrefix = networkPrefix + string(index) + ".";
               gridID = index;
               numOfGrids = numOfEights*4;
               routerType = routerType;
                
     
        }
        
         middle[4]: HybridRouter {
            parameters:
                id = networkPrefix + string(index + 4) + ".";
			
				optSwitch = "NonBlockingSwitch4x4_P";
			
				elRouter = (numOfEights > 1) ? 35 : 34;
				
				numX = sqrt(numOfEights*64);
				numY = sqrt(numOfEights*64);
			
				level = "NET3";
        }
        
     
        
        
        middleLines[4] : LineBidirectional {
         	 parameters:
                 Length_Line = coreSizeX - layoutDim1;
        }
        
        
        
        Xup[4] : CrossBidirectional {
             parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
        }
        
        X1up[2] : CrossBidirectional {
             parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
        }
        
        X3up[2] : CrossBidirectional {
             parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
        }

		exLines[4] : LineBidirectional {
		     parameters:
                 Length_Line = layoutDim1;
		}
		
		toMidLinesLong[8] : LineBidirectional {
		 	parameters:
		 	    Length_Line = layoutDim1 * 2 + (coreSizeX - layoutDim1);   
		}
		
		
		
		linesHorizNSup[4] : LineBidirectional { 
             parameters:
                 Length_Line = ((coreSizeX - layoutDim1)) / 2 + layoutDim1 / 2 + layoutDim1;
        }
       
        bendsNSup[4] : BendBidirectional {
            
        }
        
     
        linesVertEWup[4] : LineBidirectional { 
             parameters:
                 Length_Line = ((coreSizeX - layoutDim1)) / 2 + layoutDim1 / 2 + layoutDim1;
        }
        
        linesMidEWup[4] : LineBidirectional { 
             parameters:
                 Length_Line = ((coreSizeX - layoutDim1)) / 2 + layoutDim1 / 2 + layoutDim1;
        }
        
        bendsEWup[4] : BendBidirectional {
            
        }
        
        exitEWlines[4] : LineBidirectional {
         	   parameters:
         	       Length_Line = coreSizeX * 2;
        }
        
        Xperimeter[4] : CrossBidirectional {
              parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
        }
        
        exitNSlines[4] : LineBidirectional {
         	parameters:
         		Length_Line = coreSizeX;
        }

    connections allowunconnected:
        for i=0..3, for j=0..15 {
            grid[i].ElGatewayIn[j] <-- ElGatewayIn[j+i*16];
            grid[i].ElGatewayOut[j] --> ElGatewayOut[j+i*16];
            grid[i].PhGatewayIn[j] <--> PhGatewayIn[j+i*16];
            grid[i].PhGatewayOut[j] <--> PhGatewayOut[j+i*16];
         
            
            grid[i].PhSource[j] <--> PhSource[j+i*16];
            
            ElIOout[j+i*16] <-- grid[i].ElIOout[j] if useIOplane;
            ElIOin[j+i*16] --> grid[i].ElIOin[j] if useIOplane;
            PhIOout[j+i*16] <--> grid[i].PhIOout[j] if useIOplane;
            PhIOin[j+i*16] <--> grid[i].PhIOin[j] if useIOplane;
           
       }
        
        
        middle[0].elIn[1] <-- ElectronicChannel <-- middle[1].elOut[3];
        middle[0].elOut[1] --> ElectronicChannel --> middle[1].elIn[3];
     

        middle[1].elIn[2] <-- ElectronicChannel <-- middle[2].elOut[0];
        middle[1].elOut[2] --> ElectronicChannel --> middle[2].elIn[0];
       

        middle[2].elIn[3] <-- ElectronicChannel <-- middle[3].elOut[1];
        middle[2].elOut[3] --> ElectronicChannel --> middle[3].elIn[1];
      
        
        middle[3].elIn[0] <-- ElectronicChannel <-- middle[0].elOut[2];
        middle[3].elOut[0] --> ElectronicChannel --> middle[0].elIn[2];
       
        
        
        
        //leveling up to the top level middle
        middle[0].elIn[0] <-- ElectronicChannel_3_5 <-- grid[1].levelUpOut[1] if (gridID != 3);
        middle[0].elOut[0] --> ElectronicChannel_3_5 --> grid[1].levelUpIn[1] if (gridID != 3);
        
        middle[0].elIn[0] <-- levelUpIn[0] if (gridID == 3);
      	middle[0].elOut[0] --> levelUpOut[0] if (gridID == 3);
      	
      	grid[1].levelUpOut[1] --> levelUpOut[1] if (gridID == 3);
      	grid[1].levelUpIn[1] <-- levelUpIn[1] if (gridID == 3);
      
        
        middle[0].elIn[3] <-- ElectronicChannel_3_5 <-- grid[3].levelUpOut[0] if (gridID != 2);
        middle[0].elOut[3] --> ElectronicChannel_3_5 --> grid[3].levelUpIn[0] if (gridID != 2);
        
        middle[0].elIn[3] <-- levelUpIn[2] if (gridID == 2);
      	middle[0].elOut[3] --> levelUpOut[2] if (gridID == 2);
      	
      	grid[3].levelUpOut[0] --> levelUpOut[3] if (gridID == 2);
      	grid[3].levelUpIn[0] <-- levelUpIn[3] if (gridID == 2);
       
        
        middle[1].elIn[0] <-- ElectronicChannel_3_5 <-- grid[0].levelUpOut[1] if (gridID != 2);
        middle[1].elOut[0] --> ElectronicChannel_3_5 --> grid[0].levelUpIn[1] if (gridID != 2);
        
        middle[1].elIn[0] <-- levelUpIn[0] if (gridID == 2);
      	middle[1].elOut[0] --> levelUpOut[0] if (gridID == 2);
      	
      	grid[0].levelUpOut[1] --> levelUpOut[1] if (gridID == 2);
      	grid[0].levelUpIn[1] <-- levelUpIn[1] if (gridID == 2);
      
        
        middle[1].elIn[1] <-- ElectronicChannel_3_5 <-- grid[2].levelUpOut[0] if (gridID != 3);
        middle[1].elOut[1] --> ElectronicChannel_3_5 --> grid[2].levelUpIn[0] if (gridID != 3);
        
        middle[1].elIn[1] <-- levelUpIn[2] if (gridID == 3);
      	middle[1].elOut[1] --> levelUpOut[2] if (gridID == 3);
      	
      	grid[2].levelUpOut[0] --> levelUpOut[3] if (gridID == 3);
      	grid[2].levelUpIn[0] <-- levelUpIn[3] if (gridID == 3);
       
        
        middle[3].elIn[3] <-- ElectronicChannel_3_5 <-- grid[0].levelUpOut[3] if (gridID != 1);
        middle[3].elOut[3] --> ElectronicChannel_3_5 --> grid[0].levelUpIn[3] if (gridID != 1);
        
        middle[3].elIn[3] <-- levelUpIn[1] if (gridID == 1);
      	middle[3].elOut[3] --> levelUpOut[1] if (gridID == 1);
      	
      	grid[0].levelUpOut[3] --> levelUpOut[0] if (gridID == 1);
      	grid[0].levelUpIn[3] <-- levelUpIn[0] if (gridID == 1);
       
        
        middle[3].elIn[2] <-- ElectronicChannel_3_5 <-- grid[2].levelUpOut[2] if (gridID != 0);
        middle[3].elOut[2] --> ElectronicChannel_3_5 --> grid[2].levelUpIn[2] if (gridID != 0);
        
        middle[3].elIn[2] <-- levelUpIn[3] if (gridID == 0);
      	middle[3].elOut[2] --> levelUpOut[3] if (gridID == 0);
      	
      	grid[2].levelUpOut[2] --> levelUpOut[2] if (gridID == 0);
      	grid[2].levelUpIn[2] <-- levelUpIn[2] if (gridID == 0);
       
        
        middle[2].elIn[2] <-- ElectronicChannel_3_5 <-- grid[3].levelUpOut[2] if (gridID != 1);
        middle[2].elOut[2] --> ElectronicChannel_3_5 --> grid[3].levelUpIn[2] if (gridID != 1);
        
        middle[2].elIn[2] <-- levelUpIn[3] if (gridID == 1);
      	middle[2].elOut[2] --> levelUpOut[3] if (gridID == 1);
      	
      	grid[3].levelUpOut[2] --> levelUpOut[2] if (gridID == 1);
      	grid[3].levelUpIn[2] <-- levelUpIn[2] if (gridID == 1);
       
        
        middle[2].elIn[1] <-- ElectronicChannel_3_5 <-- grid[1].levelUpOut[3] if (gridID != 0);
        middle[2].elOut[1] --> ElectronicChannel_3_5 --> grid[1].levelUpIn[3] if (gridID != 0);
        
        middle[2].elIn[1] <-- levelUpIn[1] if (gridID == 0);
      	middle[2].elOut[1] --> levelUpOut[1] if (gridID == 0);
      	
      	grid[1].levelUpOut[3] --> levelUpOut[0] if (gridID == 0);
      	grid[1].levelUpIn[3] <-- levelUpIn[0] if (gridID == 0);
      	
      	
     
        
        
        //inter grid express lanes
        grid[0].levelUpIn[0] <-- ElectronicChannel_3_5 <-- grid[1].levelUpOut[0];
        grid[0].levelUpOut[0] --> ElectronicChannel_3_5 --> grid[1].levelUpIn[0];
      
        
        grid[1].levelUpIn[2] <-- ElectronicChannel_3_5 <-- grid[2].levelUpOut[1];
        grid[1].levelUpOut[2] --> ElectronicChannel_3_5 --> grid[2].levelUpIn[1];
      
        
        grid[2].levelUpIn[3] <-- ElectronicChannel_3_5 <-- grid[3].levelUpOut[3];
        grid[2].levelUpOut[3] --> ElectronicChannel_3_5 --> grid[3].levelUpIn[3];
     
        
        grid[3].levelUpIn[1] <-- ElectronicChannel_3_5 <-- grid[0].levelUpOut[2];
        grid[3].levelUpOut[1] --> ElectronicChannel_3_5 --> grid[0].levelUpIn[2];
       
        
        
        
        
        //--------------------photonic connections----------------------//
        middle[0].phIn[1] <--> middleLines[0].photonicOutA;
        middleLines[0].photonicInA <--> middle[1].phOut[3];
        middle[0].phOut[1] <--> middleLines[0].photonicInB;
        middleLines[0].photonicOutB <--> middle[1].phIn[3];

        middle[1].phIn[2] <--> middleLines[1].photonicOutA;
        middleLines[1].photonicInA <--> middle[2].phOut[0];
        middle[1].phOut[2] <--> middleLines[1].photonicInB;
        middleLines[1].photonicOutB <--> middle[2].phIn[0];

        middle[2].phIn[3] <--> middleLines[2].photonicOutA;
        middleLines[2].photonicInA <--> middle[3].phOut[1];
        middle[2].phOut[3] <--> middleLines[2].photonicInB;
        middleLines[2].photonicOutB <--> middle[3].phIn[1];
        
        middle[3].phIn[0] <--> middleLines[3].photonicOutA;
        middleLines[3].photonicInA <--> middle[0].phOut[2];
        middle[3].phOut[0] <--> middleLines[3].photonicInB;
        middleLines[3].photonicOutB <--> middle[0].phIn[2];
        
        
        
        //leveling up to the top level middle
        
       //long lines to mid from main crossings
        Xup[0].photonicOutS <--> toMidLinesLong[1].photonicInA;
        toMidLinesLong[1].photonicOutA <--> middle[1].phIn[0];
        Xup[0].photonicInS <--> toMidLinesLong[1].photonicOutB;
        toMidLinesLong[1].photonicInB <--> middle[1].phOut[0];
        
        Xup[0].photonicOutW <--> toMidLinesLong[0].photonicInA;
        toMidLinesLong[0].photonicOutA <--> middle[0].phIn[0];
        Xup[0].photonicInW <--> toMidLinesLong[0].photonicOutB;
        toMidLinesLong[0].photonicInB <--> middle[0].phOut[0];
        
        Xup[2].photonicOutN <--> toMidLinesLong[4].photonicInA;
        toMidLinesLong[4].photonicOutA <--> middle[2].phIn[2];
        Xup[2].photonicInN <--> toMidLinesLong[4].photonicOutB;
        toMidLinesLong[4].photonicInB <--> middle[2].phOut[2];
        
        Xup[2].photonicOutW <--> toMidLinesLong[5].photonicInA;
        toMidLinesLong[5].photonicOutA <--> middle[3].phIn[2];
        Xup[2].photonicInW <--> toMidLinesLong[5].photonicOutB;
        toMidLinesLong[5].photonicInB <--> middle[3].phOut[2];
        
        X1up[0].photonicOutW <--> toMidLinesLong[2].photonicInA;
        toMidLinesLong[2].photonicOutA <--> middle[1].phIn[1];
        X1up[0].photonicInW <--> toMidLinesLong[2].photonicOutB;
        toMidLinesLong[2].photonicInB <--> middle[1].phOut[1];
        
        X1up[1].photonicOutW <--> toMidLinesLong[3].photonicInA;
        toMidLinesLong[3].photonicOutA <--> middle[2].phIn[1];
        X1up[1].photonicInW <--> toMidLinesLong[3].photonicOutB;
        toMidLinesLong[3].photonicInB <--> middle[2].phOut[1];
        
        X3up[0].photonicOutE <--> toMidLinesLong[7].photonicInA;
        toMidLinesLong[7].photonicOutA <--> middle[0].phIn[3];
        X3up[0].photonicInE <--> toMidLinesLong[7].photonicOutB;
        toMidLinesLong[7].photonicInB <--> middle[0].phOut[3];
        
        X3up[1].photonicOutE <--> toMidLinesLong[6].photonicInA;
        toMidLinesLong[6].photonicOutA <--> middle[3].phIn[3];
        X3up[1].photonicInE <--> toMidLinesLong[6].photonicOutB;
        toMidLinesLong[6].photonicInB <--> middle[3].phOut[3];
        
        //bends and lines connected to the grids
        grid[0].phlevelUpIn[1] <--> bendsNSup[1].photonicInnerBendOut;
        
        bendsNSup[1].photonicInnerBendIn <--> linesHorizNSup[0].photonicOutA 	if(gridID != 2);
        linesHorizNSup[0].photonicInA  <--> Xup[0].photonicOutN					if(gridID != 2);
        
        Xup[0].photonicOutN <--> exitNSlines[0].photonicInA						if(gridID == 2);
        exitNSlines[0].photonicOutA <--> Xperimeter[0].photonicInS				if(gridID == 2);
        Xperimeter[0].photonicOutN <--> phlevelUpOut[0]							if(gridID == 2);
        
        bendsNSup[1].photonicInnerBendIn <--> phlevelUpIn[1]					if(gridID == 2);
        
        
        grid[0].phlevelUpOut[1] <--> bendsNSup[1].photonicOuterBendIn;	
       
        bendsNSup[1].photonicOuterBendOut <--> linesHorizNSup[0].photonicInB	if(gridID != 2);
        linesHorizNSup[0].photonicOutB  <--> Xup[0].photonicInN 				if(gridID != 2);
        
        Xup[0].photonicInN <--> exitNSlines[0].photonicOutB						if(gridID == 2);
        exitNSlines[0].photonicInB <--> Xperimeter[0].photonicOutS				if(gridID == 2);
        Xperimeter[0].photonicInN <--> phlevelUpIn[0]							if(gridID == 2);
        
        bendsNSup[1].photonicOuterBendOut <--> phlevelUpOut[1]					if(gridID == 2);
        
        
        grid[1].phlevelUpIn[1] <--> bendsNSup[0].photonicInnerBendOut;
        
        bendsNSup[0].photonicInnerBendIn <--> linesHorizNSup[1].photonicOutA 	if(gridID != 3);
        linesHorizNSup[1].photonicInA  <--> Xup[0].photonicOutE					if(gridID != 3);
        
        Xup[0].photonicOutE <--> exitNSlines[1].photonicInA						if(gridID == 3);
        exitNSlines[1].photonicOutA <--> Xperimeter[1].photonicInS				if(gridID == 3);
        Xperimeter[1].photonicOutN <--> phlevelUpOut[0]							if(gridID == 3);
        
        bendsNSup[0].photonicInnerBendIn <--> phlevelUpIn[1]					if(gridID == 3);
        
        
        grid[1].phlevelUpOut[1] <--> bendsNSup[0].photonicOuterBendIn;	
       
        bendsNSup[0].photonicOuterBendOut <--> linesHorizNSup[1].photonicInB	if(gridID != 3);
        linesHorizNSup[1].photonicOutB  <--> Xup[0].photonicInE 				if(gridID != 3);
        
        Xup[0].photonicInE <--> exitNSlines[1].photonicOutB						if(gridID == 3);
        exitNSlines[1].photonicInB <--> Xperimeter[1].photonicOutS				if(gridID == 3);
        Xperimeter[1].photonicInN <--> phlevelUpIn[0]							if(gridID == 3);
        
        bendsNSup[0].photonicOuterBendOut <--> phlevelUpOut[1]					if(gridID == 3);
        
        
        
        grid[3].phlevelUpIn[2] <--> bendsNSup[2].photonicInnerBendOut;
        
        bendsNSup[2].photonicInnerBendIn <--> linesHorizNSup[2].photonicOutA 	if(gridID != 1);
        linesHorizNSup[2].photonicInA  <--> Xup[2].photonicOutS					if(gridID != 1);
        
        Xup[2].photonicOutS <--> exitNSlines[2].photonicInA						if(gridID == 1);
        exitNSlines[2].photonicOutA <--> Xperimeter[2].photonicInS				if(gridID == 1);
        Xperimeter[2].photonicOutN <--> phlevelUpOut[3]							if(gridID == 1);
        
        bendsNSup[2].photonicInnerBendIn <--> phlevelUpIn[2]					if(gridID == 1);
        
        
        grid[3].phlevelUpOut[2] <--> bendsNSup[2].photonicOuterBendIn;	
       
        bendsNSup[2].photonicOuterBendOut <--> linesHorizNSup[2].photonicInB	if(gridID != 1);
        linesHorizNSup[2].photonicOutB  <--> Xup[2].photonicInS 				if(gridID != 1);
        
        Xup[2].photonicInS <--> exitNSlines[2].photonicOutB						if(gridID == 1);
        exitNSlines[2].photonicInB <--> Xperimeter[2].photonicOutS				if(gridID == 1);
        Xperimeter[2].photonicInN <--> phlevelUpIn[3]							if(gridID == 1);
        
        bendsNSup[2].photonicOuterBendOut <--> phlevelUpOut[2]					if(gridID == 1);
   
   
   
        grid[2].phlevelUpIn[2] <--> bendsNSup[3].photonicInnerBendOut;
        
        bendsNSup[3].photonicInnerBendIn <--> linesHorizNSup[3].photonicOutA 	if(gridID != 0);
        linesHorizNSup[3].photonicInA  <--> Xup[2].photonicOutE					if(gridID != 0);
        
        Xup[2].photonicOutE <--> exitNSlines[3].photonicInA						if(gridID == 0);
        exitNSlines[3].photonicOutA <--> Xperimeter[3].photonicInS				if(gridID == 0);
        Xperimeter[3].photonicOutN <--> phlevelUpOut[3]							if(gridID == 0);
        
        bendsNSup[3].photonicInnerBendIn <--> phlevelUpIn[2]					if(gridID == 0);
        
        
        grid[2].phlevelUpOut[2] <--> bendsNSup[3].photonicOuterBendIn;	
       
        bendsNSup[3].photonicOuterBendOut <--> linesHorizNSup[3].photonicInB	if(gridID != 0);
        linesHorizNSup[3].photonicOutB  <--> Xup[2].photonicInE 				if(gridID != 0);
        
        Xup[2].photonicInE <--> exitNSlines[3].photonicOutB						if(gridID == 0);
        exitNSlines[3].photonicInB <--> Xperimeter[3].photonicOutS				if(gridID == 0);
        Xperimeter[3].photonicInN <--> phlevelUpIn[3]							if(gridID == 0);
        
        bendsNSup[3].photonicOuterBendOut <--> phlevelUpOut[2]					if(gridID == 0);
        
       
         
        
        grid[2].phlevelUpIn[0] <--> bendsEWup[0].photonicInnerBendOut;
        
        bendsEWup[0].photonicInnerBendIn <--> linesVertEWup[0].photonicOutA if (gridID != 3 || gridID < 0);
        linesVertEWup[0].photonicInA  <--> Xup[1].photonicOutS 				if (gridID != 3 || gridID < 0);
        
        bendsEWup[0].photonicInnerBendIn <--> exitEWlines[0].photonicOutA 	if (gridID == 3);
        exitEWlines[0].photonicInA <--> phlevelUpIn[3]						if (gridID == 3);
        
        Xup[1].photonicOutS <--> exitEWlines[2].photonicInA					if (gridID == 3);
        exitEWlines[2].photonicOutA <--> phlevelUpOut[2]					if (gridID == 3);
         
        Xup[1].photonicInN <--> linesMidEWup[0].photonicOutA;
        linesMidEWup[0].photonicInA <--> X1up[0].photonicOutE;
       
        grid[2].phlevelUpOut[0] <--> bendsEWup[0].photonicOuterBendIn;
        
        bendsEWup[0].photonicOuterBendOut <--> linesVertEWup[0].photonicInB if (gridID != 3 || gridID < 0);
        linesVertEWup[0].photonicOutB <--> Xup[1].photonicInS 				if (gridID != 3 || gridID < 0);
        
        bendsEWup[0].photonicOuterBendOut <--> exitEWlines[0].photonicInB	if (gridID == 3);
        exitEWlines[0].photonicOutB <--> phlevelUpOut[3]					if (gridID == 3);
        
        Xup[1].photonicInS <--> exitEWlines[2].photonicOutB					if (gridID == 3);
        exitEWlines[2].photonicInB <--> phlevelUpIn[2]						if (gridID == 3);
        
        Xup[1].photonicOutN <--> linesMidEWup[0].photonicInB;
        linesMidEWup[0].photonicOutB <--> X1up[0].photonicInE;
        
   
        
        grid[1].phlevelUpIn[3] <--> bendsEWup[1].photonicInnerBendOut;
        
        bendsEWup[1].photonicInnerBendIn <--> linesVertEWup[1].photonicOutA if (gridID != 0 || gridID < 0);
        linesVertEWup[1].photonicInA  <--> Xup[1].photonicOutE 				if (gridID != 0 || gridID < 0);
        
        bendsEWup[1].photonicInnerBendIn <--> exitEWlines[1].photonicOutA 	if (gridID == 0);
        exitEWlines[1].photonicInA <--> phlevelUpIn[0]						if (gridID == 0);
        
        Xup[1].photonicOutE <--> exitEWlines[0].photonicInA					if (gridID == 0);
        exitEWlines[0].photonicOutA <--> phlevelUpOut[1]					if (gridID == 0);
         
        Xup[1].photonicInW <--> linesMidEWup[1].photonicOutA;
        linesMidEWup[1].photonicInA <--> X1up[1].photonicOutE;
       
        grid[1].phlevelUpOut[3] <--> bendsEWup[1].photonicOuterBendIn;
        
        bendsEWup[1].photonicOuterBendOut <--> linesVertEWup[1].photonicInB if (gridID != 0 || gridID < 0);
        linesVertEWup[1].photonicOutB <--> Xup[1].photonicInE 				if (gridID != 0 || gridID < 0);
        
        bendsEWup[1].photonicOuterBendOut <--> exitEWlines[1].photonicInB	if (gridID == 0);
        exitEWlines[1].photonicOutB <--> phlevelUpOut[0]					if (gridID == 0);
        
        Xup[1].photonicInE <--> exitEWlines[0].photonicOutB					if (gridID == 0);
        exitEWlines[0].photonicInB <--> phlevelUpIn[1]						if (gridID == 0);
        
        Xup[1].photonicOutW <--> linesMidEWup[1].photonicInB;
        linesMidEWup[1].photonicOutB <--> X1up[1].photonicInE;
        
        
        
        
        grid[0].phlevelUpIn[3] <--> bendsEWup[2].photonicInnerBendOut;
        
        bendsEWup[2].photonicInnerBendIn <--> linesVertEWup[2].photonicOutA if (gridID != 1 || gridID < 0);
        linesVertEWup[2].photonicInA  <--> Xup[3].photonicOutW 				if (gridID != 1 || gridID < 0);
        
        bendsEWup[2].photonicInnerBendIn <--> exitEWlines[2].photonicOutA 	if (gridID == 1);
        exitEWlines[2].photonicInA <--> phlevelUpIn[0]						if (gridID == 1);
        
        Xup[3].photonicOutW <--> exitEWlines[3].photonicInA					if (gridID == 1);
        exitEWlines[3].photonicOutA <--> phlevelUpOut[1]					if (gridID == 1);
         
        Xup[3].photonicInE <--> linesMidEWup[2].photonicOutA;
        linesMidEWup[2].photonicInA <--> X3up[1].photonicOutW;
       
        grid[0].phlevelUpOut[3] <--> bendsEWup[2].photonicOuterBendIn;
        
        bendsEWup[2].photonicOuterBendOut <--> linesVertEWup[2].photonicInB if (gridID != 1 || gridID < 0);
        linesVertEWup[2].photonicOutB <--> Xup[3].photonicInW 				if (gridID != 1 || gridID < 0);
        
        bendsEWup[2].photonicOuterBendOut <--> exitEWlines[2].photonicInB	if (gridID == 1);
        exitEWlines[2].photonicOutB <--> phlevelUpOut[0]					if (gridID == 1);
        
        Xup[3].photonicInW <--> exitEWlines[3].photonicOutB					if (gridID == 1);
        exitEWlines[3].photonicInB <--> phlevelUpIn[1]						if (gridID == 1);
        
        Xup[3].photonicOutE <--> linesMidEWup[2].photonicInB;
        linesMidEWup[2].photonicOutB <--> X3up[1].photonicInW;
        
        
        
        
        grid[3].phlevelUpIn[0] <--> bendsEWup[3].photonicInnerBendOut;
        
        bendsEWup[3].photonicInnerBendIn <--> linesVertEWup[3].photonicOutA if (gridID != 2 || gridID < 0);
        linesVertEWup[3].photonicInA  <--> Xup[3].photonicOutS 				if (gridID != 2 || gridID < 0);
        
        bendsEWup[3].photonicInnerBendIn <--> exitEWlines[2].photonicOutA 	if (gridID == 2);
        exitEWlines[2].photonicInA <--> phlevelUpIn[3]						if (gridID == 2);
        
        Xup[3].photonicOutS <--> exitEWlines[3].photonicInA					if (gridID == 2);
        exitEWlines[3].photonicOutA <--> phlevelUpOut[2]					if (gridID == 2);
         
        Xup[3].photonicInN <--> linesMidEWup[3].photonicOutA;
        linesMidEWup[3].photonicInA <--> X3up[0].photonicOutW;
       
        grid[3].phlevelUpOut[0] <--> bendsEWup[3].photonicOuterBendIn;
        
        bendsEWup[3].photonicOuterBendOut <--> linesVertEWup[3].photonicInB if (gridID != 2 || gridID < 0);
        linesVertEWup[3].photonicOutB <--> Xup[3].photonicInS 				if (gridID != 2 || gridID < 0);
        
        bendsEWup[3].photonicOuterBendOut <--> exitEWlines[2].photonicInB	if (gridID == 2);
        exitEWlines[2].photonicOutB <--> phlevelUpOut[3]					if (gridID == 2);
        
        Xup[3].photonicInS <--> exitEWlines[3].photonicOutB					if (gridID == 2);
        exitEWlines[3].photonicInB <--> phlevelUpIn[2]						if (gridID == 2);
        
        Xup[3].photonicOutN <--> linesMidEWup[3].photonicInB;
        linesMidEWup[3].photonicOutB <--> X3up[0].photonicInW;
             
      
        
        //inter grid express lanes
        grid[0].phlevelUpIn[0] <--> exLines[0].photonicOutA  		if (gridID == 0 || gridID == 1 || gridID < 0);
        
        grid[0].phlevelUpIn[0] <--> Xperimeter[0].photonicOutE 		if (gridID == 2 || gridID == 3);
        Xperimeter[0].photonicInW <--> Xperimeter[1].photonicOutE 	if (gridID == 2 || gridID == 3);
        Xperimeter[1].photonicInW <--> exLines[0].photonicOutA 		if (gridID == 2 || gridID == 3);
        
        exLines[0].photonicInA <--> grid[1].phlevelUpOut[0];
        
        grid[0].phlevelUpOut[0] <--> exLines[0].photonicInB  		if (gridID == 0 || gridID == 1 || gridID < 0);
         
        grid[0].phlevelUpOut[0] <--> Xperimeter[0].photonicInE 		if (gridID == 2 || gridID == 3);
        Xperimeter[0].photonicOutW <--> Xperimeter[1].photonicInE 	if (gridID == 2 || gridID == 3);
        Xperimeter[1].photonicOutW <--> exLines[0].photonicInB 		if (gridID == 2 || gridID == 3);
        
        exLines[0].photonicOutB <--> grid[1].phlevelUpIn[0];
        
        grid[1].phlevelUpIn[2] <--> X1up[0].photonicOutN;
        X1up[0].photonicInS <--> exLines[1].photonicOutA;
        exLines[1].photonicInA <--> X1up[1].photonicOutN;
        X1up[1].photonicInS <--> grid[2].phlevelUpOut[1];
        
        grid[1].phlevelUpOut[2] <--> X1up[0].photonicInN;
        X1up[0].photonicOutS <--> exLines[1].photonicInB;
        exLines[1].photonicOutB <--> X1up[1].photonicInN;
        X1up[1].photonicOutS <--> grid[2].phlevelUpIn[1];
        
         
        grid[2].phlevelUpIn[3] <--> exLines[2].photonicOutA  		if (gridID == 2 || gridID == 3 || gridID < 0);
        
        grid[2].phlevelUpIn[3] <--> Xperimeter[2].photonicOutE 		if (gridID == 0 || gridID == 1);
        Xperimeter[2].photonicInW <--> Xperimeter[3].photonicOutE 	if (gridID == 0 || gridID == 1);
        Xperimeter[3].photonicInW <--> exLines[2].photonicOutA 		if (gridID == 0 || gridID == 1);
        
        exLines[2].photonicInA <--> grid[3].phlevelUpOut[3];
        
        grid[2].phlevelUpOut[3] <--> exLines[2].photonicInB  		if (gridID == 2 || gridID == 3 || gridID < 0);
         
        grid[2].phlevelUpOut[3] <--> Xperimeter[2].photonicInE 		if (gridID == 0 || gridID == 1);
        Xperimeter[2].photonicOutW <--> Xperimeter[3].photonicInE 	if (gridID == 0 || gridID == 1);
        Xperimeter[3].photonicOutW <--> exLines[2].photonicInB 		if (gridID == 0 || gridID == 1);
        
        exLines[2].photonicOutB <--> grid[3].phlevelUpIn[3];
        
        
        grid[3].phlevelUpIn[1] <--> X3up[0].photonicOutN;
        X3up[0].photonicInS <--> exLines[3].photonicOutA;
        exLines[3].photonicInA <--> X3up[1].photonicOutN;
        X3up[1].photonicInS <--> grid[0].phlevelUpOut[2];
        
        grid[3].phlevelUpOut[1] <--> X3up[0].photonicInN;
        X3up[0].photonicOutS <--> exLines[3].photonicInB;
        exLines[3].photonicOutB <--> X3up[1].photonicInN;
        X3up[1].photonicOutS <--> grid[0].phlevelUpIn[2];
        
}

//implements a 64-node square root grid
module SquareRootGrid16x16
{
    parameters:
  	
       

        int gridID;     //specifies where in the layout this grid is.  It matters for where the levelUp ports are placed.
       					//if just a 4x4 is being instantiated, specify id=0, then connect the level up ports together on the outside
       					//0-NW, 1-NE, 2-SE, 3-SW  (clockwise)

        int numOfSixteens;
        
        bool useIOplane;
        
        int routerType;
        
        string networkPrefix;
        
       double coreSizeX;
       
         double pse1x2Width;
    	double pse2x2Width;
    	double crossWidth;
    
    	double layoutDim1;
    	double layoutDim2;
    	double layoutDim3;
    	
    	double PathSeparation;
    	
    gates:
        input ElGatewayIn[256];
        input ElIOin[256];
        output ElGatewayOut[256];
        output ElIOout[256];
        
        inout PhGatewayIn[256];
        inout PhIOin[256];
        inout PhGatewayOut[256];
        inout PhIOout[256];
        
        inout PhSource[256];

     
        
     types:
        channel ElectronicChannel_3_5 extends electronicComponents.ElectronicChannel  
        {
            spaceLengths = 3;
            routerLengths = 5;
        }
        
         channel ElectronicChannel_8_10 extends electronicComponents.ElectronicChannel  
        {
            spaceLengths = 8;
            routerLengths = 10;
        }
        
   
        
    submodules:
        grid[4]: SquareRootGrid8x8 {
            parameters:
              networkPrefix = networkPrefix + string(index) + ".";
               gridID = index;
               numOfEights = numOfSixteens*4;
               routerType = routerType;
                
     
        }
        
         middle[4]: HybridRouter {
            parameters:
                id = networkPrefix + string(index + 4) + ".";
			
				optSwitch = "NonBlockingSwitch4x4_P";
			
				elRouter = 34;
				
				numX = sqrt(numOfSixteens*256);
				numY = sqrt(numOfSixteens*256);
			
				level = "NET4";
        }
        
     
        
        
        middleLines[4] : LineBidirectional {
         	 parameters:
                 Length_Line = coreSizeX - layoutDim1;
        }
        
        
        
        Xup[4] : CrossBidirectional {
             parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
        }
        
        X1up[2] : CrossBidirectional {
             parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
        }
        
        X3up[2] : CrossBidirectional {
             parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
        }

		exLines[4] : LineBidirectional {
		     parameters:
                 Length_Line = layoutDim1;
		}
		
		toMidLinesLong[8] : LineBidirectional {
		 	parameters:
		 	    Length_Line = coreSizeX * 4;   
		}
		
		
		
		linesHorizNSup[4] : LineBidirectional { 
             parameters:
                 Length_Line = ((coreSizeX - layoutDim1)) / 2 + layoutDim1 / 2 + layoutDim1;
        }
       
        bendsNSup[4] : BendBidirectional {
            
        }
        
     
        linesVertEWup[4] : LineBidirectional { 
             parameters:
                 Length_Line = ((coreSizeX - layoutDim1)) / 2 + layoutDim1 / 2 + layoutDim1;
        }
        
        linesMidEWup[4] : LineBidirectional { 
             parameters:
                 Length_Line = ((coreSizeX - layoutDim1)) / 2 + layoutDim1 / 2 + layoutDim1;
        }
        
        bendsEWup[4] : BendBidirectional {
            
        }

    connections allowunconnected:
        for i=0..3, for j=0..63 {
            grid[i].ElGatewayIn[j] <-- ElGatewayIn[j+i*64];
            grid[i].ElGatewayOut[j] --> ElGatewayOut[j+i*64];
            grid[i].PhGatewayIn[j] <--> PhGatewayIn[j+i*64];
            grid[i].PhGatewayOut[j] <--> PhGatewayOut[j+i*64];
         
            
            grid[i].PhSource[j] <--> PhSource[j+i*64];
            
            ElIOout[j+i*64] <-- grid[i].ElIOout[j] if useIOplane;
            ElIOin[j+i*64] --> grid[i].ElIOin[j] if useIOplane;
            PhIOout[j+i*64] <--> grid[i].PhIOout[j] if useIOplane;
            PhIOin[j+i*64] <--> grid[i].PhIOin[j] if useIOplane;
           
       }
        
        
        middle[0].elIn[1] <-- ElectronicChannel <-- middle[1].elOut[3];
        middle[0].elOut[1] --> ElectronicChannel --> middle[1].elIn[3];
     

        middle[1].elIn[2] <-- ElectronicChannel <-- middle[2].elOut[0];
        middle[1].elOut[2] --> ElectronicChannel --> middle[2].elIn[0];
       

        middle[2].elIn[3] <-- ElectronicChannel <-- middle[3].elOut[1];
        middle[2].elOut[3] --> ElectronicChannel --> middle[3].elIn[1];
      
        
        middle[3].elIn[0] <-- ElectronicChannel <-- middle[0].elOut[2];
        middle[3].elOut[0] --> ElectronicChannel --> middle[0].elIn[2];
       
        
        
        
        //leveling up to the top level middle
        middle[0].elIn[0] <-- ElectronicChannel_8_10 <-- grid[1].levelUpOut[1];
        middle[0].elOut[0] --> ElectronicChannel_8_10 --> grid[1].levelUpIn[1];
      
        
        middle[0].elIn[3] <-- ElectronicChannel_8_10 <-- grid[3].levelUpOut[0];
        middle[0].elOut[3] --> ElectronicChannel_8_10 --> grid[3].levelUpIn[0];
       
        
        middle[1].elIn[0] <-- ElectronicChannel_8_10 <-- grid[0].levelUpOut[1];
        middle[1].elOut[0] --> ElectronicChannel_8_10 --> grid[0].levelUpIn[1];
      
        
        middle[1].elIn[1] <-- ElectronicChannel_8_10 <-- grid[2].levelUpOut[0];
        middle[1].elOut[1] --> ElectronicChannel_8_10 --> grid[2].levelUpIn[0];
       
        
        middle[3].elIn[3] <-- ElectronicChannel_8_10 <-- grid[0].levelUpOut[3];
        middle[3].elOut[3] --> ElectronicChannel_8_10 --> grid[0].levelUpIn[3];
       
        
        middle[3].elIn[2] <-- ElectronicChannel_8_10 <-- grid[2].levelUpOut[2];
        middle[3].elOut[2] --> ElectronicChannel_8_10 --> grid[2].levelUpIn[2];
       
        
        middle[2].elIn[2] <-- ElectronicChannel_8_10 <-- grid[3].levelUpOut[2];
        middle[2].elOut[2] --> ElectronicChannel_8_10 --> grid[3].levelUpIn[2];
       
        
        middle[2].elIn[1] <-- ElectronicChannel_8_10 <-- grid[1].levelUpOut[3];
        middle[2].elOut[1] --> ElectronicChannel_8_10 --> grid[1].levelUpIn[3];
      
     
        
        
        //inter grid express lanes
        grid[0].levelUpIn[0] <-- ElectronicChannel_3_5 <-- grid[1].levelUpOut[0];
        grid[0].levelUpOut[0] --> ElectronicChannel_3_5 --> grid[1].levelUpIn[0];
      
        
        grid[1].levelUpIn[2] <-- ElectronicChannel_3_5 <-- grid[2].levelUpOut[1];
        grid[1].levelUpOut[2] --> ElectronicChannel_3_5 --> grid[2].levelUpIn[1];
      
        
        grid[2].levelUpIn[3] <-- ElectronicChannel_3_5 <-- grid[3].levelUpOut[3];
        grid[2].levelUpOut[3] --> ElectronicChannel_3_5 --> grid[3].levelUpIn[3];
     
        
        grid[3].levelUpIn[1] <-- ElectronicChannel_3_5 <-- grid[0].levelUpOut[2];
        grid[3].levelUpOut[1] --> ElectronicChannel_3_5 --> grid[0].levelUpIn[2];
       
        
        
        
        
        //--------------------photonic connections----------------------//
        middle[0].phIn[1] <--> middleLines[0].photonicOutA;
        middleLines[0].photonicInA <--> middle[1].phOut[3];
        middle[0].phOut[1] <--> middleLines[0].photonicInB;
        middleLines[0].photonicOutB <--> middle[1].phIn[3];

        middle[1].phIn[2] <--> middleLines[1].photonicOutA;
        middleLines[1].photonicInA <--> middle[2].phOut[0];
        middle[1].phOut[2] <--> middleLines[1].photonicInB;
        middleLines[1].photonicOutB <--> middle[2].phIn[0];

        middle[2].phIn[3] <--> middleLines[2].photonicOutA;
        middleLines[2].photonicInA <--> middle[3].phOut[1];
        middle[2].phOut[3] <--> middleLines[2].photonicInB;
        middleLines[2].photonicOutB <--> middle[3].phIn[1];
        
        middle[3].phIn[0] <--> middleLines[3].photonicOutA;
        middleLines[3].photonicInA <--> middle[0].phOut[2];
        middle[3].phOut[0] <--> middleLines[3].photonicInB;
        middleLines[3].photonicOutB <--> middle[0].phIn[2];
        
        
        
        //leveling up to the top level middle
        
       //long lines to mid from main crossings
        Xup[0].photonicOutS <--> toMidLinesLong[1].photonicInA;
        toMidLinesLong[1].photonicOutA <--> middle[1].phIn[0];
        Xup[0].photonicInS <--> toMidLinesLong[1].photonicOutB;
        toMidLinesLong[1].photonicInB <--> middle[1].phOut[0];
        
        Xup[0].photonicOutW <--> toMidLinesLong[0].photonicInA;
        toMidLinesLong[0].photonicOutA <--> middle[0].phIn[0];
        Xup[0].photonicInW <--> toMidLinesLong[0].photonicOutB;
        toMidLinesLong[0].photonicInB <--> middle[0].phOut[0];
        
        Xup[2].photonicOutN <--> toMidLinesLong[4].photonicInA;
        toMidLinesLong[4].photonicOutA <--> middle[2].phIn[2];
        Xup[2].photonicInN <--> toMidLinesLong[4].photonicOutB;
        toMidLinesLong[4].photonicInB <--> middle[2].phOut[2];
        
        Xup[2].photonicOutW <--> toMidLinesLong[5].photonicInA;
        toMidLinesLong[5].photonicOutA <--> middle[3].phIn[2];
        Xup[2].photonicInW <--> toMidLinesLong[5].photonicOutB;
        toMidLinesLong[5].photonicInB <--> middle[3].phOut[2];
        
        X1up[0].photonicOutW <--> toMidLinesLong[2].photonicInA;
        toMidLinesLong[2].photonicOutA <--> middle[1].phIn[1];
        X1up[0].photonicInW <--> toMidLinesLong[2].photonicOutB;
        toMidLinesLong[2].photonicInB <--> middle[1].phOut[1];
        
        X1up[1].photonicOutW <--> toMidLinesLong[3].photonicInA;
        toMidLinesLong[3].photonicOutA <--> middle[2].phIn[1];
        X1up[1].photonicInW <--> toMidLinesLong[3].photonicOutB;
        toMidLinesLong[3].photonicInB <--> middle[2].phOut[1];
        
        X3up[0].photonicOutE <--> toMidLinesLong[7].photonicInA;
        toMidLinesLong[7].photonicOutA <--> middle[0].phIn[3];
        X3up[0].photonicInE <--> toMidLinesLong[7].photonicOutB;
        toMidLinesLong[7].photonicInB <--> middle[0].phOut[3];
        
        X3up[1].photonicOutE <--> toMidLinesLong[6].photonicInA;
        toMidLinesLong[6].photonicOutA <--> middle[3].phIn[3];
        X3up[1].photonicInE <--> toMidLinesLong[6].photonicOutB;
        toMidLinesLong[6].photonicInB <--> middle[3].phOut[3];
        
        //bends and lines connected to the grids
        grid[0].phlevelUpIn[1] <--> linesHorizNSup[0].photonicOutA;
        linesHorizNSup[0].photonicInA <--> bendsNSup[1].photonicInnerBendOut;
        bendsNSup[1].photonicInnerBendIn <--> Xup[0].photonicOutN;
        
        grid[0].phlevelUpOut[1] <--> linesHorizNSup[0].photonicInB;
        linesHorizNSup[0].photonicOutB <--> bendsNSup[1].photonicOuterBendIn;
        bendsNSup[1].photonicOuterBendOut <--> Xup[0].photonicInN;
        
        grid[1].phlevelUpIn[1] <--> linesHorizNSup[1].photonicOutA;
        linesHorizNSup[1].photonicInA <--> bendsNSup[0].photonicInnerBendOut;
        bendsNSup[0].photonicInnerBendIn <--> Xup[0].photonicOutE;
        
        grid[1].phlevelUpOut[1] <--> linesHorizNSup[1].photonicInB;
        linesHorizNSup[1].photonicOutB <--> bendsNSup[0].photonicOuterBendIn;
        bendsNSup[0].photonicOuterBendOut <--> Xup[0].photonicInE;
        
        
        grid[3].phlevelUpIn[2] <--> linesHorizNSup[2].photonicOutA;
        linesHorizNSup[2].photonicInA <--> bendsNSup[2].photonicInnerBendOut;
        bendsNSup[2].photonicInnerBendIn <--> Xup[2].photonicOutS;
        
        grid[3].phlevelUpOut[2] <--> linesHorizNSup[2].photonicInB;
        linesHorizNSup[2].photonicOutB <--> bendsNSup[2].photonicOuterBendIn;
        bendsNSup[2].photonicOuterBendOut <--> Xup[2].photonicInS;
        
        grid[2].phlevelUpIn[2] <--> linesHorizNSup[3].photonicOutA;
        linesHorizNSup[3].photonicInA <--> bendsNSup[3].photonicInnerBendOut;
        bendsNSup[3].photonicInnerBendIn <--> Xup[2].photonicOutE;
        
        grid[2].phlevelUpOut[2] <--> linesHorizNSup[3].photonicInB;
        linesHorizNSup[3].photonicOutB <--> bendsNSup[3].photonicOuterBendIn;
        bendsNSup[3].photonicOuterBendOut <--> Xup[2].photonicInE;
         
        
        grid[2].phlevelUpIn[0] <--> Xup[1].photonicOutS;
        Xup[1].photonicInN <--> linesVertEWup[0].photonicOutA;
        linesVertEWup[0].photonicInA <--> bendsEWup[0].photonicInnerBendOut;
        bendsEWup[0].photonicInnerBendIn <--> linesMidEWup[0].photonicOutA;
        linesMidEWup[0].photonicInA <--> X1up[0].photonicOutE;
       
        grid[2].phlevelUpOut[0] <--> Xup[1].photonicInS;
        Xup[1].photonicOutN <--> linesVertEWup[0].photonicInB;
        linesVertEWup[0].photonicOutB <--> bendsEWup[0].photonicOuterBendIn;
        bendsEWup[0].photonicOuterBendOut <--> linesMidEWup[0].photonicInB;
        linesMidEWup[0].photonicOutB <--> X1up[0].photonicInE;
        
        
        grid[1].phlevelUpIn[3] <--> Xup[1].photonicOutE;
        Xup[1].photonicInW <--> linesVertEWup[1].photonicOutA;
        linesVertEWup[1].photonicInA <--> bendsEWup[1].photonicInnerBendOut;
        bendsEWup[1].photonicInnerBendIn <--> linesMidEWup[1].photonicOutA;
        linesMidEWup[1].photonicInA <--> X1up[1].photonicOutE;
       
        grid[1].phlevelUpOut[3] <--> Xup[1].photonicInE;
        Xup[1].photonicOutW <--> linesVertEWup[1].photonicInB;
        linesVertEWup[1].photonicOutB <--> bendsEWup[1].photonicOuterBendIn;
        bendsEWup[1].photonicOuterBendOut <--> linesMidEWup[1].photonicInB;
        linesMidEWup[1].photonicOutB <--> X1up[1].photonicInE;
        
        
        
        grid[0].phlevelUpIn[3] <--> Xup[3].photonicOutW;
        Xup[3].photonicInE <--> linesVertEWup[2].photonicOutA;
        linesVertEWup[2].photonicInA <--> bendsEWup[2].photonicInnerBendOut;
        bendsEWup[2].photonicInnerBendIn <--> linesMidEWup[2].photonicOutA;
        linesMidEWup[2].photonicInA <--> X3up[1].photonicOutW;
       
        grid[0].phlevelUpOut[3] <--> Xup[3].photonicInW;
        Xup[3].photonicOutE <--> linesVertEWup[2].photonicInB;
        linesVertEWup[2].photonicOutB <--> bendsEWup[2].photonicOuterBendIn;
        bendsEWup[2].photonicOuterBendOut <--> linesMidEWup[2].photonicInB;
        linesMidEWup[2].photonicOutB <--> X3up[1].photonicInW;
        
        
        grid[3].phlevelUpIn[0] <--> Xup[3].photonicOutS;
        Xup[3].photonicInN <--> linesVertEWup[3].photonicOutA;
        linesVertEWup[3].photonicInA <--> bendsEWup[3].photonicInnerBendOut;
        bendsEWup[3].photonicInnerBendIn <--> linesMidEWup[3].photonicOutA;
        linesMidEWup[3].photonicInA <--> X3up[0].photonicOutW;
       
        grid[3].phlevelUpOut[0] <--> Xup[3].photonicInS;
        Xup[3].photonicOutN <--> linesVertEWup[3].photonicInB;
        linesVertEWup[3].photonicOutB <--> bendsEWup[3].photonicOuterBendIn;
        bendsEWup[3].photonicOuterBendOut <--> linesMidEWup[3].photonicInB;
        linesMidEWup[3].photonicOutB <--> X3up[0].photonicInW;
        
      
        
        //inter grid express lanes
        grid[0].phlevelUpIn[0] <--> exLines[0].photonicOutA;
        exLines[0].photonicInA <--> grid[1].phlevelUpOut[0];
        
        grid[0].phlevelUpOut[0] <--> exLines[0].photonicOutB;
        exLines[0].photonicInB <--> grid[1].phlevelUpIn[0];
        
        grid[1].phlevelUpIn[2] <--> X1up[0].photonicOutN;
        X1up[0].photonicInS <--> exLines[1].photonicOutA;
        exLines[1].photonicInA <--> X1up[1].photonicOutN;
        X1up[1].photonicInS <--> grid[2].phlevelUpOut[1];
        
        grid[1].phlevelUpOut[2] <--> X1up[0].photonicInN;
        X1up[0].photonicOutS <--> exLines[1].photonicInB;
        exLines[1].photonicOutB <--> X1up[1].photonicInN;
        X1up[1].photonicOutS <--> grid[2].phlevelUpIn[1];
        
        grid[2].phlevelUpIn[3] <--> exLines[2].photonicOutA;
         exLines[2].photonicInA <-->grid[3].phlevelUpOut[3];
         
        grid[2].phlevelUpOut[3] <--> exLines[2].photonicOutB;
         exLines[2].photonicInB <-->grid[3].phlevelUpIn[3];
        
        grid[3].phlevelUpIn[1] <--> X3up[0].photonicOutN;
        X3up[0].photonicInS <--> exLines[3].photonicOutA;
        exLines[3].photonicInA <--> X3up[1].photonicOutN;
        X3up[1].photonicInS <--> grid[0].phlevelUpOut[2];
        
        grid[3].phlevelUpOut[1] <--> X3up[0].photonicInN;
        X3up[0].photonicOutS <--> exLines[3].photonicInB;
        exLines[3].photonicOutB <--> X3up[1].photonicInN;
        X3up[1].photonicOutS <--> grid[0].phlevelUpIn[2];
        
}


