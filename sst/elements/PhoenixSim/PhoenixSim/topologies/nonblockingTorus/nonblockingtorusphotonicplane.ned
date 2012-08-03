package topologies.nonblockingTorus;

import topologies.nonblockingTorus.NonblockingTorusTile*;
import photonic.devices.UTurnBidirectional;

import electronicComponents.ElectronicChannel;


module NonblockingTorusPhotonicPlane
{
    parameters:
        int numOfNodesX;
        double coreSizeX;
        double coreSizeY;
     
    gates:        
        inout electronicGatewayIn[numOfNodesX * 2];
        inout electronicGatewayOut[numOfNodesX * 2];

        inout photonicMemoryIn[numOfNodesX * 2];
        inout photonicMemoryOut[numOfNodesX * 2];
        
        input electronicControlInMemory[numOfNodesX * 2];
        input electronicControlInProc[numOfNodesX * 2];
        
        output electronicControlOutMemory[numOfNodesX * 2];
        output electronicControlOutProc[numOfNodesX * 2];
        
      
    submodules:
        tileNE_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)]: NonblockingTorusTileNE_Empty {
            parameters:
                nodeId = index*2 + (numOfNodesX%4==0 ? 1 : 0) + (floor(index*2/numOfNodesX)+(numOfNodesX%4==0 ? 1 : 0))*numOfNodesX;
                dispValueX = (numOfNodesX%4==0 ? 120+80 : 120);
                dispValueY = (numOfNodesX%4==0 ? 120+80 : 120);
                dispValueZ = numOfNodesX>>1;
                  numIF = 1;
                  numX = numOfNodesX;
                @display("p=$dispValueX,$dispValueY,m,$dispValueZ,160,160;b=50,50,,blue,,1");
        }
        tileNW_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)]: NonblockingTorusTileNW_Empty {
            parameters:
                nodeId = index*2 + (numOfNodesX%4==0 ? 1 : 0) + (floor(index*2/numOfNodesX)+(numOfNodesX%4==0 ? 0 : 1))*numOfNodesX;
                dispValueX = (numOfNodesX%4==0 ? 120+80 : 120);
                dispValueY = (numOfNodesX%4==0 ? 120 : 120+80);
                dispValueZ = numOfNodesX>>1;
                  numIF = 1;
                    numX = numOfNodesX;
                @display("p=$dispValueX,$dispValueY,m,$dispValueZ,160,160;b=50,50,,red,,1");
        }
        tileSE_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)]: NonblockingTorusTileSE_Empty {
            parameters:
                nodeId = index*2 + (numOfNodesX%4==0 ? 0 : 1) + (floor(index*2/numOfNodesX)+(numOfNodesX%4==0 ? 1 : 0))*numOfNodesX;
                dispValueX = (numOfNodesX%4==0 ? 120 : 120+80);
                dispValueY = (numOfNodesX%4==0 ? 120+80 : 120);
                dispValueZ = numOfNodesX>>1;
                  numIF = 1;
                    numX = numOfNodesX;
                @display("p=$dispValueX,$dispValueY,m,$dispValueZ,160,160;b=50,50,,yellow,,1");
        }
        tileSW_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)]: NonblockingTorusTileSW_Empty {
            parameters:
                nodeId = index*2 + (numOfNodesX%4==0 ? 0 : 1) + (floor(index*2/numOfNodesX)+(numOfNodesX%4==0 ? 0 : 1))*numOfNodesX;
                dispValueX = (numOfNodesX%4==0 ? 120 : 120+80);
                dispValueY = (numOfNodesX%4==0 ? 120 : 120+80);
                dispValueZ = numOfNodesX>>1;
                  numIF = 1;
                    numX = numOfNodesX;
                @display("p=$dispValueX,$dispValueY,m,$dispValueZ,160,160;b=50,50,,green,,1");


        }
        tileNE_North[numOfNodesX>>2]: NonblockingTorusTileNE_PopulatedNorth {
            parameters:
                nodeId = (numOfNodesX - 1 - (index*2+(numOfNodesX%4==0?0:1)))*numOfNodesX+numOfNodesX - 1 - (index*2+(numOfNodesX%4==0?0:1));
                dispValueX = (numOfNodesX%4==0 ? (numOfNodesX-1)*80-index*160 : ((numOfNodesX-1)*80-index*160)-80);
                dispValueY = (numOfNodesX%4==0 ? (numOfNodesX-1)*80-index*160 : ((numOfNodesX-1)*80-index*160)-80);
                @display("p=120,120,e,$dispValueX,$dispValueY;b=50,50,,grey,,1");
                numIF = 1;
                numX = numOfNodesX;
                
          
        }
        tileNW_North[numOfNodesX+2>>2]: NonblockingTorusTileNW_PopulatedNorth {
            parameters:
                nodeId = (numOfNodesX - 1 - (index*2+(numOfNodesX%4==0?1:0)))*numOfNodesX+index*2+(numOfNodesX%4==0?1:0);
                dispValueX = (numOfNodesX%4==0 ? index*160+80 : index*160);
                dispValueY = (numOfNodesX%4==0 ? ((numOfNodesX-1)*80-index*160)-80 : (numOfNodesX-1)*80-index*160);
                @display("p=120,120,e,$dispValueX,$dispValueY;b=50,50,,grey,,1");
                  numIF = 1;
                    numX = numOfNodesX;
                  
                        
           
        }
        tileSE_North[numOfNodesX>>2]: NonblockingTorusTileSE_PopulatedNorth {
            parameters:
                nodeId = (numOfNodesX - 1 - (index*2+(numOfNodesX%4==0?0:1)))*numOfNodesX+index*2+(numOfNodesX%4==0?0:1);
                dispValueX = (numOfNodesX%4==0 ? index*160 : 80+index*160);
                dispValueY = (numOfNodesX%4==0 ? (numOfNodesX-1)*80-index*160 : ((numOfNodesX-1)*80-index*160)-80);
                @display("p=120,120,e,$dispValueX,$dispValueY;b=50,50,,grey,,1");
                  numIF = 1;
                    numX = numOfNodesX;
                  
                        
            
        }
        tileSW_North[numOfNodesX+2>>2]: NonblockingTorusTileSW_PopulatedNorth {
            parameters:
                nodeId = (numOfNodesX - 1 - (index*2+(numOfNodesX%4==0?1:0)))*numOfNodesX+numOfNodesX - 1 - (index*2+(numOfNodesX%4==0?1:0));
                dispValueX = (numOfNodesX%4==0 ? ((numOfNodesX-1)*80-index*160)-80 : (numOfNodesX-1)*80-index*160);
                dispValueY = (numOfNodesX%4==0 ? (numOfNodesX-1)*80-index*160-80 : (numOfNodesX-1)*80-index*160);
				@display("p=120,120,e,$dispValueX,$dispValueY;b=50,50,,grey,,1");
				  numIF = 1;
				    numX = numOfNodesX;

      
          
        }
        tileNE_South[numOfNodesX+2>>2]: NonblockingTorusTileNE_PopulatedSouth {
            parameters:
                nodeId = (index*2+(numOfNodesX%4==0?1:0))*numOfNodesX+index*2+(numOfNodesX%4==0?1:0);
                dispValueX = (numOfNodesX%4==0 ? 80+index*160 : index*160);
                dispValueY = (numOfNodesX%4==0 ? 80+index*160 : index*160);
                @display("p=120,120,e,$dispValueX,$dispValueY;b=50,50,,grey,,1");
                  numIF = 1;
                    numX = numOfNodesX;
                  
                        
          
        }
        tileNW_South[numOfNodesX>>2]: NonblockingTorusTileNW_PopulatedSouth {
            parameters:
                nodeId = (index*2+(numOfNodesX%4==0?0:1))*numOfNodesX+numOfNodesX-1-(index*2+(numOfNodesX%4==0?0:1));
                dispValueX = (numOfNodesX%4==0 ? (numOfNodesX-1)*80-index*160 : ((numOfNodesX-1)*80-index*160)-80);
                dispValueY = (numOfNodesX%4==0 ? index*160 : index*160+80);
                @display("p=120,120,e,$dispValueX,$dispValueY;b=50,50,,grey,,1");
                  numIF = 1;
                    numX = numOfNodesX;
                        
           
        }
        tileSE_South[numOfNodesX+2>>2]: NonblockingTorusTileSE_PopulatedSouth {
            parameters:
                nodeId = (index*2+(numOfNodesX%4==0?1:0))*numOfNodesX+numOfNodesX-1-(index*2+(numOfNodesX%4==0?1:0));
                dispValueX = (numOfNodesX%4==0 ? ((numOfNodesX-1)*80-index*160)-80 : (numOfNodesX-1)*80-index*160);
                dispValueY = (numOfNodesX%4==0 ? index*160+80 : index*160);
                @display("p=120,120,e,$dispValueX,$dispValueY;b=50,50,,grey,,1");
                  numIF = 1;
                    numX = numOfNodesX;
                        
            
        }
        tileSW_South[numOfNodesX>>2]: NonblockingTorusTileSW_PopulatedSouth {
            parameters:
                nodeId = (index*2+(numOfNodesX%4==0?0:1))*numOfNodesX+index*2+(numOfNodesX%4==0?0:1);
                dispValueX = (numOfNodesX%4==0 ? index*160 : 80+index*160);
                dispValueY = (numOfNodesX%4==0 ? index*160 : 80+index*160);
                @display("p=120,120,e,$dispValueX,$dispValueY;b=50,50,,grey,,1");
                  numIF = 1;
                    numX = numOfNodesX;
            

        }
        uturnNS[numOfNodesX*2]: UTurnBidirectional {
            parameters:
                TurnDistance = ((25+25)+120)+(coreSizeY-620);
                disp_Width = (numOfNodesX+1)*80;
                @display("p=120,40,m,$numOfNodesX,80,$disp_Width;b=50,15,,,,1");
        }
        uturnEW[numOfNodesX*2]: UTurnBidirectional {
            parameters:
                TurnDistance = ((25+25)+258)+(coreSizeY-1034);
                disp_Width2 = (numOfNodesX+1)*80;
                @display("p=40,120,m,2,$disp_Width2,80;b=15,50,,,,1");


        }
    connections allowunconnected:

	//
	// GW Connections
	//

        for i=0..(numOfNodesX>>2)-1, for j = i*4+1..i*4+1, 
        for k=((numOfNodesX*2-2)-i*4)..((numOfNodesX*2-2)-i*4), 
        for l = ((numOfNodesX*2-1)-i*4)..((numOfNodesX*2-1)-i*4)
        {
            electronicGatewayIn[i*4+(numOfNodesX%4==0 ? 0 : 2)] <--> tileSW_South[i].electronicGatewayIn;
            electronicGatewayOut[i*4+(numOfNodesX%4==0 ? 0 : 2)]  <--> tileSW_South[i].electronicGatewayOut;
            photonicMemoryIn[i*4+(numOfNodesX%4==0 ? 0 : 2)] <-->  tileSW_South[i].photonicMemoryIn;
            photonicMemoryOut[i*4+(numOfNodesX%4==0 ? 0 : 2)] <--> tileSW_South[i].photonicMemoryOut;
            
            electronicControlInProc[i*4+(numOfNodesX%4==0 ? 0 : 2)] --> ElectronicChannel{power = false;@display("m=w;ls=red,0");} --> tileSW_South[i].electronicControlInProc;
            electronicControlOutProc[i*4+(numOfNodesX%4==0 ? 0 : 2)] <-- ElectronicChannel{power = false;@display("m=w;ls=red,0");} <-- tileSW_South[i].electronicControlOutProc;
            electronicControlInMemory[i*4+(numOfNodesX%4==0 ? 0 : 2)] --> tileSW_South[i].electronicControlInMemory;
            electronicControlOutMemory[i*4+(numOfNodesX%4==0 ? 0 : 2)] <-- tileSW_South[i].electronicControlOutMemory;
            

            electronicGatewayIn[j+(numOfNodesX%4==0 ? 0 : 2)] <--> tileNW_South[i].electronicGatewayIn;
            electronicGatewayOut[j+(numOfNodesX%4==0 ? 0 : 2)] <--> tileNW_South[i].electronicGatewayOut;
            photonicMemoryIn[j+(numOfNodesX%4==0 ? 0 : 2)] <--> tileNW_South[i].photonicMemoryIn;
            photonicMemoryOut[j+(numOfNodesX%4==0 ? 0 : 2)] <--> tileNW_South[i].photonicMemoryOut;
            
            electronicControlInProc[j+(numOfNodesX%4==0 ? 0 : 2)] --> ElectronicChannel{power = false;@display("m=w;ls=red,0");} --> tileNW_South[i].electronicControlInProc;
            electronicControlOutProc[j+(numOfNodesX%4==0 ? 0 : 2)] <-- ElectronicChannel{power = false;@display("m=w;ls=red,0");} <-- tileNW_South[i].electronicControlOutProc;
            electronicControlInMemory[j+(numOfNodesX%4==0 ? 0 : 2)] --> tileNW_South[i].electronicControlInMemory;
            electronicControlOutMemory[j+(numOfNodesX%4==0 ? 0 : 2)] <-- tileNW_South[i].electronicControlOutMemory;
            
           


            electronicGatewayIn[k-(numOfNodesX%4==0 ? 0 : 2)] <--> tileSE_North[i].electronicGatewayIn;
            electronicGatewayOut[k-(numOfNodesX%4==0 ? 0 : 2)] <--> tileSE_North[i].electronicGatewayOut;
            photonicMemoryIn[k-(numOfNodesX%4==0 ? 0 : 2)] <--> tileSE_North[i].photonicMemoryIn;
            photonicMemoryOut[k-(numOfNodesX%4==0 ? 0 : 2)] <--> tileSE_North[i].photonicMemoryOut;
            
            electronicControlInProc[k-(numOfNodesX%4==0 ? 0 : 2)] --> ElectronicChannel{power = false;@display("m=w;ls=red,0");} --> tileSE_North[i].electronicControlInProc;
            electronicControlOutProc[k-(numOfNodesX%4==0 ? 0 : 2)] <-- ElectronicChannel{power = false;@display("m=w;ls=red,0");} <-- tileSE_North[i].electronicControlOutProc;
            electronicControlInMemory[k-(numOfNodesX%4==0 ? 0 : 2)] --> tileSE_North[i].electronicControlInMemory;
            electronicControlOutMemory[k-(numOfNodesX%4==0 ? 0 : 2)] <-- tileSE_North[i].electronicControlOutMemory;
            
          


            electronicGatewayIn[l-(numOfNodesX%4==0 ? 0 : 2)] <--> tileNE_North[i].electronicGatewayIn;
            electronicGatewayOut[l-(numOfNodesX%4==0 ? 0 : 2)] <--> tileNE_North[i].electronicGatewayOut;
            photonicMemoryIn[l-(numOfNodesX%4==0 ? 0 : 2)] <--> tileNE_North[i].photonicMemoryIn;
            photonicMemoryOut[l-(numOfNodesX%4==0 ? 0 : 2)] <--> tileNE_North[i].photonicMemoryOut;
            
            electronicControlInProc[l-(numOfNodesX%4==0 ? 0 : 2)] --> ElectronicChannel{power = false;@display("m=w;ls=red,0");} --> tileNE_North[i].electronicControlInProc;
            electronicControlOutProc[l-(numOfNodesX%4==0 ? 0 : 2)] <-- ElectronicChannel{power = false;@display("m=w;ls=red,0");} <-- tileNE_North[i].electronicControlOutProc;
            electronicControlInMemory[l-(numOfNodesX%4==0 ? 0 : 2)] --> tileNE_North[i].electronicControlInMemory;
            electronicControlOutMemory[l-(numOfNodesX%4==0 ? 0 : 2)] <-- tileNE_North[i].electronicControlOutMemory;
            
          
        }



        for i = 0..(numOfNodesX+2>>2)-1,
        for j=(numOfNodesX%4==0 ? 2 : 0)..(numOfNodesX%4==0 ? 2 : 0)
        {
            electronicGatewayIn[i*4+j] <--> tileNE_South[i].electronicGatewayIn;
            electronicGatewayOut[i*4+j] <--> tileNE_South[i].electronicGatewayOut;
            photonicMemoryIn[i*4+j] <--> tileNE_South[i].photonicMemoryIn;
            photonicMemoryOut[i*4+j] <--> tileNE_South[i].photonicMemoryOut;
            
            electronicControlInProc[i*4+j] --> ElectronicChannel{power = false;@display("m=w;ls=red,0");} --> tileNE_South[i].electronicControlInProc;
            electronicControlOutProc[i*4+j] <-- ElectronicChannel{power = false;@display("m=w;ls=red,0");} <-- tileNE_South[i].electronicControlOutProc;
            electronicControlInMemory[i*4+j] --> tileNE_South[i].electronicControlInMemory;
            electronicControlOutMemory[i*4+j] <-- tileNE_South[i].electronicControlOutMemory;
            
          


            electronicGatewayIn[(i*4+1)+j] <--> tileSE_South[i].electronicGatewayIn;
            electronicGatewayOut[(i*4+1)+j] <--> tileSE_South[i].electronicGatewayOut;
            photonicMemoryIn[(i*4+1)+j] <--> tileSE_South[i].photonicMemoryIn;
            photonicMemoryOut[(i*4+1)+j] <--> tileSE_South[i].photonicMemoryOut;
            
            electronicControlInProc[(i*4+1)+j] --> ElectronicChannel{power = false;@display("m=w;ls=red,0");} --> tileSE_South[i].electronicControlInProc;
            electronicControlOutProc[(i*4+1)+j] <-- ElectronicChannel{power = false;@display("m=w;ls=red,0");} <-- tileSE_South[i].electronicControlOutProc;
            electronicControlInMemory[(i*4+1)+j] --> tileSE_South[i].electronicControlInMemory;
            electronicControlOutMemory[(i*4+1)+j] <-- tileSE_South[i].electronicControlOutMemory;
            
          


            electronicGatewayIn[((numOfNodesX*2-2)-i*4)-j] <--> tileNW_North[i].electronicGatewayIn;
            electronicGatewayOut[((numOfNodesX*2-2)-i*4)-j] <--> tileNW_North[i].electronicGatewayOut;
            photonicMemoryIn[((numOfNodesX*2-2)-i*4)-j] <--> tileNW_North[i].photonicMemoryIn;
            photonicMemoryOut[((numOfNodesX*2-2)-i*4)-j] <--> tileNW_North[i].photonicMemoryOut;
            
            electronicControlInProc[((numOfNodesX*2-2)-i*4)-j] --> ElectronicChannel{power = false;@display("m=w;ls=red,0");} --> tileNW_North[i].electronicControlInProc;
            electronicControlOutProc[((numOfNodesX*2-2)-i*4)-j] <-- ElectronicChannel{power = false;@display("m=w;ls=red,0");} <-- tileNW_North[i].electronicControlOutProc;
            electronicControlInMemory[((numOfNodesX*2-2)-i*4)-j] --> tileNW_North[i].electronicControlInMemory;
            electronicControlOutMemory[((numOfNodesX*2-2)-i*4)-j] <-- tileNW_North[i].electronicControlOutMemory;
            
            
          


            electronicGatewayIn[((numOfNodesX*2-1)-i*4)-j] <--> tileSW_North[i].electronicGatewayIn;
            electronicGatewayOut[((numOfNodesX*2-1)-i*4)-j] <--> tileSW_North[i].electronicGatewayOut;
            photonicMemoryIn[((numOfNodesX*2-1)-i*4)-j] <--> tileSW_North[i].photonicMemoryIn;
            photonicMemoryOut[((numOfNodesX*2-1)-i*4)-j] <--> tileSW_North[i].photonicMemoryOut;
            
            electronicControlInProc[((numOfNodesX*2-1)-i*4)-j] --> ElectronicChannel{power = false;@display("m=w;ls=red,0");} --> tileSW_North[i].electronicControlInProc;
            electronicControlOutProc[((numOfNodesX*2-1)-i*4)-j] <-- ElectronicChannel{power = false;@display("m=w;ls=red,0");} <-- tileSW_North[i].electronicControlOutProc;
            electronicControlInMemory[((numOfNodesX*2-1)-i*4)-j] --> tileSW_North[i].electronicControlInMemory;
            electronicControlOutMemory[((numOfNodesX*2-1)-i*4)-j] <-- tileSW_North[i].electronicControlOutMemory;
            
           
        }
       

	//
	//Tile Connections
	//
	// Special Cases


        // North-West Node
        uturnNS[0].photonicInnerBendOut <--> tileSW_South[0].photonicInN0 if numOfNodesX%4==0;
        uturnNS[0].photonicOuterBendOut <--> tileSW_South[0].photonicInN1 if numOfNodesX%4==0;

        uturnNS[0].photonicInnerBendIn <--> tileSW_South[0].photonicOutN1 if numOfNodesX%4==0;
        uturnNS[0].photonicOuterBendIn <--> tileSW_South[0].photonicOutN0 if numOfNodesX%4==0;

		tileSW_South[0].electronicOutN0 --> tileSW_South[0].electronicInN1 if numOfNodesX%4==0;
		tileSW_South[0].electronicOutN1 --> tileSW_South[0].electronicInN0 if numOfNodesX%4==0;
		
		

		////

        uturnNS[0].photonicInnerBendOut <--> tileNE_South[0].photonicInN0 if numOfNodesX%4!=0;
        uturnNS[0].photonicOuterBendOut <--> tileNE_South[0].photonicInN1 if numOfNodesX%4!=0;

        uturnNS[0].photonicInnerBendIn <--> tileNE_South[0].photonicOutN1 if numOfNodesX%4!=0;
        uturnNS[0].photonicOuterBendIn <--> tileNE_South[0].photonicOutN0 if numOfNodesX%4!=0;

		tileNE_South[0].electronicOutN0 --> tileNE_South[0].electronicInN1 if numOfNodesX%4!=0;
		tileNE_South[0].electronicOutN1 --> tileNE_South[0].electronicInN0 if numOfNodesX%4!=0;
		
		

		////

        uturnEW[0].photonicInnerBendOut <--> tileSW_South[0].photonicInW0 if numOfNodesX%4==0;
        uturnEW[0].photonicOuterBendOut <--> tileSW_South[0].photonicInW1 if numOfNodesX%4==0;

        uturnEW[0].photonicInnerBendIn <--> tileSW_South[0].photonicOutW1 if numOfNodesX%4==0;
        uturnEW[0].photonicOuterBendIn <--> tileSW_South[0].photonicOutW0 if numOfNodesX%4==0;
		
		tileSW_South[0].electronicOutW0 --> tileSW_South[0].electronicInW1 if numOfNodesX%4==0;
		tileSW_South[0].electronicOutW1 --> tileSW_South[0].electronicInW0 if numOfNodesX%4==0;
		
		
		
		////
        
        uturnEW[0].photonicInnerBendOut <--> tileNE_South[0].photonicInW0 if numOfNodesX%4!=0;
        uturnEW[0].photonicOuterBendOut <--> tileNE_South[0].photonicInW1 if numOfNodesX%4!=0;

        uturnEW[0].photonicInnerBendIn <--> tileNE_South[0].photonicOutW1 if numOfNodesX%4!=0;
        uturnEW[0].photonicOuterBendIn <--> tileNE_South[0].photonicOutW0 if numOfNodesX%4!=0;
        
		tileNE_South[0].electronicOutW0 --> tileNE_South[0].electronicInW1 if numOfNodesX%4!=0;
		tileNE_South[0].electronicOutW1 --> tileNE_South[0].electronicInW0 if numOfNodesX%4!=0;
		
		
        // North-East Node
        uturnNS[numOfNodesX-1].photonicInnerBendOut <--> tileNW_South[0].photonicInN0 if numOfNodesX%4==0;
        uturnNS[numOfNodesX-1].photonicOuterBendOut <--> tileNW_South[0].photonicInN1 if numOfNodesX%4==0;

        uturnNS[numOfNodesX-1].photonicInnerBendIn <--> tileNW_South[0].photonicOutN1 if numOfNodesX%4==0;
        uturnNS[numOfNodesX-1].photonicOuterBendIn <--> tileNW_South[0].photonicOutN0 if numOfNodesX%4==0;

		tileNW_South[0].electronicOutN0 --> tileNW_South[0].electronicInN1 if numOfNodesX%4==0;
		tileNW_South[0].electronicOutN1 --> tileNW_South[0].electronicInN0 if numOfNodesX%4==0;
		
		

		////

        uturnNS[numOfNodesX-1].photonicInnerBendOut <--> tileSE_South[0].photonicInN0 if numOfNodesX%4!=0;
        uturnNS[numOfNodesX-1].photonicOuterBendOut <--> tileSE_South[0].photonicInN1 if numOfNodesX%4!=0;

        uturnNS[numOfNodesX-1].photonicInnerBendIn <--> tileSE_South[0].photonicOutN1 if numOfNodesX%4!=0;
        uturnNS[numOfNodesX-1].photonicOuterBendIn <--> tileSE_South[0].photonicOutN0 if numOfNodesX%4!=0;
		
		tileSE_South[0].electronicOutN0 --> tileSE_South[0].electronicInN1 if numOfNodesX%4!=0;
		tileSE_South[0].electronicOutN1 --> tileSE_South[0].electronicInN0 if numOfNodesX%4!=0;
		
	
		
		////
		
        uturnEW[1].photonicInnerBendOut <--> tileNW_South[0].photonicInE1 if numOfNodesX%4==0;
        uturnEW[1].photonicOuterBendOut <--> tileNW_South[0].photonicInE0 if numOfNodesX%4==0;

        uturnEW[1].photonicInnerBendIn <--> tileNW_South[0].photonicOutE0 if numOfNodesX%4==0;
        uturnEW[1].photonicOuterBendIn <--> tileNW_South[0].photonicOutE1 if numOfNodesX%4==0;
		
		tileNW_South[0].electronicOutE0 --> tileNW_South[0].electronicInE1 if numOfNodesX%4==0;
		tileNW_South[0].electronicOutE1 --> tileNW_South[0].electronicInE0 if numOfNodesX%4==0;
		
		
		
		////
		
        uturnEW[1].photonicInnerBendOut <--> tileSE_South[0].photonicInE1 if numOfNodesX%4!=0;
        uturnEW[1].photonicOuterBendOut <--> tileSE_South[0].photonicInE0 if numOfNodesX%4!=0;

        uturnEW[1].photonicInnerBendIn <--> tileSE_South[0].photonicOutE0 if numOfNodesX%4!=0;
        uturnEW[1].photonicOuterBendIn <--> tileSE_South[0].photonicOutE1 if numOfNodesX%4!=0;

		tileSE_South[0].electronicOutE0 --> tileSE_South[0].electronicInE1 if numOfNodesX%4!=0;
		tileSE_South[0].electronicOutE1 --> tileSE_South[0].electronicInE0 if numOfNodesX%4!=0;
		
		

        // South-West Node
        uturnNS[numOfNodesX].photonicInnerBendOut <--> tileSE_North[0].photonicInS1 if numOfNodesX%4==0;
        uturnNS[numOfNodesX].photonicOuterBendOut <--> tileSE_North[0].photonicInS0 if numOfNodesX%4==0;

        uturnNS[numOfNodesX].photonicInnerBendIn <--> tileSE_North[0].photonicOutS0 if numOfNodesX%4==0;
        uturnNS[numOfNodesX].photonicOuterBendIn <--> tileSE_North[0].photonicOutS1 if numOfNodesX%4==0;
        
        tileSE_North[0].electronicOutS0 --> tileSE_North[0].electronicInS1 if numOfNodesX%4==0;
		tileSE_North[0].electronicOutS1 --> tileSE_North[0].electronicInS0 if numOfNodesX%4==0;
		
		
		
		////
		
        uturnNS[numOfNodesX].photonicInnerBendOut <--> tileNW_North[0].photonicInS1 if numOfNodesX%4!=0;
        uturnNS[numOfNodesX].photonicOuterBendOut <--> tileNW_North[0].photonicInS0 if numOfNodesX%4!=0;

        uturnNS[numOfNodesX].photonicInnerBendIn <--> tileNW_North[0].photonicOutS0 if numOfNodesX%4!=0;
        uturnNS[numOfNodesX].photonicOuterBendIn <--> tileNW_North[0].photonicOutS1 if numOfNodesX%4!=0;
        
        tileNW_North[0].electronicOutS0 --> tileNW_North[0].electronicInS1 if numOfNodesX%4!=0;
		tileNW_North[0].electronicOutS1 --> tileNW_North[0].electronicInS0 if numOfNodesX%4!=0;
		
		
		
		////

        uturnEW[numOfNodesX*2-2].photonicInnerBendOut <--> tileSE_North[0].photonicInW0 if numOfNodesX%4==0;
        uturnEW[numOfNodesX*2-2].photonicOuterBendOut <--> tileSE_North[0].photonicInW1 if numOfNodesX%4==0;

        uturnEW[numOfNodesX*2-2].photonicInnerBendIn <--> tileSE_North[0].photonicOutW1 if numOfNodesX%4==0;
        uturnEW[numOfNodesX*2-2].photonicOuterBendIn <--> tileSE_North[0].photonicOutW0 if numOfNodesX%4==0;
        
        tileSE_North[0].electronicOutW0 --> tileSE_North[0].electronicInW1 if numOfNodesX%4==0;
		tileSE_North[0].electronicOutW1 --> tileSE_North[0].electronicInW0 if numOfNodesX%4==0;
		
		
		
		////
		
        uturnEW[numOfNodesX*2-2].photonicInnerBendOut <--> tileNW_North[0].photonicInW0 if numOfNodesX%4!=0;
        uturnEW[numOfNodesX*2-2].photonicOuterBendOut <--> tileNW_North[0].photonicInW1 if numOfNodesX%4!=0;

        uturnEW[numOfNodesX*2-2].photonicInnerBendIn <--> tileNW_North[0].photonicOutW1 if numOfNodesX%4!=0;
        uturnEW[numOfNodesX*2-2].photonicOuterBendIn <--> tileNW_North[0].photonicOutW0 if numOfNodesX%4!=0;

		tileNW_North[0].electronicOutW0 --> tileNW_North[0].electronicInW1 if numOfNodesX%4!=0;
		tileNW_North[0].electronicOutW1 --> tileNW_North[0].electronicInW0 if numOfNodesX%4!=0;
		
		

        // South-East Node
        uturnNS[numOfNodesX*2-1].photonicInnerBendOut <--> tileNE_North[0].photonicInS1 if numOfNodesX%4==0;
        uturnNS[numOfNodesX*2-1].photonicOuterBendOut <--> tileNE_North[0].photonicInS0 if numOfNodesX%4==0;

        uturnNS[numOfNodesX*2-1].photonicInnerBendIn <--> tileNE_North[0].photonicOutS0 if numOfNodesX%4==0;
        uturnNS[numOfNodesX*2-1].photonicOuterBendIn <--> tileNE_North[0].photonicOutS1 if numOfNodesX%4==0;
        
        tileNE_North[0].electronicOutS0 --> tileNE_North[0].electronicInS1 if numOfNodesX%4==0;
		tileNE_North[0].electronicOutS1 --> tileNE_North[0].electronicInS0 if numOfNodesX%4==0;
		
		
		
		////
		
        uturnNS[numOfNodesX*2-1].photonicInnerBendOut <--> tileSW_North[0].photonicInS1 if numOfNodesX%4!=0;
        uturnNS[numOfNodesX*2-1].photonicOuterBendOut <--> tileSW_North[0].photonicInS0 if numOfNodesX%4!=0;

        uturnNS[numOfNodesX*2-1].photonicInnerBendIn <--> tileSW_North[0].photonicOutS0 if numOfNodesX%4!=0;
        uturnNS[numOfNodesX*2-1].photonicOuterBendIn <--> tileSW_North[0].photonicOutS1 if numOfNodesX%4!=0;
        
        tileSW_North[0].electronicOutS0 --> tileSW_North[0].electronicInS1 if numOfNodesX%4!=0;
		tileSW_North[0].electronicOutS1 --> tileSW_North[0].electronicInS0 if numOfNodesX%4!=0;
		
		

		///
		
        uturnEW[numOfNodesX*2-1].photonicInnerBendOut <--> tileNE_North[0].photonicInE1 if numOfNodesX%4==0;
        uturnEW[numOfNodesX*2-1].photonicOuterBendOut <--> tileNE_North[0].photonicInE0 if numOfNodesX%4==0;

        uturnEW[numOfNodesX*2-1].photonicInnerBendIn <--> tileNE_North[0].photonicOutE0 if numOfNodesX%4==0;
        uturnEW[numOfNodesX*2-1].photonicOuterBendIn <--> tileNE_North[0].photonicOutE1 if numOfNodesX%4==0;
        
        tileNE_North[0].electronicOutE0 --> tileNE_North[0].electronicInE1 if numOfNodesX%4==0;
		tileNE_North[0].electronicOutE1 --> tileNE_North[0].electronicInE0 if numOfNodesX%4==0;
		
		
		
		////
		
        uturnEW[numOfNodesX*2-1].photonicInnerBendOut <--> tileSW_North[0].photonicInE1 if numOfNodesX%4!=0;
        uturnEW[numOfNodesX*2-1].photonicOuterBendOut <--> tileSW_North[0].photonicInE0 if numOfNodesX%4!=0;

        uturnEW[numOfNodesX*2-1].photonicInnerBendIn <--> tileSW_North[0].photonicOutE0 if numOfNodesX%4!=0;
        uturnEW[numOfNodesX*2-1].photonicOuterBendIn <--> tileSW_North[0].photonicOutE1 if numOfNodesX%4!=0;
        
        tileSW_North[0].electronicOutE0 --> tileSW_North[0].electronicInE1 if numOfNodesX%4!=0;
		tileSW_North[0].electronicOutE1 --> tileSW_North[0].electronicInE0 if numOfNodesX%4!=0;
		
		

	// Center 2x2 connected together

        tileNE_South[(numOfNodesX+2>>2)-1].photonicOutE0 <--> tileSE_South[(numOfNodesX+2>>2)-1].photonicInW0;
        tileNE_South[(numOfNodesX+2>>2)-1].photonicOutE1 <--> tileSE_South[(numOfNodesX+2>>2)-1].photonicInW1;
        tileNE_South[(numOfNodesX+2>>2)-1].photonicInE0 <--> tileSE_South[(numOfNodesX+2>>2)-1].photonicOutW0;
        tileNE_South[(numOfNodesX+2>>2)-1].photonicInE1 <--> tileSE_South[(numOfNodesX+2>>2)-1].photonicOutW1;

        tileNE_South[(numOfNodesX+2>>2)-1].photonicOutS0 <--> tileNW_North[(numOfNodesX+2>>2)-1].photonicInN0;
        tileNE_South[(numOfNodesX+2>>2)-1].photonicOutS1 <--> tileNW_North[(numOfNodesX+2>>2)-1].photonicInN1;
        tileNE_South[(numOfNodesX+2>>2)-1].photonicInS0 <--> tileNW_North[(numOfNodesX+2>>2)-1].photonicOutN0;
        tileNE_South[(numOfNodesX+2>>2)-1].photonicInS1 <--> tileNW_North[(numOfNodesX+2>>2)-1].photonicOutN1;

        tileSW_North[(numOfNodesX+2>>2)-1].photonicOutW0 <--> tileNW_North[(numOfNodesX+2>>2)-1].photonicInE0;
        tileSW_North[(numOfNodesX+2>>2)-1].photonicOutW1 <--> tileNW_North[(numOfNodesX+2>>2)-1].photonicInE1;
        tileSW_North[(numOfNodesX+2>>2)-1].photonicInW0 <--> tileNW_North[(numOfNodesX+2>>2)-1].photonicOutE0;
        tileSW_North[(numOfNodesX+2>>2)-1].photonicInW1 <--> tileNW_North[(numOfNodesX+2>>2)-1].photonicOutE1;

        tileSW_North[(numOfNodesX+2>>2)-1].photonicOutN0 <--> tileSE_South[(numOfNodesX+2>>2)-1].photonicInS0;
        tileSW_North[(numOfNodesX+2>>2)-1].photonicOutN1 <--> tileSE_South[(numOfNodesX+2>>2)-1].photonicInS1;
        tileSW_North[(numOfNodesX+2>>2)-1].photonicInN0 <--> tileSE_South[(numOfNodesX+2>>2)-1].photonicOutS0;
        tileSW_North[(numOfNodesX+2>>2)-1].photonicInN1 <--> tileSE_South[(numOfNodesX+2>>2)-1].photonicOutS1;

		tileNE_South[(numOfNodesX+2>>2)-1].electronicOutE0 --> tileSE_South[(numOfNodesX+2>>2)-1].electronicInW0;
        tileNE_South[(numOfNodesX+2>>2)-1].electronicOutE1 --> tileSE_South[(numOfNodesX+2>>2)-1].electronicInW1;
        tileNE_South[(numOfNodesX+2>>2)-1].electronicInE0 <-- tileSE_South[(numOfNodesX+2>>2)-1].electronicOutW0;
        tileNE_South[(numOfNodesX+2>>2)-1].electronicInE1 <-- tileSE_South[(numOfNodesX+2>>2)-1].electronicOutW1;

        tileNE_South[(numOfNodesX+2>>2)-1].electronicOutS0 --> tileNW_North[(numOfNodesX+2>>2)-1].electronicInN0;
        tileNE_South[(numOfNodesX+2>>2)-1].electronicOutS1 --> tileNW_North[(numOfNodesX+2>>2)-1].electronicInN1;
        tileNE_South[(numOfNodesX+2>>2)-1].electronicInS0 <-- tileNW_North[(numOfNodesX+2>>2)-1].electronicOutN0;
        tileNE_South[(numOfNodesX+2>>2)-1].electronicInS1 <-- tileNW_North[(numOfNodesX+2>>2)-1].electronicOutN1;

        tileSW_North[(numOfNodesX+2>>2)-1].electronicOutW0 --> tileNW_North[(numOfNodesX+2>>2)-1].electronicInE0;
        tileSW_North[(numOfNodesX+2>>2)-1].electronicOutW1 --> tileNW_North[(numOfNodesX+2>>2)-1].electronicInE1;
        tileSW_North[(numOfNodesX+2>>2)-1].electronicInW0 <-- tileNW_North[(numOfNodesX+2>>2)-1].electronicOutE0;
        tileSW_North[(numOfNodesX+2>>2)-1].electronicInW1 <-- tileNW_North[(numOfNodesX+2>>2)-1].electronicOutE1;

        tileSW_North[(numOfNodesX+2>>2)-1].electronicOutN0 --> tileSE_South[(numOfNodesX+2>>2)-1].electronicInS0;
        tileSW_North[(numOfNodesX+2>>2)-1].electronicOutN1 --> tileSE_South[(numOfNodesX+2>>2)-1].electronicInS1;
        tileSW_North[(numOfNodesX+2>>2)-1].electronicInN0 <-- tileSE_South[(numOfNodesX+2>>2)-1].electronicOutS0;
        tileSW_North[(numOfNodesX+2>>2)-1].electronicInN1 <-- tileSE_South[(numOfNodesX+2>>2)-1].electronicOutS1;
        
     
        
		

	// Setup U-turns for rest of perimeter

        for i=1..numOfNodesX-2,
        for j = (numOfNodesX>>1)..(numOfNodesX>>1)
        {
            
            //
            // North Side
            //
            
            uturnNS[i].photonicInnerBendOut <--> tileNW_Empty[i>>1].photonicInN0 if numOfNodesX%4==0&&i%2!=0;
            uturnNS[i].photonicOuterBendOut <--> tileNW_Empty[i>>1].photonicInN1 if numOfNodesX%4==0&&i%2!=0;
            uturnNS[i].photonicInnerBendIn <--> tileNW_Empty[i>>1].photonicOutN1 if numOfNodesX%4==0&&i%2!=0;
            uturnNS[i].photonicOuterBendIn <--> tileNW_Empty[i>>1].photonicOutN0 if numOfNodesX%4==0&&i%2!=0;

			tileNW_Empty[i>>1].electronicOutN0 --> tileNW_Empty[i>>1].electronicInN1 if numOfNodesX%4==0&&i%2!=0;
			tileNW_Empty[i>>1].electronicOutN1 --> tileNW_Empty[i>>1].electronicInN0 if numOfNodesX%4==0&&i%2!=0;
			
			

			////

            uturnNS[i].photonicInnerBendOut <--> tileSW_Empty[i+1>>1].photonicInN0 if numOfNodesX%4==0&&i%2==0;
            uturnNS[i].photonicOuterBendOut <--> tileSW_Empty[i+1>>1].photonicInN1 if numOfNodesX%4==0&&i%2==0;
            uturnNS[i].photonicInnerBendIn <--> tileSW_Empty[i+1>>1].photonicOutN1 if numOfNodesX%4==0&&i%2==0;
            uturnNS[i].photonicOuterBendIn <--> tileSW_Empty[i+1>>1].photonicOutN0 if numOfNodesX%4==0&&i%2==0;

			tileSW_Empty[i+1>>1].electronicOutN0 --> tileSW_Empty[i+1>>1].electronicInN1 if numOfNodesX%4==0&&i%2==0;
			tileSW_Empty[i+1>>1].electronicOutN1 --> tileSW_Empty[i+1>>1].electronicInN0 if numOfNodesX%4==0&&i%2==0;
			
			

			////

            uturnNS[i].photonicInnerBendOut <--> tileSE_Empty[i>>1].photonicInN0 if numOfNodesX%4!=0&&i%2!=0;
            uturnNS[i].photonicOuterBendOut <--> tileSE_Empty[i>>1].photonicInN1 if numOfNodesX%4!=0&&i%2!=0;
            uturnNS[i].photonicInnerBendIn <--> tileSE_Empty[i>>1].photonicOutN1 if numOfNodesX%4!=0&&i%2!=0;
            uturnNS[i].photonicOuterBendIn <--> tileSE_Empty[i>>1].photonicOutN0 if numOfNodesX%4!=0&&i%2!=0;

			tileSE_Empty[i>>1].electronicOutN0 --> tileSE_Empty[i>>1].electronicInN1 if numOfNodesX%4!=0&&i%2!=0;
			tileSE_Empty[i>>1].electronicOutN1 --> tileSE_Empty[i>>1].electronicInN0 if numOfNodesX%4!=0&&i%2!=0;
			
		

			////
			
            uturnNS[i].photonicInnerBendOut <--> tileNE_Empty[i+1>>1].photonicInN0 if numOfNodesX%4!=0&&i%2==0;
            uturnNS[i].photonicOuterBendOut <--> tileNE_Empty[i+1>>1].photonicInN1 if numOfNodesX%4!=0&&i%2==0;
            uturnNS[i].photonicInnerBendIn <--> tileNE_Empty[i+1>>1].photonicOutN1 if numOfNodesX%4!=0&&i%2==0;
            uturnNS[i].photonicOuterBendIn <--> tileNE_Empty[i+1>>1].photonicOutN0 if numOfNodesX%4!=0&&i%2==0;

			tileNE_Empty[i>>1].electronicOutN0 --> tileNE_Empty[i+1>>1].electronicInN1 if numOfNodesX%4!=0&&i%2==0;
			tileNE_Empty[i>>1].electronicOutN1 --> tileNE_Empty[i+1>>1].electronicInN0 if numOfNodesX%4!=0&&i%2==0;
			
		

			//
            // South Side
            //

            uturnNS[i+numOfNodesX].photonicInnerBendOut <--> tileNE_Empty[j*(j-1)+(i>>1)].photonicInS1 if numOfNodesX%4==0&&i%2!=0;
            uturnNS[i+numOfNodesX].photonicOuterBendOut <--> tileNE_Empty[j*(j-1)+(i>>1)].photonicInS0 if numOfNodesX%4==0&&i%2!=0;
            uturnNS[i+numOfNodesX].photonicInnerBendIn <--> tileNE_Empty[j*(j-1)+(i>>1)].photonicOutS0 if numOfNodesX%4==0&&i%2!=0;
            uturnNS[i+numOfNodesX].photonicOuterBendIn <--> tileNE_Empty[j*(j-1)+(i>>1)].photonicOutS1 if numOfNodesX%4==0&&i%2!=0;

			tileNE_Empty[j*(j-1)+(i>>1)].electronicOutS0 --> tileNE_Empty[j*(j-1)+(i>>1)].electronicInS1 if numOfNodesX%4==0&&i%2!=0;
			tileNE_Empty[j*(j-1)+(i>>1)].electronicOutS1 --> tileNE_Empty[j*(j-1)+(i>>1)].electronicInS0 if numOfNodesX%4==0&&i%2!=0;
			
		

			////
			
            uturnNS[i+numOfNodesX].photonicInnerBendOut <--> tileSE_Empty[j*(j-1)+(i+1>>1)].photonicInS1 if numOfNodesX%4==0&&i%2==0;
            uturnNS[i+numOfNodesX].photonicOuterBendOut <--> tileSE_Empty[j*(j-1)+(i+1>>1)].photonicInS0 if numOfNodesX%4==0&&i%2==0;
            uturnNS[i+numOfNodesX].photonicInnerBendIn <--> tileSE_Empty[j*(j-1)+(i+1>>1)].photonicOutS0 if numOfNodesX%4==0&&i%2==0;
            uturnNS[i+numOfNodesX].photonicOuterBendIn <--> tileSE_Empty[j*(j-1)+(i+1>>1)].photonicOutS1 if numOfNodesX%4==0&&i%2==0;

			tileSE_Empty[j*(j-1)+(i+1>>1)].electronicOutS0 --> tileSE_Empty[j*(j-1)+(i+1>>1)].electronicInS1 if numOfNodesX%4==0&&i%2!=0;
			tileSE_Empty[j*(j-1)+(i+1>>1)].electronicOutS1 --> tileSE_Empty[j*(j-1)+(i+1>>1)].electronicInS0 if numOfNodesX%4==0&&i%2!=0;
			
		

			////

            uturnNS[i+numOfNodesX].photonicInnerBendOut <--> tileSW_Empty[j*(j-1)+(i>>1)].photonicInS1 if numOfNodesX%4!=0&&i%2!=0;
            uturnNS[i+numOfNodesX].photonicOuterBendOut <--> tileSW_Empty[j*(j-1)+(i>>1)].photonicInS0 if numOfNodesX%4!=0&&i%2!=0;
            uturnNS[i+numOfNodesX].photonicInnerBendIn <--> tileSW_Empty[j*(j-1)+(i>>1)].photonicOutS0 if numOfNodesX%4!=0&&i%2!=0;
            uturnNS[i+numOfNodesX].photonicOuterBendIn <--> tileSW_Empty[j*(j-1)+(i>>1)].photonicOutS1 if numOfNodesX%4!=0&&i%2!=0;

			tileSW_Empty[j*(j-1)+(i>>1)].electronicOutS0 --> tileSW_Empty[j*(j-1)+(i>>1)].electronicInS1 if numOfNodesX%4!=0&&i%2!=0;
			tileSW_Empty[j*(j-1)+(i>>1)].electronicOutS1 --> tileSW_Empty[j*(j-1)+(i>>1)].electronicInS0 if numOfNodesX%4!=0&&i%2!=0;
			
			

			////
			
            uturnNS[i+numOfNodesX].photonicInnerBendOut <--> tileNW_Empty[j*(j-1)+(i+1>>1)].photonicInS1 if numOfNodesX%4!=0&&i%2==0;
            uturnNS[i+numOfNodesX].photonicOuterBendOut <--> tileNW_Empty[j*(j-1)+(i+1>>1)].photonicInS0 if numOfNodesX%4!=0&&i%2==0;
            uturnNS[i+numOfNodesX].photonicInnerBendIn <--> tileNW_Empty[j*(j-1)+(i+1>>1)].photonicOutS0 if numOfNodesX%4!=0&&i%2==0;
            uturnNS[i+numOfNodesX].photonicOuterBendIn <--> tileNW_Empty[j*(j-1)+(i+1>>1)].photonicOutS1 if numOfNodesX%4!=0&&i%2==0;

			tileNW_Empty[j*(j-1)+(i+1>>1)].electronicOutS0 --> tileNW_Empty[j*(j-1)+(i+1>>1)].electronicInS1 if numOfNodesX%4!=0&&i%2==0;
			tileNW_Empty[j*(j-1)+(i+1>>1)].electronicOutS1 --> tileNW_Empty[j*(j-1)+(i+1>>1)].electronicInS0 if numOfNodesX%4!=0&&i%2==0;
			
			

			//
            // West Side
            //

            uturnEW[i*2].photonicInnerBendOut <--> tileSE_Empty[(i>>1)*(numOfNodesX>>1)].photonicInW0 if numOfNodesX%4==0&&i%2!=0;
            uturnEW[i*2].photonicOuterBendOut <--> tileSE_Empty[(i>>1)*(numOfNodesX>>1)].photonicInW1 if numOfNodesX%4==0&&i%2!=0;
            uturnEW[i*2].photonicInnerBendIn <--> tileSE_Empty[(i>>1)*(numOfNodesX>>1)].photonicOutW1 if numOfNodesX%4==0&&i%2!=0;
            uturnEW[i*2].photonicOuterBendIn <--> tileSE_Empty[(i>>1)*(numOfNodesX>>1)].photonicOutW0 if numOfNodesX%4==0&&i%2!=0;

			tileSE_Empty[(i>>1)*(numOfNodesX>>1)].electronicOutW0 --> tileSE_Empty[(i>>1)*(numOfNodesX>>1)].electronicInW1 if numOfNodesX%4==0&&i%2!=0;
			tileSE_Empty[(i>>1)*(numOfNodesX>>1)].electronicOutW1 --> tileSE_Empty[(i>>1)*(numOfNodesX>>1)].electronicInW0 if numOfNodesX%4==0&&i%2!=0;
			
			

			////
			
            uturnEW[i*2].photonicInnerBendOut <--> tileSW_Empty[(i+1>>1)*(numOfNodesX>>1)].photonicInW0 if numOfNodesX%4==0&&i%2==0;
            uturnEW[i*2].photonicOuterBendOut <--> tileSW_Empty[(i+1>>1)*(numOfNodesX>>1)].photonicInW1 if numOfNodesX%4==0&&i%2==0;
            uturnEW[i*2].photonicInnerBendIn <--> tileSW_Empty[(i+1>>1)*(numOfNodesX>>1)].photonicOutW1 if numOfNodesX%4==0&&i%2==0;
            uturnEW[i*2].photonicOuterBendIn <--> tileSW_Empty[(i+1>>1)*(numOfNodesX>>1)].photonicOutW0 if numOfNodesX%4==0&&i%2==0;

			tileSW_Empty[(i+1>>1)*(numOfNodesX>>1)].electronicOutW0 --> tileSW_Empty[(i+1>>1)*(numOfNodesX>>1)].electronicInW1 if numOfNodesX%4==0&&i%2==0;
			tileSW_Empty[(i+1>>1)*(numOfNodesX>>1)].electronicOutW1 --> tileSW_Empty[(i+1>>1)*(numOfNodesX>>1)].electronicInW0 if numOfNodesX%4==0&&i%2==0;
			
			

			////

            uturnEW[i*2].photonicInnerBendOut <--> tileNW_Empty[(i>>1)*(numOfNodesX>>1)].photonicInW0 if numOfNodesX%4!=0&&i%2!=0;
            uturnEW[i*2].photonicOuterBendOut <--> tileNW_Empty[(i>>1)*(numOfNodesX>>1)].photonicInW1 if numOfNodesX%4!=0&&i%2!=0;
            uturnEW[i*2].photonicInnerBendIn <--> tileNW_Empty[(i>>1)*(numOfNodesX>>1)].photonicOutW1 if numOfNodesX%4!=0&&i%2!=0;
            uturnEW[i*2].photonicOuterBendIn <--> tileNW_Empty[(i>>1)*(numOfNodesX>>1)].photonicOutW0 if numOfNodesX%4!=0&&i%2!=0;

			tileNW_Empty[(i>>1)*(numOfNodesX>>1)].electronicOutW0 --> tileNW_Empty[(i>>1)*(numOfNodesX>>1)].electronicInW1 if numOfNodesX%4!=0&&i%2!=0;
			tileNW_Empty[(i>>1)*(numOfNodesX>>1)].electronicOutW1 --> tileNW_Empty[(i>>1)*(numOfNodesX>>1)].electronicInW0 if numOfNodesX%4!=0&&i%2!=0;
			
			

			////
			
            uturnEW[i*2].photonicInnerBendOut <--> tileNE_Empty[(i+1>>1)*(numOfNodesX>>1)].photonicInW0 if numOfNodesX%4!=0&&i%2==0;
            uturnEW[i*2].photonicOuterBendOut <--> tileNE_Empty[(i+1>>1)*(numOfNodesX>>1)].photonicInW1 if numOfNodesX%4!=0&&i%2==0;
            uturnEW[i*2].photonicInnerBendIn <--> tileNE_Empty[(i+1>>1)*(numOfNodesX>>1)].photonicOutW1 if numOfNodesX%4!=0&&i%2==0;
            uturnEW[i*2].photonicOuterBendIn <--> tileNE_Empty[(i+1>>1)*(numOfNodesX>>1)].photonicOutW0 if numOfNodesX%4!=0&&i%2==0;

			tileNE_Empty[(i+1>>1)*(numOfNodesX>>1)].electronicOutW0 --> tileNE_Empty[(i+1>>1)*(numOfNodesX>>1)].electronicInW1 if numOfNodesX%4!=0&&i%2==0;
			tileNE_Empty[(i+1>>1)*(numOfNodesX>>1)].electronicOutW1 --> tileNE_Empty[(i+1>>1)*(numOfNodesX>>1)].electronicInW0 if numOfNodesX%4!=0&&i%2==0;
			
		

			//
            // East Side
            //
            
            uturnEW[i*2+1].photonicInnerBendOut <--> tileNE_Empty[j*(i>>1)+(j-1)].photonicInE1 if numOfNodesX%4==0&&i%2!=0;
            uturnEW[i*2+1].photonicOuterBendOut <--> tileNE_Empty[j*(i>>1)+(j-1)].photonicInE0 if numOfNodesX%4==0&&i%2!=0;
            uturnEW[i*2+1].photonicInnerBendIn <--> tileNE_Empty[j*(i>>1)+(j-1)].photonicOutE0 if numOfNodesX%4==0&&i%2!=0;
            uturnEW[i*2+1].photonicOuterBendIn <--> tileNE_Empty[j*(i>>1)+(j-1)].photonicOutE1 if numOfNodesX%4==0&&i%2!=0;

			tileNE_Empty[j*(i>>1)+(j-1)].electronicOutE0 --> tileNE_Empty[j*(i>>1)+(j-1)].electronicInE1 if numOfNodesX%4==0&&i%2!=0;
			tileNE_Empty[j*(i>>1)+(j-1)].electronicOutE1 --> tileNE_Empty[j*(i>>1)+(j-1)].electronicInE0 if numOfNodesX%4==0&&i%2!=0;
			
			

			////
			
            uturnEW[i*2+1].photonicInnerBendOut <--> tileNW_Empty[j*(i+1>>1)+(j-1)].photonicInE1 if numOfNodesX%4==0&&i%2==0;
            uturnEW[i*2+1].photonicOuterBendOut <--> tileNW_Empty[j*(i+1>>1)+(j-1)].photonicInE0 if numOfNodesX%4==0&&i%2==0;
            uturnEW[i*2+1].photonicInnerBendIn <--> tileNW_Empty[j*(i+1>>1)+(j-1)].photonicOutE0 if numOfNodesX%4==0&&i%2==0;
            uturnEW[i*2+1].photonicOuterBendIn <--> tileNW_Empty[j*(i+1>>1)+(j-1)].photonicOutE1 if numOfNodesX%4==0&&i%2==0;

			tileNW_Empty[j*(i+1>>1)+(j-1)].electronicOutE0 --> tileNW_Empty[j*(i+1>>1)+(j-1)].electronicInE1 if numOfNodesX%4==0&&i%2==0;
			tileNW_Empty[j*(i+1>>1)+(j-1)].electronicOutE1 --> tileNW_Empty[j*(i+1>>1)+(j-1)].electronicInE0 if numOfNodesX%4==0&&i%2==0;
			
		

			////

            uturnEW[i*2+1].photonicInnerBendOut <--> tileSW_Empty[j*(i>>1)+(j-1)].photonicInE1 if numOfNodesX%4!=0&&i%2!=0;
            uturnEW[i*2+1].photonicOuterBendOut <--> tileSW_Empty[j*(i>>1)+(j-1)].photonicInE0 if numOfNodesX%4!=0&&i%2!=0;
            uturnEW[i*2+1].photonicInnerBendIn <--> tileSW_Empty[j*(i>>1)+(j-1)].photonicOutE0 if numOfNodesX%4!=0&&i%2!=0;
            uturnEW[i*2+1].photonicOuterBendIn <--> tileSW_Empty[j*(i>>1)+(j-1)].photonicOutE1 if numOfNodesX%4!=0&&i%2!=0;

			tileSW_Empty[j*(i>>1)+(j-1)].electronicOutE0 --> tileSW_Empty[j*(i>>1)+(j-1)].electronicInE1 if numOfNodesX%4!=0&&i%2!=0;
			tileSW_Empty[j*(i>>1)+(j-1)].electronicOutE1 --> tileSW_Empty[j*(i>>1)+(j-1)].electronicInE0 if numOfNodesX%4!=0&&i%2!=0;
			
			

			////
			
            uturnEW[i*2+1].photonicInnerBendOut <--> tileSE_Empty[j*(i+1>>1)+(j-1)].photonicInE1 if numOfNodesX%4!=0&&i%2==0;
            uturnEW[i*2+1].photonicOuterBendOut <--> tileSE_Empty[j*(i+1>>1)+(j-1)].photonicInE0 if numOfNodesX%4!=0&&i%2==0;
            uturnEW[i*2+1].photonicInnerBendIn <--> tileSE_Empty[j*(i+1>>1)+(j-1)].photonicOutE0 if numOfNodesX%4!=0&&i%2==0;
            uturnEW[i*2+1].photonicOuterBendIn <--> tileSE_Empty[j*(i+1>>1)+(j-1)].photonicOutE1 if numOfNodesX%4!=0&&i%2==0;
            
			tileSE_Empty[j*(i+1>>1)+(j-1)].electronicOutE0 --> tileSE_Empty[j*(i+1>>1)+(j-1)].electronicInE1 if numOfNodesX%4!=0&&i%2==0;
			tileSE_Empty[j*(i+1>>1)+(j-1)].electronicOutE1 --> tileSE_Empty[j*(i+1>>1)+(j-1)].electronicInE0 if numOfNodesX%4!=0&&i%2==0;
			
			
        }


        for i=0..(numOfNodesX>>1)-1, for j=0..(numOfNodesX>>1)-1 {

            // Dimension divisible by 4 case
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicOutE0 <--> tileNW_Empty[i+j*(numOfNodesX>>1)].photonicInW0 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicOutE1 <--> tileNW_Empty[i+j*(numOfNodesX>>1)].photonicInW1 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicInE0 <--> tileNW_Empty[i+j*(numOfNodesX>>1)].photonicOutW0 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicInE1 <--> tileNW_Empty[i+j*(numOfNodesX>>1)].photonicOutW1 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;

            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicOutS0 <--> tileSE_Empty[i+j*(numOfNodesX>>1)].photonicInN0 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicOutS1 <--> tileSE_Empty[i+j*(numOfNodesX>>1)].photonicInN1 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicInS0 <--> tileSE_Empty[i+j*(numOfNodesX>>1)].photonicOutN0 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicInS1 <--> tileSE_Empty[i+j*(numOfNodesX>>1)].photonicOutN1 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;

            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicOutN0 <--> tileSE_Empty[i+(j-1)*(numOfNodesX>>1)].photonicInS0 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&j>0;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicOutN1 <--> tileSE_Empty[i+(j-1)*(numOfNodesX>>1)].photonicInS1 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&j>0;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicInN0 <--> tileSE_Empty[i+(j-1)*(numOfNodesX>>1)].photonicOutS0 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&j>0;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicInN1 <--> tileSE_Empty[i+(j-1)*(numOfNodesX>>1)].photonicOutS1 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&j>0;

            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicOutW0 <--> tileNW_Empty[(i-1)+j*(numOfNodesX>>1)].photonicInE0 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&i>0;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicOutW1 <--> tileNW_Empty[(i-1)+j*(numOfNodesX>>1)].photonicInE1 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&i>0;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicInW0 <--> tileNW_Empty[(i-1)+j*(numOfNodesX>>1)].photonicOutE0 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&i>0;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicInW1 <--> tileNW_Empty[(i-1)+j*(numOfNodesX>>1)].photonicOutE1 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&i>0;


            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicOutE0 <--> tileSE_Empty[(i+1)+j*(numOfNodesX>>1)].photonicInW0 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&i<(numOfNodesX>>1)-1;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicOutE1 <--> tileSE_Empty[(i+1)+j*(numOfNodesX>>1)].photonicInW1 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&i<(numOfNodesX>>1)-1;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicInE0 <--> tileSE_Empty[(i+1)+j*(numOfNodesX>>1)].photonicOutW0 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&i<(numOfNodesX>>1)-1;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicInE1 <--> tileSE_Empty[(i+1)+j*(numOfNodesX>>1)].photonicOutW1 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&i<(numOfNodesX>>1)-1;

            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicOutS0 <--> tileNW_Empty[i+(j+1)*(numOfNodesX>>1)].photonicInN0 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&j<(numOfNodesX>>1)-1;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicOutS1 <--> tileNW_Empty[i+(j+1)*(numOfNodesX>>1)].photonicInN1 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&j<(numOfNodesX>>1)-1;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicInS0 <--> tileNW_Empty[i+(j+1)*(numOfNodesX>>1)].photonicOutN0 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&j<(numOfNodesX>>1)-1;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicInS1 <--> tileNW_Empty[i+(j+1)*(numOfNodesX>>1)].photonicOutN1 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&j<(numOfNodesX>>1)-1;

            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicOutN0 <--> tileNW_Empty[i+j*(numOfNodesX>>1)].photonicInS0 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicOutN1 <--> tileNW_Empty[i+j*(numOfNodesX>>1)].photonicInS1 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicInN0 <--> tileNW_Empty[i+j*(numOfNodesX>>1)].photonicOutS0 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicInN1 <--> tileNW_Empty[i+j*(numOfNodesX>>1)].photonicOutS1 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;

            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicOutW0 <--> tileSE_Empty[i+j*(numOfNodesX>>1)].photonicInE0 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicOutW1 <--> tileSE_Empty[i+j*(numOfNodesX>>1)].photonicInE1 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicInW0 <--> tileSE_Empty[i+j*(numOfNodesX>>1)].photonicOutE0 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicInW1 <--> tileSE_Empty[i+j*(numOfNodesX>>1)].photonicOutE1 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;



            // Dimension not divisible by 4 case
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicOutE0 <--> tileNW_Empty[(i+1)+j*(numOfNodesX>>1)].photonicInW0 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&i<(numOfNodesX>>1)-1;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicOutE1 <--> tileNW_Empty[(i+1)+j*(numOfNodesX>>1)].photonicInW1 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&i<(numOfNodesX>>1)-1;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicInE0 <--> tileNW_Empty[(i+1)+j*(numOfNodesX>>1)].photonicOutW0 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&i<(numOfNodesX>>1)-1;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicInE1 <--> tileNW_Empty[(i+1)+j*(numOfNodesX>>1)].photonicOutW1 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&i<(numOfNodesX>>1)-1;

            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicOutS0 <--> tileSE_Empty[i+(j+1)*(numOfNodesX>>1)].photonicInN0 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&j<(numOfNodesX>>1)-1;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicOutS1 <--> tileSE_Empty[i+(j+1)*(numOfNodesX>>1)].photonicInN1 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&j<(numOfNodesX>>1)-1;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicInS0 <--> tileSE_Empty[i+(j+1)*(numOfNodesX>>1)].photonicOutN0 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&j<(numOfNodesX>>1)-1;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicInS1 <--> tileSE_Empty[i+(j+1)*(numOfNodesX>>1)].photonicOutN1 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&j<(numOfNodesX>>1)-1;

            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicOutN0 <--> tileSE_Empty[i+j*(numOfNodesX>>1)].photonicInS0 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicOutN1 <--> tileSE_Empty[i+j*(numOfNodesX>>1)].photonicInS1 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicInN0 <--> tileSE_Empty[i+j*(numOfNodesX>>1)].photonicOutS0 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicInN1 <--> tileSE_Empty[i+j*(numOfNodesX>>1)].photonicOutS1 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;

            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicOutW0 <--> tileNW_Empty[i+j*(numOfNodesX>>1)].photonicInE0 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicOutW1 <--> tileNW_Empty[i+j*(numOfNodesX>>1)].photonicInE1 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicInW0 <--> tileNW_Empty[i+j*(numOfNodesX>>1)].photonicOutE0 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].photonicInW1 <--> tileNW_Empty[i+j*(numOfNodesX>>1)].photonicOutE1 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;


            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicOutE0 <--> tileSE_Empty[i+j*(numOfNodesX>>1)].photonicInW0 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicOutE1 <--> tileSE_Empty[i+j*(numOfNodesX>>1)].photonicInW1 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicInE0 <--> tileSE_Empty[i+j*(numOfNodesX>>1)].photonicOutW0 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicInE1 <--> tileSE_Empty[i+j*(numOfNodesX>>1)].photonicOutW1 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;

            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicOutS0 <--> tileNW_Empty[i+j*(numOfNodesX>>1)].photonicInN0 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicOutS1 <--> tileNW_Empty[i+j*(numOfNodesX>>1)].photonicInN1 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicInS0 <--> tileNW_Empty[i+j*(numOfNodesX>>1)].photonicOutN0 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicInS1 <--> tileNW_Empty[i+j*(numOfNodesX>>1)].photonicOutN1 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;

            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicOutN0 <--> tileNW_Empty[i+(j-1)*(numOfNodesX>>1)].photonicInS0 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&j>0;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicOutN1 <--> tileNW_Empty[i+(j-1)*(numOfNodesX>>1)].photonicInS1 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&j>0;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicInN0 <--> tileNW_Empty[i+(j-1)*(numOfNodesX>>1)].photonicOutS0 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&j>0;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicInN1 <--> tileNW_Empty[i+(j-1)*(numOfNodesX>>1)].photonicOutS1 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&j>0;

            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicOutW0 <--> tileSE_Empty[(i-1)+j*(numOfNodesX>>1)].photonicInE0 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&i>0;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicOutW1 <--> tileSE_Empty[(i-1)+j*(numOfNodesX>>1)].photonicInE1 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&i>0;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicInW0 <--> tileSE_Empty[(i-1)+j*(numOfNodesX>>1)].photonicOutE0 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&i>0;
            tileNE_Empty[i+j*(numOfNodesX>>1)].photonicInW1 <--> tileSE_Empty[(i-1)+j*(numOfNodesX>>1)].photonicOutE1 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&i>0;



			//ELECTRONIC CASE


// Dimension divisible by 4 case
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicOutE0 --> tileNW_Empty[i+j*(numOfNodesX>>1)].electronicInW0 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicOutE1 --> tileNW_Empty[i+j*(numOfNodesX>>1)].electronicInW1 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicInE0 <-- tileNW_Empty[i+j*(numOfNodesX>>1)].electronicOutW0 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicInE1 <-- tileNW_Empty[i+j*(numOfNodesX>>1)].electronicOutW1 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;

            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicOutS0 --> tileSE_Empty[i+j*(numOfNodesX>>1)].electronicInN0 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicOutS1 --> tileSE_Empty[i+j*(numOfNodesX>>1)].electronicInN1 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicInS0 <-- tileSE_Empty[i+j*(numOfNodesX>>1)].electronicOutN0 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicInS1 <-- tileSE_Empty[i+j*(numOfNodesX>>1)].electronicOutN1 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;

            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicOutN0 --> tileSE_Empty[i+(j-1)*(numOfNodesX>>1)].electronicInS0 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&j>0;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicOutN1 --> tileSE_Empty[i+(j-1)*(numOfNodesX>>1)].electronicInS1 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&j>0;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicInN0 <-- tileSE_Empty[i+(j-1)*(numOfNodesX>>1)].electronicOutS0 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&j>0;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicInN1 <-- tileSE_Empty[i+(j-1)*(numOfNodesX>>1)].electronicOutS1 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&j>0;

            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicOutW0 --> tileNW_Empty[(i-1)+j*(numOfNodesX>>1)].electronicInE0 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&i>0;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicOutW1 --> tileNW_Empty[(i-1)+j*(numOfNodesX>>1)].electronicInE1 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&i>0;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicInW0 <-- tileNW_Empty[(i-1)+j*(numOfNodesX>>1)].electronicOutE0 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&i>0;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicInW1 <-- tileNW_Empty[(i-1)+j*(numOfNodesX>>1)].electronicOutE1 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&i>0;


            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicOutE0 --> tileSE_Empty[(i+1)+j*(numOfNodesX>>1)].electronicInW0 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&i<(numOfNodesX>>1)-1;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicOutE1 --> tileSE_Empty[(i+1)+j*(numOfNodesX>>1)].electronicInW1 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&i<(numOfNodesX>>1)-1;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicInE0 <-- tileSE_Empty[(i+1)+j*(numOfNodesX>>1)].electronicOutW0 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&i<(numOfNodesX>>1)-1;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicInE1 <-- tileSE_Empty[(i+1)+j*(numOfNodesX>>1)].electronicOutW1 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&i<(numOfNodesX>>1)-1;

            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicOutS0 --> tileNW_Empty[i+(j+1)*(numOfNodesX>>1)].electronicInN0 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&j<(numOfNodesX>>1)-1;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicOutS1 --> tileNW_Empty[i+(j+1)*(numOfNodesX>>1)].electronicInN1 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&j<(numOfNodesX>>1)-1;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicInS0 <-- tileNW_Empty[i+(j+1)*(numOfNodesX>>1)].electronicOutN0 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&j<(numOfNodesX>>1)-1;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicInS1 <-- tileNW_Empty[i+(j+1)*(numOfNodesX>>1)].electronicOutN1 if ((numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&j<(numOfNodesX>>1)-1;

            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicOutN0 --> tileNW_Empty[i+j*(numOfNodesX>>1)].electronicInS0 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicOutN1 --> tileNW_Empty[i+j*(numOfNodesX>>1)].electronicInS1 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicInN0 <-- tileNW_Empty[i+j*(numOfNodesX>>1)].electronicOutS0 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicInN1 <-- tileNW_Empty[i+j*(numOfNodesX>>1)].electronicOutS1 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;

            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicOutW0 --> tileSE_Empty[i+j*(numOfNodesX>>1)].electronicInE0 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicOutW1 --> tileSE_Empty[i+j*(numOfNodesX>>1)].electronicInE1 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicInW0 <-- tileSE_Empty[i+j*(numOfNodesX>>1)].electronicOutE0 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicInW1 <-- tileSE_Empty[i+j*(numOfNodesX>>1)].electronicOutE1 if (numOfNodesX%4==0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;



            // Dimension not divisible by 4 case
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicOutE0 --> tileNW_Empty[(i+1)+j*(numOfNodesX>>1)].electronicInW0 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&i<(numOfNodesX>>1)-1;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicOutE1 --> tileNW_Empty[(i+1)+j*(numOfNodesX>>1)].electronicInW1 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&i<(numOfNodesX>>1)-1;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicInE0 <-- tileNW_Empty[(i+1)+j*(numOfNodesX>>1)].electronicOutW0 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&i<(numOfNodesX>>1)-1;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicInE1 <-- tileNW_Empty[(i+1)+j*(numOfNodesX>>1)].electronicOutW1 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&i<(numOfNodesX>>1)-1;

            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicOutS0 --> tileSE_Empty[i+(j+1)*(numOfNodesX>>1)].electronicInN0 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&j<(numOfNodesX>>1)-1;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicOutS1 --> tileSE_Empty[i+(j+1)*(numOfNodesX>>1)].electronicInN1 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&j<(numOfNodesX>>1)-1;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicInS0 <-- tileSE_Empty[i+(j+1)*(numOfNodesX>>1)].electronicOutN0 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&j<(numOfNodesX>>1)-1;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicInS1 <-- tileSE_Empty[i+(j+1)*(numOfNodesX>>1)].electronicOutN1 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j+1)&&j<(numOfNodesX>>1)-1;

            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicOutN0 --> tileSE_Empty[i+j*(numOfNodesX>>1)].electronicInS0 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicOutN1 --> tileSE_Empty[i+j*(numOfNodesX>>1)].electronicInS1 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicInN0 <-- tileSE_Empty[i+j*(numOfNodesX>>1)].electronicOutS0 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicInN1 <-- tileSE_Empty[i+j*(numOfNodesX>>1)].electronicOutS1 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;

            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicOutW0 --> tileNW_Empty[i+j*(numOfNodesX>>1)].electronicInE0 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicOutW1 --> tileNW_Empty[i+j*(numOfNodesX>>1)].electronicInE1 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicInW0 <-- tileNW_Empty[i+j*(numOfNodesX>>1)].electronicOutE0 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileSW_Empty[i+j*(numOfNodesX>>1)].electronicInW1 <-- tileNW_Empty[i+j*(numOfNodesX>>1)].electronicOutE1 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;


            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicOutE0 --> tileSE_Empty[i+j*(numOfNodesX>>1)].electronicInW0 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicOutE1 --> tileSE_Empty[i+j*(numOfNodesX>>1)].electronicInW1 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicInE0 <-- tileSE_Empty[i+j*(numOfNodesX>>1)].electronicOutW0 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicInE1 <-- tileSE_Empty[i+j*(numOfNodesX>>1)].electronicOutW1 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;

            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicOutS0 --> tileNW_Empty[i+j*(numOfNodesX>>1)].electronicInN0 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicOutS1 --> tileNW_Empty[i+j*(numOfNodesX>>1)].electronicInN1 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicInS0 <-- tileNW_Empty[i+j*(numOfNodesX>>1)].electronicOutN0 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicInS1 <-- tileNW_Empty[i+j*(numOfNodesX>>1)].electronicOutN1 if (numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j;

            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicOutN0 --> tileNW_Empty[i+(j-1)*(numOfNodesX>>1)].electronicInS0 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&j>0;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicOutN1 --> tileNW_Empty[i+(j-1)*(numOfNodesX>>1)].electronicInS1 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&j>0;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicInN0 <-- tileNW_Empty[i+(j-1)*(numOfNodesX>>1)].electronicOutS0 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&j>0;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicInN1 <-- tileNW_Empty[i+(j-1)*(numOfNodesX>>1)].electronicOutS1 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&j>0;

            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicOutW0 --> tileSE_Empty[(i-1)+j*(numOfNodesX>>1)].electronicInE0 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&i>0;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicOutW1 --> tileSE_Empty[(i-1)+j*(numOfNodesX>>1)].electronicInE1 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&i>0;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicInW0 <-- tileSE_Empty[(i-1)+j*(numOfNodesX>>1)].electronicOutE0 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&i>0;
            tileNE_Empty[i+j*(numOfNodesX>>1)].electronicInW1 <-- tileSE_Empty[(i-1)+j*(numOfNodesX>>1)].electronicOutE1 if ((numOfNodesX%4!=0&&i!=j)&&((numOfNodesX>>1)-1)-i!=j-1)&&i>0;



        }

	//Connect Diagonal Node Tiles to the rest of the network

	// First Handle Chip corner and chip center case

        tileSW_South[0].photonicOutE0 <--> tileNW_Empty[0].photonicInW0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_South[0].photonicOutE1 <--> tileNW_Empty[0].photonicInW1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_South[0].photonicInE0 <--> tileNW_Empty[0].photonicOutW0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_South[0].photonicInE1 <--> tileNW_Empty[0].photonicOutW1 if numOfNodesX%4==0&&numOfNodesX>2;

        tileSW_South[0].photonicOutS0 <--> tileSE_Empty[0].photonicInN0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_South[0].photonicOutS1 <--> tileSE_Empty[0].photonicInN1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_South[0].photonicInS0 <--> tileSE_Empty[0].photonicOutN0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_South[0].photonicInS1 <--> tileSE_Empty[0].photonicOutN1 if numOfNodesX%4==0&&numOfNodesX>2;



        tileNW_South[0].photonicOutW0 <--> tileSW_Empty[(numOfNodesX>>1)-1].photonicInE0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_South[0].photonicOutW1 <--> tileSW_Empty[(numOfNodesX>>1)-1].photonicInE1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_South[0].photonicInW0 <--> tileSW_Empty[(numOfNodesX>>1)-1].photonicOutE0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_South[0].photonicInW1 <--> tileSW_Empty[(numOfNodesX>>1)-1].photonicOutE1 if numOfNodesX%4==0&&numOfNodesX>2;

        tileNW_South[0].photonicOutS0 <--> tileNE_Empty[(numOfNodesX>>1)-1].photonicInN0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_South[0].photonicOutS1 <--> tileNE_Empty[(numOfNodesX>>1)-1].photonicInN1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_South[0].photonicInS0 <--> tileNE_Empty[(numOfNodesX>>1)-1].photonicOutN0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_South[0].photonicInS1 <--> tileNE_Empty[(numOfNodesX>>1)-1].photonicOutN1 if numOfNodesX%4==0&&numOfNodesX>2;



        tileSE_North[0].photonicOutE0 <--> tileNE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].photonicInW0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_North[0].photonicOutE1 <--> tileNE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].photonicInW1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_North[0].photonicInE0 <--> tileNE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].photonicOutW0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_North[0].photonicInE1 <--> tileNE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].photonicOutW1 if numOfNodesX%4==0&&numOfNodesX>2;

        tileSE_North[0].photonicOutN0 <--> tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].photonicInS0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_North[0].photonicOutN1 <--> tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].photonicInS1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_North[0].photonicInN0 <--> tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].photonicOutS0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_North[0].photonicInN1 <--> tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].photonicOutS1 if numOfNodesX%4==0&&numOfNodesX>2;



        tileNE_North[0].photonicOutW0 <--> tileSE_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].photonicInE0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_North[0].photonicOutW1 <--> tileSE_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].photonicInE1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_North[0].photonicInW0 <--> tileSE_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].photonicOutE0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_North[0].photonicInW1 <--> tileSE_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].photonicOutE1 if numOfNodesX%4==0&&numOfNodesX>2;

        tileNE_North[0].photonicOutN0 <--> tileNW_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].photonicInS0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_North[0].photonicOutN1 <--> tileNW_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].photonicInS1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_North[0].photonicInN0 <--> tileNW_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].photonicOutS0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_North[0].photonicInN1 <--> tileNW_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].photonicOutS1 if numOfNodesX%4==0&&numOfNodesX>2;









        tileNE_South[0].photonicOutE0 <--> tileSE_Empty[0].photonicInW0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[0].photonicOutE1 <--> tileSE_Empty[0].photonicInW1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[0].photonicInE0 <--> tileSE_Empty[0].photonicOutW0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[0].photonicInE1 <--> tileSE_Empty[0].photonicOutW1 if numOfNodesX%4!=0&&numOfNodesX>2;

        tileNE_South[0].photonicOutS0 <--> tileNW_Empty[0].photonicInN0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[0].photonicOutS1 <--> tileNW_Empty[0].photonicInN1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[0].photonicInS0 <--> tileNW_Empty[0].photonicOutN0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[0].photonicInS1 <--> tileNW_Empty[0].photonicOutN1 if numOfNodesX%4!=0&&numOfNodesX>2;



        tileSE_South[0].photonicOutW0 <--> tileNE_Empty[(numOfNodesX>>1)-1].photonicInE0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[0].photonicOutW1 <--> tileNE_Empty[(numOfNodesX>>1)-1].photonicInE1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[0].photonicInW0 <--> tileNE_Empty[(numOfNodesX>>1)-1].photonicOutE0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[0].photonicInW1 <--> tileNE_Empty[(numOfNodesX>>1)-1].photonicOutE1 if numOfNodesX%4!=0&&numOfNodesX>2;

        tileSE_South[0].photonicOutS0 <--> tileSW_Empty[(numOfNodesX>>1)-1].photonicInN0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[0].photonicOutS1 <--> tileSW_Empty[(numOfNodesX>>1)-1].photonicInN1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[0].photonicInS0 <--> tileSW_Empty[(numOfNodesX>>1)-1].photonicOutN0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[0].photonicInS1 <--> tileSW_Empty[(numOfNodesX>>1)-1].photonicOutN1 if numOfNodesX%4!=0&&numOfNodesX>2;



        tileNW_North[0].photonicOutE0 <--> tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].photonicInW0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[0].photonicOutE1 <--> tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].photonicInW1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[0].photonicInE0 <--> tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].photonicOutW0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[0].photonicInE1 <--> tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].photonicOutW1 if numOfNodesX%4!=0&&numOfNodesX>2;

        tileNW_North[0].photonicOutN0 <--> tileNE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].photonicInS0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[0].photonicOutN1 <--> tileNE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].photonicInS1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[0].photonicInN0 <--> tileNE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].photonicOutS0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[0].photonicInN1 <--> tileNE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].photonicOutS1 if numOfNodesX%4!=0&&numOfNodesX>2;



        tileSW_North[0].photonicOutW0 <--> tileNW_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].photonicInE0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[0].photonicOutW1 <--> tileNW_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].photonicInE1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[0].photonicInW0 <--> tileNW_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].photonicOutE0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[0].photonicInW1 <--> tileNW_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].photonicOutE1 if numOfNodesX%4!=0&&numOfNodesX>2;

        tileSW_North[0].photonicOutN0 <--> tileSE_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].photonicInS0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[0].photonicOutN1 <--> tileSE_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].photonicInS1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[0].photonicInN0 <--> tileSE_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].photonicOutS0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[0].photonicInN1 <--> tileSE_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].photonicOutS1 if numOfNodesX%4!=0&&numOfNodesX>2;

//
		tileSW_South[0].electronicOutE0 --> tileNW_Empty[0].electronicInW0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_South[0].electronicOutE1 --> tileNW_Empty[0].electronicInW1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_South[0].electronicInE0 <-- tileNW_Empty[0].electronicOutW0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_South[0].electronicInE1 <-- tileNW_Empty[0].electronicOutW1 if numOfNodesX%4==0&&numOfNodesX>2;

        tileSW_South[0].electronicOutS0 --> tileSE_Empty[0].electronicInN0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_South[0].electronicOutS1 --> tileSE_Empty[0].electronicInN1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_South[0].electronicInS0 <-- tileSE_Empty[0].electronicOutN0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_South[0].electronicInS1 <-- tileSE_Empty[0].electronicOutN1 if numOfNodesX%4==0&&numOfNodesX>2;



        tileNW_South[0].electronicOutW0 --> tileSW_Empty[(numOfNodesX>>1)-1].electronicInE0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_South[0].electronicOutW1 --> tileSW_Empty[(numOfNodesX>>1)-1].electronicInE1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_South[0].electronicInW0 <-- tileSW_Empty[(numOfNodesX>>1)-1].electronicOutE0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_South[0].electronicInW1 <-- tileSW_Empty[(numOfNodesX>>1)-1].electronicOutE1 if numOfNodesX%4==0&&numOfNodesX>2;

        tileNW_South[0].electronicOutS0 --> tileNE_Empty[(numOfNodesX>>1)-1].electronicInN0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_South[0].electronicOutS1 --> tileNE_Empty[(numOfNodesX>>1)-1].electronicInN1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_South[0].electronicInS0 <-- tileNE_Empty[(numOfNodesX>>1)-1].electronicOutN0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_South[0].electronicInS1 <-- tileNE_Empty[(numOfNodesX>>1)-1].electronicOutN1 if numOfNodesX%4==0&&numOfNodesX>2;



        tileSE_North[0].electronicOutE0 --> tileNE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].electronicInW0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_North[0].electronicOutE1 --> tileNE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].electronicInW1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_North[0].electronicInE0 <-- tileNE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].electronicOutW0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_North[0].electronicInE1 <-- tileNE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].electronicOutW1 if numOfNodesX%4==0&&numOfNodesX>2;

        tileSE_North[0].electronicOutN0 --> tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].electronicInS0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_North[0].electronicOutN1 --> tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].electronicInS1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_North[0].electronicInN0 <-- tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].electronicOutS0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_North[0].electronicInN1 <-- tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].electronicOutS1 if numOfNodesX%4==0&&numOfNodesX>2;



        tileNE_North[0].electronicOutW0 --> tileSE_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].electronicInE0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_North[0].electronicOutW1 --> tileSE_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].electronicInE1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_North[0].electronicInW0 <-- tileSE_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].electronicOutE0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_North[0].electronicInW1 <-- tileSE_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].electronicOutE1 if numOfNodesX%4==0&&numOfNodesX>2;

        tileNE_North[0].electronicOutN0 --> tileNW_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].electronicInS0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_North[0].electronicOutN1 --> tileNW_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].electronicInS1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_North[0].electronicInN0 <-- tileNW_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].electronicOutS0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_North[0].electronicInN1 <-- tileNW_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].electronicOutS1 if numOfNodesX%4==0&&numOfNodesX>2;









        tileNE_South[0].electronicOutE0 --> tileSE_Empty[0].electronicInW0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[0].electronicOutE1 --> tileSE_Empty[0].electronicInW1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[0].electronicInE0 <-- tileSE_Empty[0].electronicOutW0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[0].electronicInE1 <-- tileSE_Empty[0].electronicOutW1 if numOfNodesX%4!=0&&numOfNodesX>2;

        tileNE_South[0].electronicOutS0 --> tileNW_Empty[0].electronicInN0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[0].electronicOutS1 --> tileNW_Empty[0].electronicInN1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[0].electronicInS0 <-- tileNW_Empty[0].electronicOutN0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[0].electronicInS1 <-- tileNW_Empty[0].electronicOutN1 if numOfNodesX%4!=0&&numOfNodesX>2;



        tileSE_South[0].electronicOutW0 --> tileNE_Empty[(numOfNodesX>>1)-1].electronicInE0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[0].electronicOutW1 --> tileNE_Empty[(numOfNodesX>>1)-1].electronicInE1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[0].electronicInW0 <-- tileNE_Empty[(numOfNodesX>>1)-1].electronicOutE0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[0].electronicInW1 <-- tileNE_Empty[(numOfNodesX>>1)-1].electronicOutE1 if numOfNodesX%4!=0&&numOfNodesX>2;

        tileSE_South[0].electronicOutS0 --> tileSW_Empty[(numOfNodesX>>1)-1].electronicInN0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[0].electronicOutS1 --> tileSW_Empty[(numOfNodesX>>1)-1].electronicInN1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[0].electronicInS0 <-- tileSW_Empty[(numOfNodesX>>1)-1].electronicOutN0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[0].electronicInS1 <-- tileSW_Empty[(numOfNodesX>>1)-1].electronicOutN1 if numOfNodesX%4!=0&&numOfNodesX>2;



        tileNW_North[0].electronicOutE0 --> tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].electronicInW0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[0].electronicOutE1 --> tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].electronicInW1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[0].electronicInE0 <-- tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].electronicOutW0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[0].electronicInE1 <-- tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].electronicOutW1 if numOfNodesX%4!=0&&numOfNodesX>2;

        tileNW_North[0].electronicOutN0 --> tileNE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].electronicInS0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[0].electronicOutN1 --> tileNE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].electronicInS1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[0].electronicInN0 <-- tileNE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].electronicOutS0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[0].electronicInN1 <-- tileNE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-1)].electronicOutS1 if numOfNodesX%4!=0&&numOfNodesX>2;



        tileSW_North[0].electronicOutW0 --> tileNW_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].electronicInE0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[0].electronicOutW1 --> tileNW_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].electronicInE1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[0].electronicInW0 <-- tileNW_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].electronicOutE0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[0].electronicInW1 <-- tileNW_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].electronicOutE1 if numOfNodesX%4!=0&&numOfNodesX>2;

        tileSW_North[0].electronicOutN0 --> tileSE_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].electronicInS0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[0].electronicOutN1 --> tileSE_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].electronicInS1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[0].electronicInN0 <-- tileSE_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].electronicOutS0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[0].electronicInN1 <-- tileSE_Empty[(numOfNodesX>>1)*(numOfNodesX>>1)-1].electronicOutS1 if numOfNodesX%4!=0&&numOfNodesX>2;




	// Center Node Cases
for i = (numOfNodesX>>2)..(numOfNodesX>>2)
{
        tileNE_South[(numOfNodesX+2>>2)-1].photonicOutW0 <--> tileSE_Empty[((numOfNodesX>>1)*i-1)-i].photonicInE0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].photonicOutW1 <--> tileSE_Empty[((numOfNodesX>>1)*i-1)-i].photonicInE1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].photonicInW0 <--> tileSE_Empty[((numOfNodesX>>1)*i-1)-i].photonicOutE0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].photonicInW1 <--> tileSE_Empty[((numOfNodesX>>1)*i-1)-i].photonicOutE1 if numOfNodesX%4==0&&numOfNodesX>2;

        tileNE_South[(numOfNodesX+2>>2)-1].photonicOutN0 <--> tileNW_Empty[((numOfNodesX>>1)*i-1)-i].photonicInS0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].photonicOutN1 <--> tileNW_Empty[((numOfNodesX>>1)*i-1)-i].photonicInS1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].photonicInN0 <--> tileNW_Empty[((numOfNodesX>>1)*i-1)-i].photonicOutS0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].photonicInN1 <--> tileNW_Empty[((numOfNodesX>>1)*i-1)-i].photonicOutS1 if numOfNodesX%4==0&&numOfNodesX>2;


        tileSE_South[(numOfNodesX+2>>2)-1].photonicOutE0 <--> tileNE_Empty[(numOfNodesX>>1)*i-i].photonicInW0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].photonicOutE1 <--> tileNE_Empty[(numOfNodesX>>1)*i-i].photonicInW1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].photonicInE0 <--> tileNE_Empty[(numOfNodesX>>1)*i-i].photonicOutW0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].photonicInE1 <--> tileNE_Empty[(numOfNodesX>>1)*i-i].photonicOutW1 if numOfNodesX%4==0&&numOfNodesX>2;

        tileSE_South[(numOfNodesX+2>>2)-1].photonicOutN0 <--> tileSW_Empty[(numOfNodesX>>1)*i-i].photonicInS0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].photonicOutN1 <--> tileSW_Empty[(numOfNodesX>>1)*i-i].photonicInS1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].photonicInN0 <--> tileSW_Empty[(numOfNodesX>>1)*i-i].photonicOutS0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].photonicInN1 <--> tileSW_Empty[(numOfNodesX>>1)*i-i].photonicOutS1 if numOfNodesX%4==0&&numOfNodesX>2;


        tileNW_North[(numOfNodesX+2>>2)-1].photonicOutW0 <--> tileSW_Empty[((numOfNodesX>>1)*i+i)-1].photonicInE0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].photonicOutW1 <--> tileSW_Empty[((numOfNodesX>>1)*i+i)-1].photonicInE1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].photonicInW0 <--> tileSW_Empty[((numOfNodesX>>1)*i+i)-1].photonicOutE0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].photonicInW1 <--> tileSW_Empty[((numOfNodesX>>1)*i+i)-1].photonicOutE1 if numOfNodesX%4==0&&numOfNodesX>2;

        tileNW_North[(numOfNodesX+2>>2)-1].photonicOutS0 <--> tileNE_Empty[((numOfNodesX>>1)*i+i)-1].photonicInN0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].photonicOutS1 <--> tileNE_Empty[((numOfNodesX>>1)*i+i)-1].photonicInN1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].photonicInS0 <--> tileNE_Empty[((numOfNodesX>>1)*i+i)-1].photonicOutN0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].photonicInS1 <--> tileNE_Empty[((numOfNodesX>>1)*i+i)-1].photonicOutN1 if numOfNodesX%4==0&&numOfNodesX>2;


        tileSW_North[(numOfNodesX+2>>2)-1].photonicOutE0 <--> tileNW_Empty[(numOfNodesX>>1)*i+i].photonicInW0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].photonicOutE1 <--> tileNW_Empty[(numOfNodesX>>1)*i+i].photonicInW1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].photonicInE0 <--> tileNW_Empty[(numOfNodesX>>1)*i+i].photonicOutW0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].photonicInE1 <--> tileNW_Empty[(numOfNodesX>>1)*i+i].photonicOutW1 if numOfNodesX%4==0&&numOfNodesX>2;

        tileSW_North[(numOfNodesX+2>>2)-1].photonicOutS0 <--> tileSE_Empty[(numOfNodesX>>1)*i+i].photonicInN0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].photonicOutS1 <--> tileSE_Empty[(numOfNodesX>>1)*i+i].photonicInN1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].photonicInS0 <--> tileSE_Empty[(numOfNodesX>>1)*i+i].photonicOutN0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].photonicInS1 <--> tileSE_Empty[(numOfNodesX>>1)*i+i].photonicOutN1 if numOfNodesX%4==0&&numOfNodesX>2;







        tileNE_South[(numOfNodesX+2>>2)-1].photonicOutW0 <--> tileSE_Empty[((numOfNodesX>>1)*i-1)+i].photonicInE0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].photonicOutW1 <--> tileSE_Empty[((numOfNodesX>>1)*i-1)+i].photonicInE1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].photonicInW0 <--> tileSE_Empty[((numOfNodesX>>1)*i-1)+i].photonicOutE0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].photonicInW1 <--> tileSE_Empty[((numOfNodesX>>1)*i-1)+i].photonicOutE1 if numOfNodesX%4!=0&&numOfNodesX>2;

        tileNE_South[(numOfNodesX+2>>2)-1].photonicOutN0 <--> tileNW_Empty[(numOfNodesX>>1)*(i-1)+i].photonicInS0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].photonicOutN1 <--> tileNW_Empty[(numOfNodesX>>1)*(i-1)+i].photonicInS1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].photonicInN0 <--> tileNW_Empty[(numOfNodesX>>1)*(i-1)+i].photonicOutS0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].photonicInN1 <--> tileNW_Empty[(numOfNodesX>>1)*(i-1)+i].photonicOutS1 if numOfNodesX%4!=0&&numOfNodesX>2;


        tileSE_South[(numOfNodesX+2>>2)-1].photonicOutE0 <--> tileNE_Empty[((numOfNodesX>>1)*i+1)+i].photonicInW0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].photonicOutE1 <--> tileNE_Empty[((numOfNodesX>>1)*i+1)+i].photonicInW1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].photonicInE0 <--> tileNE_Empty[((numOfNodesX>>1)*i+1)+i].photonicOutW0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].photonicInE1 <--> tileNE_Empty[((numOfNodesX>>1)*i+1)+i].photonicOutW1 if numOfNodesX%4!=0&&numOfNodesX>2;

        tileSE_South[(numOfNodesX+2>>2)-1].photonicOutN0 <--> tileSW_Empty[(numOfNodesX>>1)*(i-1)+i].photonicInS0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].photonicOutN1 <--> tileSW_Empty[(numOfNodesX>>1)*(i-1)+i].photonicInS1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].photonicInN0 <--> tileSW_Empty[(numOfNodesX>>1)*(i-1)+i].photonicOutS0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].photonicInN1 <--> tileSW_Empty[(numOfNodesX>>1)*(i-1)+i].photonicOutS1 if numOfNodesX%4!=0&&numOfNodesX>2;


        tileNW_North[(numOfNodesX+2>>2)-1].photonicOutW0 <--> tileSW_Empty[((numOfNodesX>>1)*i-1)+i].photonicInE0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].photonicOutW1 <--> tileSW_Empty[((numOfNodesX>>1)*i-1)+i].photonicInE1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].photonicInW0 <--> tileSW_Empty[((numOfNodesX>>1)*i-1)+i].photonicOutE0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].photonicInW1 <--> tileSW_Empty[((numOfNodesX>>1)*i-1)+i].photonicOutE1 if numOfNodesX%4!=0&&numOfNodesX>2;

        tileNW_North[(numOfNodesX+2>>2)-1].photonicOutS0 <--> tileNE_Empty[(numOfNodesX>>1)*(i+1)+i].photonicInN0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].photonicOutS1 <--> tileNE_Empty[(numOfNodesX>>1)*(i+1)+i].photonicInN1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].photonicInS0 <--> tileNE_Empty[(numOfNodesX>>1)*(i+1)+i].photonicOutN0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].photonicInS1 <--> tileNE_Empty[(numOfNodesX>>1)*(i+1)+i].photonicOutN1 if numOfNodesX%4!=0&&numOfNodesX>2;


        tileSW_North[(numOfNodesX+2>>2)-1].photonicOutE0 <--> tileNW_Empty[((numOfNodesX>>1)*i+1)+i].photonicInW0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].photonicOutE1 <--> tileNW_Empty[((numOfNodesX>>1)*i+1)+i].photonicInW1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].photonicInE0 <--> tileNW_Empty[((numOfNodesX>>1)*i+1)+i].photonicOutW0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].photonicInE1 <--> tileNW_Empty[((numOfNodesX>>1)*i+1)+i].photonicOutW1 if numOfNodesX%4!=0&&numOfNodesX>2;

        tileSW_North[(numOfNodesX+2>>2)-1].photonicOutS0 <--> tileSE_Empty[(numOfNodesX>>1)*(i+1)+i].photonicInN0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].photonicOutS1 <--> tileSE_Empty[(numOfNodesX>>1)*(i+1)+i].photonicInN1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].photonicInS0 <--> tileSE_Empty[(numOfNodesX>>1)*(i+1)+i].photonicOutN0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].photonicInS1 <--> tileSE_Empty[(numOfNodesX>>1)*(i+1)+i].photonicOutN1 if numOfNodesX%4!=0&&numOfNodesX>2;

//


tileNE_South[(numOfNodesX+2>>2)-1].electronicOutW0 --> tileSE_Empty[((numOfNodesX>>1)*i-1)-i].electronicInE0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].electronicOutW1 --> tileSE_Empty[((numOfNodesX>>1)*i-1)-i].electronicInE1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].electronicInW0 <-- tileSE_Empty[((numOfNodesX>>1)*i-1)-i].electronicOutE0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].electronicInW1 <-- tileSE_Empty[((numOfNodesX>>1)*i-1)-i].electronicOutE1 if numOfNodesX%4==0&&numOfNodesX>2;

        tileNE_South[(numOfNodesX+2>>2)-1].electronicOutN0 --> tileNW_Empty[((numOfNodesX>>1)*i-1)-i].electronicInS0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].electronicOutN1 --> tileNW_Empty[((numOfNodesX>>1)*i-1)-i].electronicInS1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].electronicInN0 <-- tileNW_Empty[((numOfNodesX>>1)*i-1)-i].electronicOutS0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].electronicInN1 <-- tileNW_Empty[((numOfNodesX>>1)*i-1)-i].electronicOutS1 if numOfNodesX%4==0&&numOfNodesX>2;


        tileSE_South[(numOfNodesX+2>>2)-1].electronicOutE0 --> tileNE_Empty[(numOfNodesX>>1)*i-i].electronicInW0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].electronicOutE1 --> tileNE_Empty[(numOfNodesX>>1)*i-i].electronicInW1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].electronicInE0 <-- tileNE_Empty[(numOfNodesX>>1)*i-i].electronicOutW0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].electronicInE1 <-- tileNE_Empty[(numOfNodesX>>1)*i-i].electronicOutW1 if numOfNodesX%4==0&&numOfNodesX>2;

        tileSE_South[(numOfNodesX+2>>2)-1].electronicOutN0 --> tileSW_Empty[(numOfNodesX>>1)*i-i].electronicInS0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].electronicOutN1 --> tileSW_Empty[(numOfNodesX>>1)*i-i].electronicInS1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].electronicInN0 <-- tileSW_Empty[(numOfNodesX>>1)*i-i].electronicOutS0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].electronicInN1 <-- tileSW_Empty[(numOfNodesX>>1)*i-i].electronicOutS1 if numOfNodesX%4==0&&numOfNodesX>2;


        tileNW_North[(numOfNodesX+2>>2)-1].electronicOutW0 --> tileSW_Empty[((numOfNodesX>>1)*i+i)-1].electronicInE0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].electronicOutW1 --> tileSW_Empty[((numOfNodesX>>1)*i+i)-1].electronicInE1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].electronicInW0 <-- tileSW_Empty[((numOfNodesX>>1)*i+i)-1].electronicOutE0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].electronicInW1 <-- tileSW_Empty[((numOfNodesX>>1)*i+i)-1].electronicOutE1 if numOfNodesX%4==0&&numOfNodesX>2;

        tileNW_North[(numOfNodesX+2>>2)-1].electronicOutS0 --> tileNE_Empty[((numOfNodesX>>1)*i+i)-1].electronicInN0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].electronicOutS1 --> tileNE_Empty[((numOfNodesX>>1)*i+i)-1].electronicInN1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].electronicInS0 <-- tileNE_Empty[((numOfNodesX>>1)*i+i)-1].electronicOutN0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].electronicInS1 <-- tileNE_Empty[((numOfNodesX>>1)*i+i)-1].electronicOutN1 if numOfNodesX%4==0&&numOfNodesX>2;


        tileSW_North[(numOfNodesX+2>>2)-1].electronicOutE0 --> tileNW_Empty[(numOfNodesX>>1)*i+i].electronicInW0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].electronicOutE1 --> tileNW_Empty[(numOfNodesX>>1)*i+i].electronicInW1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].electronicInE0 <-- tileNW_Empty[(numOfNodesX>>1)*i+i].electronicOutW0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].electronicInE1 <-- tileNW_Empty[(numOfNodesX>>1)*i+i].electronicOutW1 if numOfNodesX%4==0&&numOfNodesX>2;

        tileSW_North[(numOfNodesX+2>>2)-1].electronicOutS0 --> tileSE_Empty[(numOfNodesX>>1)*i+i].electronicInN0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].electronicOutS1 --> tileSE_Empty[(numOfNodesX>>1)*i+i].electronicInN1 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].electronicInS0 <-- tileSE_Empty[(numOfNodesX>>1)*i+i].electronicOutN0 if numOfNodesX%4==0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].electronicInS1 <-- tileSE_Empty[(numOfNodesX>>1)*i+i].electronicOutN1 if numOfNodesX%4==0&&numOfNodesX>2;







        tileNE_South[(numOfNodesX+2>>2)-1].electronicOutW0 --> tileSE_Empty[((numOfNodesX>>1)*i-1)+i].electronicInE0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].electronicOutW1 --> tileSE_Empty[((numOfNodesX>>1)*i-1)+i].electronicInE1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].electronicInW0 <-- tileSE_Empty[((numOfNodesX>>1)*i-1)+i].electronicOutE0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].electronicInW1 <-- tileSE_Empty[((numOfNodesX>>1)*i-1)+i].electronicOutE1 if numOfNodesX%4!=0&&numOfNodesX>2;

        tileNE_South[(numOfNodesX+2>>2)-1].electronicOutN0 --> tileNW_Empty[(numOfNodesX>>1)*(i-1)+i].electronicInS0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].electronicOutN1 --> tileNW_Empty[(numOfNodesX>>1)*(i-1)+i].electronicInS1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].electronicInN0 <-- tileNW_Empty[(numOfNodesX>>1)*(i-1)+i].electronicOutS0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNE_South[(numOfNodesX+2>>2)-1].electronicInN1 <-- tileNW_Empty[(numOfNodesX>>1)*(i-1)+i].electronicOutS1 if numOfNodesX%4!=0&&numOfNodesX>2;


        tileSE_South[(numOfNodesX+2>>2)-1].electronicOutE0 --> tileNE_Empty[((numOfNodesX>>1)*i+1)+i].electronicInW0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].electronicOutE1 --> tileNE_Empty[((numOfNodesX>>1)*i+1)+i].electronicInW1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].electronicInE0 <-- tileNE_Empty[((numOfNodesX>>1)*i+1)+i].electronicOutW0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].electronicInE1 <-- tileNE_Empty[((numOfNodesX>>1)*i+1)+i].electronicOutW1 if numOfNodesX%4!=0&&numOfNodesX>2;

        tileSE_South[(numOfNodesX+2>>2)-1].electronicOutN0 --> tileSW_Empty[(numOfNodesX>>1)*(i-1)+i].electronicInS0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].electronicOutN1 --> tileSW_Empty[(numOfNodesX>>1)*(i-1)+i].electronicInS1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].electronicInN0 <-- tileSW_Empty[(numOfNodesX>>1)*(i-1)+i].electronicOutS0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSE_South[(numOfNodesX+2>>2)-1].electronicInN1 <-- tileSW_Empty[(numOfNodesX>>1)*(i-1)+i].electronicOutS1 if numOfNodesX%4!=0&&numOfNodesX>2;


        tileNW_North[(numOfNodesX+2>>2)-1].electronicOutW0 --> tileSW_Empty[((numOfNodesX>>1)*i-1)+i].electronicInE0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].electronicOutW1 --> tileSW_Empty[((numOfNodesX>>1)*i-1)+i].electronicInE1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].electronicInW0 <-- tileSW_Empty[((numOfNodesX>>1)*i-1)+i].electronicOutE0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].electronicInW1 <-- tileSW_Empty[((numOfNodesX>>1)*i-1)+i].electronicOutE1 if numOfNodesX%4!=0&&numOfNodesX>2;

        tileNW_North[(numOfNodesX+2>>2)-1].electronicOutS0 --> tileNE_Empty[(numOfNodesX>>1)*(i+1)+i].electronicInN0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].electronicOutS1 --> tileNE_Empty[(numOfNodesX>>1)*(i+1)+i].electronicInN1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].electronicInS0 <-- tileNE_Empty[(numOfNodesX>>1)*(i+1)+i].electronicOutN0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileNW_North[(numOfNodesX+2>>2)-1].electronicInS1 <-- tileNE_Empty[(numOfNodesX>>1)*(i+1)+i].electronicOutN1 if numOfNodesX%4!=0&&numOfNodesX>2;


        tileSW_North[(numOfNodesX+2>>2)-1].electronicOutE0 --> tileNW_Empty[((numOfNodesX>>1)*i+1)+i].electronicInW0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].electronicOutE1 --> tileNW_Empty[((numOfNodesX>>1)*i+1)+i].electronicInW1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].electronicInE0 <-- tileNW_Empty[((numOfNodesX>>1)*i+1)+i].electronicOutW0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].electronicInE1 <-- tileNW_Empty[((numOfNodesX>>1)*i+1)+i].electronicOutW1 if numOfNodesX%4!=0&&numOfNodesX>2;

        tileSW_North[(numOfNodesX+2>>2)-1].electronicOutS0 --> tileSE_Empty[(numOfNodesX>>1)*(i+1)+i].electronicInN0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].electronicOutS1 --> tileSE_Empty[(numOfNodesX>>1)*(i+1)+i].electronicInN1 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].electronicInS0 <-- tileSE_Empty[(numOfNodesX>>1)*(i+1)+i].electronicOutN0 if numOfNodesX%4!=0&&numOfNodesX>2;
        tileSW_North[(numOfNodesX+2>>2)-1].electronicInS1 <-- tileSE_Empty[(numOfNodesX>>1)*(i+1)+i].electronicOutN1 if numOfNodesX%4!=0&&numOfNodesX>2;




}
	//Loop to finish up connecting other nodes to network

	//Diagonals that aren't located in center

        for i=1..(numOfNodesX>>2)-1,
        for j = (numOfNodesX>>1)..(numOfNodesX>>1)
        {
            tileSW_South[i].photonicOutE0 <--> tileNW_Empty[(numOfNodesX>>1)*i+i].photonicInW0 if numOfNodesX%4==0;
            tileSW_South[i].photonicOutE1 <--> tileNW_Empty[(numOfNodesX>>1)*i+i].photonicInW1 if numOfNodesX%4==0;
            tileSW_South[i].photonicInE0 <--> tileNW_Empty[(numOfNodesX>>1)*i+i].photonicOutW0 if numOfNodesX%4==0;
            tileSW_South[i].photonicInE1 <--> tileNW_Empty[(numOfNodesX>>1)*i+i].photonicOutW1 if numOfNodesX%4==0;

            tileSW_South[i].photonicOutS0 <--> tileSE_Empty[(numOfNodesX>>1)*i+i].photonicInN0 if numOfNodesX%4==0;
            tileSW_South[i].photonicOutS1 <--> tileSE_Empty[(numOfNodesX>>1)*i+i].photonicInN1 if numOfNodesX%4==0;
            tileSW_South[i].photonicInS0 <--> tileSE_Empty[(numOfNodesX>>1)*i+i].photonicOutN0 if numOfNodesX%4==0;
            tileSW_South[i].photonicInS1 <--> tileSE_Empty[(numOfNodesX>>1)*i+i].photonicOutN1 if numOfNodesX%4==0;

            tileSW_South[i].photonicOutW0 <--> tileNW_Empty[((numOfNodesX>>1)*i+i)-1].photonicInE0 if numOfNodesX%4==0;
            tileSW_South[i].photonicOutW1 <--> tileNW_Empty[((numOfNodesX>>1)*i+i)-1].photonicInE1 if numOfNodesX%4==0;
            tileSW_South[i].photonicInW0 <--> tileNW_Empty[((numOfNodesX>>1)*i+i)-1].photonicOutE0 if numOfNodesX%4==0;
            tileSW_South[i].photonicInW1 <--> tileNW_Empty[((numOfNodesX>>1)*i+i)-1].photonicOutE1 if numOfNodesX%4==0;

            tileSW_South[i].photonicOutN0 <--> tileSE_Empty[(numOfNodesX>>1)*(i-1)+i].photonicInS0 if numOfNodesX%4==0;
            tileSW_South[i].photonicOutN1 <--> tileSE_Empty[(numOfNodesX>>1)*(i-1)+i].photonicInS1 if numOfNodesX%4==0;
            tileSW_South[i].photonicInN0 <--> tileSE_Empty[(numOfNodesX>>1)*(i-1)+i].photonicOutS0 if numOfNodesX%4==0;
            tileSW_South[i].photonicInN1 <--> tileSE_Empty[(numOfNodesX>>1)*(i-1)+i].photonicOutS1 if numOfNodesX%4==0;



            tileNW_South[i].photonicOutE0 <--> tileSW_Empty[(numOfNodesX>>1)*(i+1)-i].photonicInW0 if numOfNodesX%4==0;
            tileNW_South[i].photonicOutE1 <--> tileSW_Empty[(numOfNodesX>>1)*(i+1)-i].photonicInW1 if numOfNodesX%4==0;
            tileNW_South[i].photonicInE0 <--> tileSW_Empty[(numOfNodesX>>1)*(i+1)-i].photonicOutW0 if numOfNodesX%4==0;
            tileNW_South[i].photonicInE1 <--> tileSW_Empty[(numOfNodesX>>1)*(i+1)-i].photonicOutW1 if numOfNodesX%4==0;

            tileNW_South[i].photonicOutS0 <--> tileNE_Empty[((numOfNodesX>>1)*(i+1)-i)-1].photonicInN0 if numOfNodesX%4==0;
            tileNW_South[i].photonicOutS1 <--> tileNE_Empty[((numOfNodesX>>1)*(i+1)-i)-1].photonicInN1 if numOfNodesX%4==0;
            tileNW_South[i].photonicInS0 <--> tileNE_Empty[((numOfNodesX>>1)*(i+1)-i)-1].photonicOutN0 if numOfNodesX%4==0;
            tileNW_South[i].photonicInS1 <--> tileNE_Empty[((numOfNodesX>>1)*(i+1)-i)-1].photonicOutN1 if numOfNodesX%4==0;

            tileNW_South[i].photonicOutW0 <--> tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-1].photonicInE0 if numOfNodesX%4==0;
            tileNW_South[i].photonicOutW1 <--> tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-1].photonicInE1 if numOfNodesX%4==0;
            tileNW_South[i].photonicInW0 <--> tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-1].photonicOutE0 if numOfNodesX%4==0;
            tileNW_South[i].photonicInW1 <--> tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-1].photonicOutE1 if numOfNodesX%4==0;

            tileNW_South[i].photonicOutN0 <--> tileNE_Empty[((numOfNodesX>>1)*i-i)-1].photonicInS0 if numOfNodesX%4==0;
            tileNW_South[i].photonicOutN1 <--> tileNE_Empty[((numOfNodesX>>1)*i-i)-1].photonicInS1 if numOfNodesX%4==0;
            tileNW_South[i].photonicInN0 <--> tileNE_Empty[((numOfNodesX>>1)*i-i)-1].photonicOutS0 if numOfNodesX%4==0;
            tileNW_South[i].photonicInN1 <--> tileNE_Empty[((numOfNodesX>>1)*i-i)-1].photonicOutS1 if numOfNodesX%4==0;


            tileSE_North[i].photonicOutE0 <--> tileNE_Empty[j*((j-i)-1)+i].photonicInW0 if numOfNodesX%4==0;
            tileSE_North[i].photonicOutE1 <--> tileNE_Empty[j*((j-i)-1)+i].photonicInW1 if numOfNodesX%4==0;
            tileSE_North[i].photonicInE0 <--> tileNE_Empty[j*((j-i)-1)+i].photonicOutW0 if numOfNodesX%4==0;
            tileSE_North[i].photonicInE1 <--> tileNE_Empty[j*((j-i)-1)+i].photonicOutW1 if numOfNodesX%4==0;

            tileSE_North[i].photonicOutS0 <--> tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-i)+i].photonicInN0 if numOfNodesX%4==0;
            tileSE_North[i].photonicOutS1 <--> tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-i)+i].photonicInN1 if numOfNodesX%4==0;
            tileSE_North[i].photonicInS0 <--> tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-i)+i].photonicOutN0 if numOfNodesX%4==0;
            tileSE_North[i].photonicInS1 <--> tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-i)+i].photonicOutN1 if numOfNodesX%4==0;

            tileSE_North[i].photonicOutW0 <--> tileNE_Empty[(j*((j-i)-1)+i)-1].photonicInE0 if numOfNodesX%4==0;
            tileSE_North[i].photonicOutW1 <--> tileNE_Empty[(j*((j-i)-1)+i)-1].photonicInE1 if numOfNodesX%4==0;
            tileSE_North[i].photonicInW0 <--> tileNE_Empty[(j*((j-i)-1)+i)-1].photonicOutE0 if numOfNodesX%4==0;
            tileSE_North[i].photonicInW1 <--> tileNE_Empty[(j*((j-i)-1)+i)-1].photonicOutE1 if numOfNodesX%4==0;

            tileSE_North[i].photonicOutN0 <--> tileSW_Empty[j*((j-i)-1)+i].photonicInS0 if numOfNodesX%4==0;
            tileSE_North[i].photonicOutN1 <--> tileSW_Empty[j*((j-i)-1)+i].photonicInS1 if numOfNodesX%4==0;
            tileSE_North[i].photonicInN0 <--> tileSW_Empty[j*((j-i)-1)+i].photonicOutS0 if numOfNodesX%4==0;
            tileSE_North[i].photonicInN1 <--> tileSW_Empty[j*((j-i)-1)+i].photonicOutS1 if numOfNodesX%4==0;


            tileNE_North[i].photonicOutE0 <--> tileSE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-i)-i].photonicInW0 if numOfNodesX%4==0;
            tileNE_North[i].photonicOutE1 <--> tileSE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-i)-i].photonicInW1 if numOfNodesX%4==0;
            tileNE_North[i].photonicInE0 <--> tileSE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-i)-i].photonicOutW0 if numOfNodesX%4==0;
            tileNE_North[i].photonicInE1 <--> tileSE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-i)-i].photonicOutW1 if numOfNodesX%4==0;

            tileNE_North[i].photonicOutS0 <--> tileNW_Empty[(j*((j-i)+1)-i)-1].photonicInN0 if numOfNodesX%4==0;
            tileNE_North[i].photonicOutS1 <--> tileNW_Empty[(j*((j-i)+1)-i)-1].photonicInN1 if numOfNodesX%4==0;
            tileNE_North[i].photonicInS0 <--> tileNW_Empty[(j*((j-i)+1)-i)-1].photonicOutN0 if numOfNodesX%4==0;
            tileNE_North[i].photonicInS1 <--> tileNW_Empty[(j*((j-i)+1)-i)-1].photonicOutN1 if numOfNodesX%4==0;

            tileNE_North[i].photonicOutW0 <--> tileSE_Empty[(j*(j-i)-i)-1].photonicInE0 if numOfNodesX%4==0;
            tileNE_North[i].photonicOutW1 <--> tileSE_Empty[(j*(j-i)-i)-1].photonicInE1 if numOfNodesX%4==0;
            tileNE_North[i].photonicInW0 <--> tileSE_Empty[(j*(j-i)-i)-1].photonicOutE0 if numOfNodesX%4==0;
            tileNE_North[i].photonicInW1 <--> tileSE_Empty[(j*(j-i)-i)-1].photonicOutE1 if numOfNodesX%4==0;

            tileNE_North[i].photonicOutN0 <--> tileNW_Empty[(j*(j-i)-i)-1].photonicInS0 if numOfNodesX%4==0;
            tileNE_North[i].photonicOutN1 <--> tileNW_Empty[(j*(j-i)-i)-1].photonicInS1 if numOfNodesX%4==0;
            tileNE_North[i].photonicInN0 <--> tileNW_Empty[(j*(j-i)-i)-1].photonicOutS0 if numOfNodesX%4==0;
            tileNE_North[i].photonicInN1 <--> tileNW_Empty[(j*(j-i)-i)-1].photonicOutS1 if numOfNodesX%4==0;
            
            ///
            
            tileSW_South[i].electronicOutE0 --> tileNW_Empty[(numOfNodesX>>1)*i+i].electronicInW0 if numOfNodesX%4==0;
            tileSW_South[i].electronicOutE1 --> tileNW_Empty[(numOfNodesX>>1)*i+i].electronicInW1 if numOfNodesX%4==0;
            tileSW_South[i].electronicInE0 <-- tileNW_Empty[(numOfNodesX>>1)*i+i].electronicOutW0 if numOfNodesX%4==0;
            tileSW_South[i].electronicInE1 <-- tileNW_Empty[(numOfNodesX>>1)*i+i].electronicOutW1 if numOfNodesX%4==0;

            tileSW_South[i].electronicOutS0 --> tileSE_Empty[(numOfNodesX>>1)*i+i].electronicInN0 if numOfNodesX%4==0;
            tileSW_South[i].electronicOutS1 --> tileSE_Empty[(numOfNodesX>>1)*i+i].electronicInN1 if numOfNodesX%4==0;
            tileSW_South[i].electronicInS0 <-- tileSE_Empty[(numOfNodesX>>1)*i+i].electronicOutN0 if numOfNodesX%4==0;
            tileSW_South[i].electronicInS1 <-- tileSE_Empty[(numOfNodesX>>1)*i+i].electronicOutN1 if numOfNodesX%4==0;

            tileSW_South[i].electronicOutW0 --> tileNW_Empty[((numOfNodesX>>1)*i+i)-1].electronicInE0 if numOfNodesX%4==0;
            tileSW_South[i].electronicOutW1 --> tileNW_Empty[((numOfNodesX>>1)*i+i)-1].electronicInE1 if numOfNodesX%4==0;
            tileSW_South[i].electronicInW0 <-- tileNW_Empty[((numOfNodesX>>1)*i+i)-1].electronicOutE0 if numOfNodesX%4==0;
            tileSW_South[i].electronicInW1 <-- tileNW_Empty[((numOfNodesX>>1)*i+i)-1].electronicOutE1 if numOfNodesX%4==0;

            tileSW_South[i].electronicOutN0 --> tileSE_Empty[(numOfNodesX>>1)*(i-1)+i].electronicInS0 if numOfNodesX%4==0;
            tileSW_South[i].electronicOutN1 --> tileSE_Empty[(numOfNodesX>>1)*(i-1)+i].electronicInS1 if numOfNodesX%4==0;
            tileSW_South[i].electronicInN0 <-- tileSE_Empty[(numOfNodesX>>1)*(i-1)+i].electronicOutS0 if numOfNodesX%4==0;
            tileSW_South[i].electronicInN1 <-- tileSE_Empty[(numOfNodesX>>1)*(i-1)+i].electronicOutS1 if numOfNodesX%4==0;



            tileNW_South[i].electronicOutE0 --> tileSW_Empty[(numOfNodesX>>1)*(i+1)-i].electronicInW0 if numOfNodesX%4==0;
            tileNW_South[i].electronicOutE1 --> tileSW_Empty[(numOfNodesX>>1)*(i+1)-i].electronicInW1 if numOfNodesX%4==0;
            tileNW_South[i].electronicInE0 <-- tileSW_Empty[(numOfNodesX>>1)*(i+1)-i].electronicOutW0 if numOfNodesX%4==0;
            tileNW_South[i].electronicInE1 <-- tileSW_Empty[(numOfNodesX>>1)*(i+1)-i].electronicOutW1 if numOfNodesX%4==0;

            tileNW_South[i].electronicOutS0 --> tileNE_Empty[((numOfNodesX>>1)*(i+1)-i)-1].electronicInN0 if numOfNodesX%4==0;
            tileNW_South[i].electronicOutS1 --> tileNE_Empty[((numOfNodesX>>1)*(i+1)-i)-1].electronicInN1 if numOfNodesX%4==0;
            tileNW_South[i].electronicInS0 <-- tileNE_Empty[((numOfNodesX>>1)*(i+1)-i)-1].electronicOutN0 if numOfNodesX%4==0;
            tileNW_South[i].electronicInS1 <-- tileNE_Empty[((numOfNodesX>>1)*(i+1)-i)-1].electronicOutN1 if numOfNodesX%4==0;

            tileNW_South[i].electronicOutW0 --> tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-1].electronicInE0 if numOfNodesX%4==0;
            tileNW_South[i].electronicOutW1 --> tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-1].electronicInE1 if numOfNodesX%4==0;
            tileNW_South[i].electronicInW0 <-- tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-1].electronicOutE0 if numOfNodesX%4==0;
            tileNW_South[i].electronicInW1 <-- tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-1].electronicOutE1 if numOfNodesX%4==0;

            tileNW_South[i].electronicOutN0 --> tileNE_Empty[((numOfNodesX>>1)*i-i)-1].electronicInS0 if numOfNodesX%4==0;
            tileNW_South[i].electronicOutN1 --> tileNE_Empty[((numOfNodesX>>1)*i-i)-1].electronicInS1 if numOfNodesX%4==0;
            tileNW_South[i].electronicInN0 <-- tileNE_Empty[((numOfNodesX>>1)*i-i)-1].electronicOutS0 if numOfNodesX%4==0;
            tileNW_South[i].electronicInN1 <-- tileNE_Empty[((numOfNodesX>>1)*i-i)-1].electronicOutS1 if numOfNodesX%4==0;


            tileSE_North[i].electronicOutE0 --> tileNE_Empty[j*((j-i)-1)+i].electronicInW0 if numOfNodesX%4==0;
            tileSE_North[i].electronicOutE1 --> tileNE_Empty[j*((j-i)-1)+i].electronicInW1 if numOfNodesX%4==0;
            tileSE_North[i].electronicInE0 <-- tileNE_Empty[j*((j-i)-1)+i].electronicOutW0 if numOfNodesX%4==0;
            tileSE_North[i].electronicInE1 <-- tileNE_Empty[j*((j-i)-1)+i].electronicOutW1 if numOfNodesX%4==0;

            tileSE_North[i].electronicOutS0 --> tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-i)+i].electronicInN0 if numOfNodesX%4==0;
            tileSE_North[i].electronicOutS1 --> tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-i)+i].electronicInN1 if numOfNodesX%4==0;
            tileSE_North[i].electronicInS0 <-- tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-i)+i].electronicOutN0 if numOfNodesX%4==0;
            tileSE_North[i].electronicInS1 <-- tileSW_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-i)+i].electronicOutN1 if numOfNodesX%4==0;

            tileSE_North[i].electronicOutW0 --> tileNE_Empty[(j*((j-i)-1)+i)-1].electronicInE0 if numOfNodesX%4==0;
            tileSE_North[i].electronicOutW1 --> tileNE_Empty[(j*((j-i)-1)+i)-1].electronicInE1 if numOfNodesX%4==0;
            tileSE_North[i].electronicInW0 <-- tileNE_Empty[(j*((j-i)-1)+i)-1].electronicOutE0 if numOfNodesX%4==0;
            tileSE_North[i].electronicInW1 <-- tileNE_Empty[(j*((j-i)-1)+i)-1].electronicOutE1 if numOfNodesX%4==0;

            tileSE_North[i].electronicOutN0 --> tileSW_Empty[j*((j-i)-1)+i].electronicInS0 if numOfNodesX%4==0;
            tileSE_North[i].electronicOutN1 --> tileSW_Empty[j*((j-i)-1)+i].electronicInS1 if numOfNodesX%4==0;
            tileSE_North[i].electronicInN0 <-- tileSW_Empty[j*((j-i)-1)+i].electronicOutS0 if numOfNodesX%4==0;
            tileSE_North[i].electronicInN1 <-- tileSW_Empty[j*((j-i)-1)+i].electronicOutS1 if numOfNodesX%4==0;


            tileNE_North[i].electronicOutE0 --> tileSE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-i)-i].electronicInW0 if numOfNodesX%4==0;
            tileNE_North[i].electronicOutE1 --> tileSE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-i)-i].electronicInW1 if numOfNodesX%4==0;
            tileNE_North[i].electronicInE0 <-- tileSE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-i)-i].electronicOutW0 if numOfNodesX%4==0;
            tileNE_North[i].electronicInE1 <-- tileSE_Empty[(numOfNodesX>>1)*((numOfNodesX>>1)-i)-i].electronicOutW1 if numOfNodesX%4==0;

            tileNE_North[i].electronicOutS0 --> tileNW_Empty[(j*((j-i)+1)-i)-1].electronicInN0 if numOfNodesX%4==0;
            tileNE_North[i].electronicOutS1 --> tileNW_Empty[(j*((j-i)+1)-i)-1].electronicInN1 if numOfNodesX%4==0;
            tileNE_North[i].electronicInS0 <-- tileNW_Empty[(j*((j-i)+1)-i)-1].electronicOutN0 if numOfNodesX%4==0;
            tileNE_North[i].electronicInS1 <-- tileNW_Empty[(j*((j-i)+1)-i)-1].electronicOutN1 if numOfNodesX%4==0;

            tileNE_North[i].electronicOutW0 --> tileSE_Empty[(j*(j-i)-i)-1].electronicInE0 if numOfNodesX%4==0;
            tileNE_North[i].electronicOutW1 --> tileSE_Empty[(j*(j-i)-i)-1].electronicInE1 if numOfNodesX%4==0;
            tileNE_North[i].electronicInW0 <-- tileSE_Empty[(j*(j-i)-i)-1].electronicOutE0 if numOfNodesX%4==0;
            tileNE_North[i].electronicInW1 <-- tileSE_Empty[(j*(j-i)-i)-1].electronicOutE1 if numOfNodesX%4==0;

            tileNE_North[i].electronicOutN0 --> tileNW_Empty[(j*(j-i)-i)-1].electronicInS0 if numOfNodesX%4==0;
            tileNE_North[i].electronicOutN1 --> tileNW_Empty[(j*(j-i)-i)-1].electronicInS1 if numOfNodesX%4==0;
            tileNE_North[i].electronicInN0 <-- tileNW_Empty[(j*(j-i)-i)-1].electronicOutS0 if numOfNodesX%4==0;
            tileNE_North[i].electronicInN1 <-- tileNW_Empty[(j*(j-i)-i)-1].electronicOutS1 if numOfNodesX%4==0;
            
            
           

        }


	// Odd Network dimension case

        for i=0..(numOfNodesX>>2)-1,
        for j = (numOfNodesX>>1)..(numOfNodesX>>1)
        {
            tileSW_South[i].photonicOutE0 <--> tileNW_Empty[((numOfNodesX>>1)*i+i)+1].photonicInW0 if numOfNodesX%4!=0;
            tileSW_South[i].photonicOutE1 <--> tileNW_Empty[((numOfNodesX>>1)*i+i)+1].photonicInW1 if numOfNodesX%4!=0;
            tileSW_South[i].photonicInE0 <--> tileNW_Empty[((numOfNodesX>>1)*i+i)+1].photonicOutW0 if numOfNodesX%4!=0;
            tileSW_South[i].photonicInE1 <--> tileNW_Empty[((numOfNodesX>>1)*i+i)+1].photonicOutW1 if numOfNodesX%4!=0;

            tileSW_South[i].photonicOutS0 <--> tileSE_Empty[(numOfNodesX>>1)*(i+1)+i].photonicInN0 if numOfNodesX%4!=0;
            tileSW_South[i].photonicOutS1 <--> tileSE_Empty[(numOfNodesX>>1)*(i+1)+i].photonicInN1 if numOfNodesX%4!=0;
            tileSW_South[i].photonicInS0 <--> tileSE_Empty[(numOfNodesX>>1)*(i+1)+i].photonicOutN0 if numOfNodesX%4!=0;
            tileSW_South[i].photonicInS1 <--> tileSE_Empty[(numOfNodesX>>1)*(i+1)+i].photonicOutN1 if numOfNodesX%4!=0;

            tileSW_South[i].photonicOutW0 <--> tileNW_Empty[(numOfNodesX>>1)*i+i].photonicInE0 if numOfNodesX%4!=0;
            tileSW_South[i].photonicOutW1 <--> tileNW_Empty[(numOfNodesX>>1)*i+i].photonicInE1 if numOfNodesX%4!=0;
            tileSW_South[i].photonicInW0 <--> tileNW_Empty[(numOfNodesX>>1)*i+i].photonicOutE0 if numOfNodesX%4!=0;
            tileSW_South[i].photonicInW1 <--> tileNW_Empty[(numOfNodesX>>1)*i+i].photonicOutE1 if numOfNodesX%4!=0;

            tileSW_South[i].photonicOutN0 <--> tileSE_Empty[(numOfNodesX>>1)*i+i].photonicInS0 if numOfNodesX%4!=0;
            tileSW_South[i].photonicOutN1 <--> tileSE_Empty[(numOfNodesX>>1)*i+i].photonicInS1 if numOfNodesX%4!=0;
            tileSW_South[i].photonicInN0 <--> tileSE_Empty[(numOfNodesX>>1)*i+i].photonicOutS0 if numOfNodesX%4!=0;
            tileSW_South[i].photonicInN1 <--> tileSE_Empty[(numOfNodesX>>1)*i+i].photonicOutS1 if numOfNodesX%4!=0;



            tileNW_South[i].photonicOutE0 <--> tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-1].photonicInW0 if numOfNodesX%4!=0;
            tileNW_South[i].photonicOutE1 <--> tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-1].photonicInW1 if numOfNodesX%4!=0;
            tileNW_South[i].photonicInE0 <--> tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-1].photonicOutW0 if numOfNodesX%4!=0;
            tileNW_South[i].photonicInE1 <--> tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-1].photonicOutW1 if numOfNodesX%4!=0;

            tileNW_South[i].photonicOutS0 <--> tileNE_Empty[((numOfNodesX>>1)*(i+2)-i)-1].photonicInN0 if numOfNodesX%4!=0;
            tileNW_South[i].photonicOutS1 <--> tileNE_Empty[((numOfNodesX>>1)*(i+2)-i)-1].photonicInN1 if numOfNodesX%4!=0;
            tileNW_South[i].photonicInS0 <--> tileNE_Empty[((numOfNodesX>>1)*(i+2)-i)-1].photonicOutN0 if numOfNodesX%4!=0;
            tileNW_South[i].photonicInS1 <--> tileNE_Empty[((numOfNodesX>>1)*(i+2)-i)-1].photonicOutN1 if numOfNodesX%4!=0;

            tileNW_South[i].photonicOutW0 <--> tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-2].photonicInE0 if numOfNodesX%4!=0;
            tileNW_South[i].photonicOutW1 <--> tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-2].photonicInE1 if numOfNodesX%4!=0;
            tileNW_South[i].photonicInW0 <--> tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-2].photonicOutE0 if numOfNodesX%4!=0;
            tileNW_South[i].photonicInW1 <--> tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-2].photonicOutE1 if numOfNodesX%4!=0;

            tileNW_South[i].photonicOutN0 <--> tileNE_Empty[((numOfNodesX>>1)*(i+1)-i)-1].photonicInS0 if numOfNodesX%4!=0;
            tileNW_South[i].photonicOutN1 <--> tileNE_Empty[((numOfNodesX>>1)*(i+1)-i)-1].photonicInS1 if numOfNodesX%4!=0;
            tileNW_South[i].photonicInN0 <--> tileNE_Empty[((numOfNodesX>>1)*(i+1)-i)-1].photonicOutS0 if numOfNodesX%4!=0;
            tileNW_South[i].photonicInN1 <--> tileNE_Empty[((numOfNodesX>>1)*(i+1)-i)-1].photonicOutS1 if numOfNodesX%4!=0;


            tileSE_North[i].photonicOutE0 <--> tileNE_Empty[(j*((j-i)-1)+i)+1].photonicInW0 if numOfNodesX%4!=0;
            tileSE_North[i].photonicOutE1 <--> tileNE_Empty[(j*((j-i)-1)+i)+1].photonicInW1 if numOfNodesX%4!=0;
            tileSE_North[i].photonicInE0 <--> tileNE_Empty[(j*((j-i)-1)+i)+1].photonicOutW0 if numOfNodesX%4!=0;
            tileSE_North[i].photonicInE1 <--> tileNE_Empty[(j*((j-i)-1)+i)+1].photonicOutW1 if numOfNodesX%4!=0;

            tileSE_North[i].photonicOutS0 <--> tileSW_Empty[j*((j-i)-1)+i].photonicInN0 if numOfNodesX%4!=0;
            tileSE_North[i].photonicOutS1 <--> tileSW_Empty[j*((j-i)-1)+i].photonicInN1 if numOfNodesX%4!=0;
            tileSE_North[i].photonicInS0 <--> tileSW_Empty[j*((j-i)-1)+i].photonicOutN0 if numOfNodesX%4!=0;
            tileSE_North[i].photonicInS1 <--> tileSW_Empty[j*((j-i)-1)+i].photonicOutN1 if numOfNodesX%4!=0;

            tileSE_North[i].photonicOutW0 <--> tileNE_Empty[j*((j-i)-1)+i].photonicInE0 if numOfNodesX%4!=0;
            tileSE_North[i].photonicOutW1 <--> tileNE_Empty[j*((j-i)-1)+i].photonicInE1 if numOfNodesX%4!=0;
            tileSE_North[i].photonicInW0 <--> tileNE_Empty[j*((j-i)-1)+i].photonicOutE0 if numOfNodesX%4!=0;
            tileSE_North[i].photonicInW1 <--> tileNE_Empty[j*((j-i)-1)+i].photonicOutE1 if numOfNodesX%4!=0;

            tileSE_North[i].photonicOutN0 <--> tileSW_Empty[j*((j-i)-2)+i].photonicInS0 if numOfNodesX%4!=0;
            tileSE_North[i].photonicOutN1 <--> tileSW_Empty[j*((j-i)-2)+i].photonicInS1 if numOfNodesX%4!=0;
            tileSE_North[i].photonicInN0 <--> tileSW_Empty[j*((j-i)-2)+i].photonicOutS0 if numOfNodesX%4!=0;
            tileSE_North[i].photonicInN1 <--> tileSW_Empty[j*((j-i)-2)+i].photonicOutS1 if numOfNodesX%4!=0;


            tileNE_North[i].photonicOutE0 <--> tileSE_Empty[(j*(j-i)-i)-1].photonicInW0 if numOfNodesX%4!=0;
            tileNE_North[i].photonicOutE1 <--> tileSE_Empty[(j*(j-i)-i)-1].photonicInW1 if numOfNodesX%4!=0;
            tileNE_North[i].photonicInE0 <--> tileSE_Empty[(j*(j-i)-i)-1].photonicOutW0 if numOfNodesX%4!=0;
            tileNE_North[i].photonicInE1 <--> tileSE_Empty[(j*(j-i)-i)-1].photonicOutW1 if numOfNodesX%4!=0;

            tileNE_North[i].photonicOutS0 <--> tileNW_Empty[(j*(j-i)-i)-1].photonicInN0 if numOfNodesX%4!=0;
            tileNE_North[i].photonicOutS1 <--> tileNW_Empty[(j*(j-i)-i)-1].photonicInN1 if numOfNodesX%4!=0;
            tileNE_North[i].photonicInS0 <--> tileNW_Empty[(j*(j-i)-i)-1].photonicOutN0 if numOfNodesX%4!=0;
            tileNE_North[i].photonicInS1 <--> tileNW_Empty[(j*(j-i)-i)-1].photonicOutN1 if numOfNodesX%4!=0;

            tileNE_North[i].photonicOutW0 <--> tileSE_Empty[(j*(j-i)-i)-2].photonicInE0 if numOfNodesX%4!=0;
            tileNE_North[i].photonicOutW1 <--> tileSE_Empty[(j*(j-i)-i)-2].photonicInE1 if numOfNodesX%4!=0;
            tileNE_North[i].photonicInW0 <--> tileSE_Empty[(j*(j-i)-i)-2].photonicOutE0 if numOfNodesX%4!=0;
            tileNE_North[i].photonicInW1 <--> tileSE_Empty[(j*(j-i)-i)-2].photonicOutE1 if numOfNodesX%4!=0;

            tileNE_North[i].photonicOutN0 <--> tileNW_Empty[(j*((j-i)-1)-i)-1].photonicInS0 if numOfNodesX%4!=0;
            tileNE_North[i].photonicOutN1 <--> tileNW_Empty[(j*((j-i)-1)-i)-1].photonicInS1 if numOfNodesX%4!=0;
            tileNE_North[i].photonicInN0 <--> tileNW_Empty[(j*((j-i)-1)-i)-1].photonicOutS0 if numOfNodesX%4!=0;
            tileNE_North[i].photonicInN1 <--> tileNW_Empty[(j*((j-i)-1)-i)-1].photonicOutS1 if numOfNodesX%4!=0;

			///
			
			tileSW_South[i].electronicOutE0 --> tileNW_Empty[((numOfNodesX>>1)*i+i)+1].electronicInW0 if numOfNodesX%4!=0;
            tileSW_South[i].electronicOutE1 --> tileNW_Empty[((numOfNodesX>>1)*i+i)+1].electronicInW1 if numOfNodesX%4!=0;
            tileSW_South[i].electronicInE0 <-- tileNW_Empty[((numOfNodesX>>1)*i+i)+1].electronicOutW0 if numOfNodesX%4!=0;
            tileSW_South[i].electronicInE1 <-- tileNW_Empty[((numOfNodesX>>1)*i+i)+1].electronicOutW1 if numOfNodesX%4!=0;

            tileSW_South[i].electronicOutS0 --> tileSE_Empty[(numOfNodesX>>1)*(i+1)+i].electronicInN0 if numOfNodesX%4!=0;
            tileSW_South[i].electronicOutS1 --> tileSE_Empty[(numOfNodesX>>1)*(i+1)+i].electronicInN1 if numOfNodesX%4!=0;
            tileSW_South[i].electronicInS0 <-- tileSE_Empty[(numOfNodesX>>1)*(i+1)+i].electronicOutN0 if numOfNodesX%4!=0;
            tileSW_South[i].electronicInS1 <-- tileSE_Empty[(numOfNodesX>>1)*(i+1)+i].electronicOutN1 if numOfNodesX%4!=0;

            tileSW_South[i].electronicOutW0 --> tileNW_Empty[(numOfNodesX>>1)*i+i].electronicInE0 if numOfNodesX%4!=0;
            tileSW_South[i].electronicOutW1 --> tileNW_Empty[(numOfNodesX>>1)*i+i].electronicInE1 if numOfNodesX%4!=0;
            tileSW_South[i].electronicInW0 <-- tileNW_Empty[(numOfNodesX>>1)*i+i].electronicOutE0 if numOfNodesX%4!=0;
            tileSW_South[i].electronicInW1 <-- tileNW_Empty[(numOfNodesX>>1)*i+i].electronicOutE1 if numOfNodesX%4!=0;

            tileSW_South[i].electronicOutN0 --> tileSE_Empty[(numOfNodesX>>1)*i+i].electronicInS0 if numOfNodesX%4!=0;
            tileSW_South[i].electronicOutN1 --> tileSE_Empty[(numOfNodesX>>1)*i+i].electronicInS1 if numOfNodesX%4!=0;
            tileSW_South[i].electronicInN0 <-- tileSE_Empty[(numOfNodesX>>1)*i+i].electronicOutS0 if numOfNodesX%4!=0;
            tileSW_South[i].electronicInN1 <-- tileSE_Empty[(numOfNodesX>>1)*i+i].electronicOutS1 if numOfNodesX%4!=0;



            tileNW_South[i].electronicOutE0 --> tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-1].electronicInW0 if numOfNodesX%4!=0;
            tileNW_South[i].electronicOutE1 --> tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-1].electronicInW1 if numOfNodesX%4!=0;
            tileNW_South[i].electronicInE0 <-- tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-1].electronicOutW0 if numOfNodesX%4!=0;
            tileNW_South[i].electronicInE1 <-- tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-1].electronicOutW1 if numOfNodesX%4!=0;

            tileNW_South[i].electronicOutS0 --> tileNE_Empty[((numOfNodesX>>1)*(i+2)-i)-1].electronicInN0 if numOfNodesX%4!=0;
            tileNW_South[i].electronicOutS1 --> tileNE_Empty[((numOfNodesX>>1)*(i+2)-i)-1].electronicInN1 if numOfNodesX%4!=0;
            tileNW_South[i].electronicInS0 <-- tileNE_Empty[((numOfNodesX>>1)*(i+2)-i)-1].electronicOutN0 if numOfNodesX%4!=0;
            tileNW_South[i].electronicInS1 <-- tileNE_Empty[((numOfNodesX>>1)*(i+2)-i)-1].electronicOutN1 if numOfNodesX%4!=0;

            tileNW_South[i].electronicOutW0 --> tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-2].electronicInE0 if numOfNodesX%4!=0;
            tileNW_South[i].electronicOutW1 --> tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-2].electronicInE1 if numOfNodesX%4!=0;
            tileNW_South[i].electronicInW0 <-- tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-2].electronicOutE0 if numOfNodesX%4!=0;
            tileNW_South[i].electronicInW1 <-- tileSW_Empty[((numOfNodesX>>1)*(i+1)-i)-2].electronicOutE1 if numOfNodesX%4!=0;

            tileNW_South[i].electronicOutN0 --> tileNE_Empty[((numOfNodesX>>1)*(i+1)-i)-1].electronicInS0 if numOfNodesX%4!=0;
            tileNW_South[i].electronicOutN1 --> tileNE_Empty[((numOfNodesX>>1)*(i+1)-i)-1].electronicInS1 if numOfNodesX%4!=0;
            tileNW_South[i].electronicInN0 <-- tileNE_Empty[((numOfNodesX>>1)*(i+1)-i)-1].electronicOutS0 if numOfNodesX%4!=0;
            tileNW_South[i].electronicInN1 <-- tileNE_Empty[((numOfNodesX>>1)*(i+1)-i)-1].electronicOutS1 if numOfNodesX%4!=0;


            tileSE_North[i].electronicOutE0 --> tileNE_Empty[(j*((j-i)-1)+i)+1].electronicInW0 if numOfNodesX%4!=0;
            tileSE_North[i].electronicOutE1 --> tileNE_Empty[(j*((j-i)-1)+i)+1].electronicInW1 if numOfNodesX%4!=0;
            tileSE_North[i].electronicInE0 <-- tileNE_Empty[(j*((j-i)-1)+i)+1].electronicOutW0 if numOfNodesX%4!=0;
            tileSE_North[i].electronicInE1 <-- tileNE_Empty[(j*((j-i)-1)+i)+1].electronicOutW1 if numOfNodesX%4!=0;

            tileSE_North[i].electronicOutS0 --> tileSW_Empty[j*((j-i)-1)+i].electronicInN0 if numOfNodesX%4!=0;
            tileSE_North[i].electronicOutS1 --> tileSW_Empty[j*((j-i)-1)+i].electronicInN1 if numOfNodesX%4!=0;
            tileSE_North[i].electronicInS0 <-- tileSW_Empty[j*((j-i)-1)+i].electronicOutN0 if numOfNodesX%4!=0;
            tileSE_North[i].electronicInS1 <-- tileSW_Empty[j*((j-i)-1)+i].electronicOutN1 if numOfNodesX%4!=0;

            tileSE_North[i].electronicOutW0 --> tileNE_Empty[j*((j-i)-1)+i].electronicInE0 if numOfNodesX%4!=0;
            tileSE_North[i].electronicOutW1 --> tileNE_Empty[j*((j-i)-1)+i].electronicInE1 if numOfNodesX%4!=0;
            tileSE_North[i].electronicInW0 <-- tileNE_Empty[j*((j-i)-1)+i].electronicOutE0 if numOfNodesX%4!=0;
            tileSE_North[i].electronicInW1 <-- tileNE_Empty[j*((j-i)-1)+i].electronicOutE1 if numOfNodesX%4!=0;

            tileSE_North[i].electronicOutN0 --> tileSW_Empty[j*((j-i)-2)+i].electronicInS0 if numOfNodesX%4!=0;
            tileSE_North[i].electronicOutN1 --> tileSW_Empty[j*((j-i)-2)+i].electronicInS1 if numOfNodesX%4!=0;
            tileSE_North[i].electronicInN0 <-- tileSW_Empty[j*((j-i)-2)+i].electronicOutS0 if numOfNodesX%4!=0;
            tileSE_North[i].electronicInN1 <-- tileSW_Empty[j*((j-i)-2)+i].electronicOutS1 if numOfNodesX%4!=0;


            tileNE_North[i].electronicOutE0 --> tileSE_Empty[(j*(j-i)-i)-1].electronicInW0 if numOfNodesX%4!=0;
            tileNE_North[i].electronicOutE1 --> tileSE_Empty[(j*(j-i)-i)-1].electronicInW1 if numOfNodesX%4!=0;
            tileNE_North[i].electronicInE0 <-- tileSE_Empty[(j*(j-i)-i)-1].electronicOutW0 if numOfNodesX%4!=0;
            tileNE_North[i].electronicInE1 <-- tileSE_Empty[(j*(j-i)-i)-1].electronicOutW1 if numOfNodesX%4!=0;

            tileNE_North[i].electronicOutS0 --> tileNW_Empty[(j*(j-i)-i)-1].electronicInN0 if numOfNodesX%4!=0;
            tileNE_North[i].electronicOutS1 --> tileNW_Empty[(j*(j-i)-i)-1].electronicInN1 if numOfNodesX%4!=0;
            tileNE_North[i].electronicInS0 <-- tileNW_Empty[(j*(j-i)-i)-1].electronicOutN0 if numOfNodesX%4!=0;
            tileNE_North[i].electronicInS1 <-- tileNW_Empty[(j*(j-i)-i)-1].electronicOutN1 if numOfNodesX%4!=0;

            tileNE_North[i].electronicOutW0 --> tileSE_Empty[(j*(j-i)-i)-2].electronicInE0 if numOfNodesX%4!=0;
            tileNE_North[i].electronicOutW1 --> tileSE_Empty[(j*(j-i)-i)-2].electronicInE1 if numOfNodesX%4!=0;
            tileNE_North[i].electronicInW0 <-- tileSE_Empty[(j*(j-i)-i)-2].electronicOutE0 if numOfNodesX%4!=0;
            tileNE_North[i].electronicInW1 <-- tileSE_Empty[(j*(j-i)-i)-2].electronicOutE1 if numOfNodesX%4!=0;

            tileNE_North[i].electronicOutN0 --> tileNW_Empty[(j*((j-i)-1)-i)-1].electronicInS0 if numOfNodesX%4!=0;
            tileNE_North[i].electronicOutN1 --> tileNW_Empty[(j*((j-i)-1)-i)-1].electronicInS1 if numOfNodesX%4!=0;
            tileNE_North[i].electronicInN0 <-- tileNW_Empty[(j*((j-i)-1)-i)-1].electronicOutS0 if numOfNodesX%4!=0;
            tileNE_North[i].electronicInN1 <-- tileNW_Empty[(j*((j-i)-1)-i)-1].electronicOutS1 if numOfNodesX%4!=0;
			
			
			
			
        }



	//The other nodes from the diagonal of type that are found in center
		// Even Network dimension case

        for i=0..(numOfNodesX>>2)-2,
        for j = (numOfNodesX>>1)..(numOfNodesX>>1) 
        {
            tileNE_South[i].photonicOutE0 <--> tileSE_Empty[(j*i+i)+1].photonicInW0 if numOfNodesX%4==0;
            tileNE_South[i].photonicOutE1 <--> tileSE_Empty[(j*i+i)+1].photonicInW1 if numOfNodesX%4==0;
            tileNE_South[i].photonicInE0 <--> tileSE_Empty[(j*i+i)+1].photonicOutW0 if numOfNodesX%4==0;
            tileNE_South[i].photonicInE1 <--> tileSE_Empty[(j*i+i)+1].photonicOutW1 if numOfNodesX%4==0;

            tileNE_South[i].photonicOutS0 <--> tileNW_Empty[j*(i+1)+i].photonicInN0 if numOfNodesX%4==0;
            tileNE_South[i].photonicOutS1 <--> tileNW_Empty[j*(i+1)+i].photonicInN1 if numOfNodesX%4==0;
            tileNE_South[i].photonicInS0 <--> tileNW_Empty[j*(i+1)+i].photonicOutN0 if numOfNodesX%4==0;
            tileNE_South[i].photonicInS1 <--> tileNW_Empty[j*(i+1)+i].photonicOutN1 if numOfNodesX%4==0;

            tileNE_South[i].photonicOutW0 <--> tileSE_Empty[j*i+i].photonicInE0 if numOfNodesX%4==0;
            tileNE_South[i].photonicOutW1 <--> tileSE_Empty[j*i+i].photonicInE1 if numOfNodesX%4==0;
            tileNE_South[i].photonicInW0 <--> tileSE_Empty[j*i+i].photonicOutE0 if numOfNodesX%4==0;
            tileNE_South[i].photonicInW1 <--> tileSE_Empty[j*i+i].photonicOutE1 if numOfNodesX%4==0;

            tileNE_South[i].photonicOutN0 <--> tileNW_Empty[j*i+i].photonicInS0 if numOfNodesX%4==0;
            tileNE_South[i].photonicOutN1 <--> tileNW_Empty[j*i+i].photonicInS1 if numOfNodesX%4==0;
            tileNE_South[i].photonicInN0 <--> tileNW_Empty[j*i+i].photonicOutS0 if numOfNodesX%4==0;
            tileNE_South[i].photonicInN1 <--> tileNW_Empty[j*i+i].photonicOutS1 if numOfNodesX%4==0;



            tileSE_South[i].photonicOutE0 <--> tileNE_Empty[(j*(i+1)-i)-1].photonicInW0 if numOfNodesX%4==0;
            tileSE_South[i].photonicOutE1 <--> tileNE_Empty[(j*(i+1)-i)-1].photonicInW1 if numOfNodesX%4==0;
            tileSE_South[i].photonicInE0 <--> tileNE_Empty[(j*(i+1)-i)-1].photonicOutW0 if numOfNodesX%4==0;
            tileSE_South[i].photonicInE1 <--> tileNE_Empty[(j*(i+1)-i)-1].photonicOutW1 if numOfNodesX%4==0;

            tileSE_South[i].photonicOutS0 <--> tileSW_Empty[(j*(i+2)-i)-1].photonicInN0 if numOfNodesX%4==0;
            tileSE_South[i].photonicOutS1 <--> tileSW_Empty[(j*(i+2)-i)-1].photonicInN1 if numOfNodesX%4==0;
            tileSE_South[i].photonicInS0 <--> tileSW_Empty[(j*(i+2)-i)-1].photonicOutN0 if numOfNodesX%4==0;
            tileSE_South[i].photonicInS1 <--> tileSW_Empty[(j*(i+2)-i)-1].photonicOutN1 if numOfNodesX%4==0;

            tileSE_South[i].photonicOutW0 <--> tileNE_Empty[(j*(i+1)-i)-2].photonicInE0 if numOfNodesX%4==0;
            tileSE_South[i].photonicOutW1 <--> tileNE_Empty[(j*(i+1)-i)-2].photonicInE1 if numOfNodesX%4==0;
            tileSE_South[i].photonicInW0 <--> tileNE_Empty[(j*(i+1)-i)-2].photonicOutE0 if numOfNodesX%4==0;
            tileSE_South[i].photonicInW1 <--> tileNE_Empty[(j*(i+1)-i)-2].photonicOutE1 if numOfNodesX%4==0;

            tileSE_South[i].photonicOutN0 <--> tileSW_Empty[(j*(i+1)-i)-1].photonicInS0 if numOfNodesX%4==0;
            tileSE_South[i].photonicOutN1 <--> tileSW_Empty[(j*(i+1)-i)-1].photonicInS1 if numOfNodesX%4==0;
            tileSE_South[i].photonicInN0 <--> tileSW_Empty[(j*(i+1)-i)-1].photonicOutS0 if numOfNodesX%4==0;
            tileSE_South[i].photonicInN1 <--> tileSW_Empty[(j*(i+1)-i)-1].photonicOutS1 if numOfNodesX%4==0;


            tileNW_North[i].photonicOutE0 <--> tileSW_Empty[(j*((j-i)-1)+i)+1].photonicInW0 if numOfNodesX%4==0;
            tileNW_North[i].photonicOutE1 <--> tileSW_Empty[(j*((j-i)-1)+i)+1].photonicInW1 if numOfNodesX%4==0;
            tileNW_North[i].photonicInE0 <--> tileSW_Empty[(j*((j-i)-1)+i)+1].photonicOutW0 if numOfNodesX%4==0;
            tileNW_North[i].photonicInE1 <--> tileSW_Empty[(j*((j-i)-1)+i)+1].photonicOutW1 if numOfNodesX%4==0;

            tileNW_North[i].photonicOutS0 <--> tileNE_Empty[j*((j-i)-1)+i].photonicInN0 if numOfNodesX%4==0;
            tileNW_North[i].photonicOutS1 <--> tileNE_Empty[j*((j-i)-1)+i].photonicInN1 if numOfNodesX%4==0;
            tileNW_North[i].photonicInS0 <--> tileNE_Empty[j*((j-i)-1)+i].photonicOutN0 if numOfNodesX%4==0;
            tileNW_North[i].photonicInS1 <--> tileNE_Empty[j*((j-i)-1)+i].photonicOutN1 if numOfNodesX%4==0;

            tileNW_North[i].photonicOutW0 <--> tileSW_Empty[j*((j-i)-1)+i].photonicInE0 if numOfNodesX%4==0;
            tileNW_North[i].photonicOutW1 <--> tileSW_Empty[j*((j-i)-1)+i].photonicInE1 if numOfNodesX%4==0;
            tileNW_North[i].photonicInW0 <--> tileSW_Empty[j*((j-i)-1)+i].photonicOutE0 if numOfNodesX%4==0;
            tileNW_North[i].photonicInW1 <--> tileSW_Empty[j*((j-i)-1)+i].photonicOutE1 if numOfNodesX%4==0;

            tileNW_North[i].photonicOutN0 <--> tileNE_Empty[j*((j-i)-2)+i].photonicInS0 if numOfNodesX%4==0;
            tileNW_North[i].photonicOutN1 <--> tileNE_Empty[j*((j-i)-2)+i].photonicInS1 if numOfNodesX%4==0;
            tileNW_North[i].photonicInN0 <--> tileNE_Empty[j*((j-i)-2)+i].photonicOutS0 if numOfNodesX%4==0;
            tileNW_North[i].photonicInN1 <--> tileNE_Empty[j*((j-i)-2)+i].photonicOutS1 if numOfNodesX%4==0;


            tileSW_North[i].photonicOutE0 <--> tileNW_Empty[(j*(j-i)-i)-1].photonicInW0 if numOfNodesX%4==0;
            tileSW_North[i].photonicOutE1 <--> tileNW_Empty[(j*(j-i)-i)-1].photonicInW1 if numOfNodesX%4==0;
            tileSW_North[i].photonicInE0 <--> tileNW_Empty[(j*(j-i)-i)-1].photonicOutW0 if numOfNodesX%4==0;
            tileSW_North[i].photonicInE1 <--> tileNW_Empty[(j*(j-i)-i)-1].photonicOutW1 if numOfNodesX%4==0;

            tileSW_North[i].photonicOutS0 <--> tileSE_Empty[(j*(j-i)-i)-1].photonicInN0 if numOfNodesX%4==0;
            tileSW_North[i].photonicOutS1 <--> tileSE_Empty[(j*(j-i)-i)-1].photonicInN1 if numOfNodesX%4==0;
            tileSW_North[i].photonicInS0 <--> tileSE_Empty[(j*(j-i)-i)-1].photonicOutN0 if numOfNodesX%4==0;
            tileSW_North[i].photonicInS1 <--> tileSE_Empty[(j*(j-i)-i)-1].photonicOutN1 if numOfNodesX%4==0;

            tileSW_North[i].photonicOutW0 <--> tileNW_Empty[(j*(j-i)-i)-2].photonicInE0 if numOfNodesX%4==0;
            tileSW_North[i].photonicOutW1 <--> tileNW_Empty[(j*(j-i)-i)-2].photonicInE1 if numOfNodesX%4==0;
            tileSW_North[i].photonicInW0 <--> tileNW_Empty[(j*(j-i)-i)-2].photonicOutE0 if numOfNodesX%4==0;
            tileSW_North[i].photonicInW1 <--> tileNW_Empty[(j*(j-i)-i)-2].photonicOutE1 if numOfNodesX%4==0;

            tileSW_North[i].photonicOutN0 <--> tileSE_Empty[(j*((j-i)-1)-i)-1].photonicInS0 if numOfNodesX%4==0;
            tileSW_North[i].photonicOutN1 <--> tileSE_Empty[(j*((j-i)-1)-i)-1].photonicInS1 if numOfNodesX%4==0;
            tileSW_North[i].photonicInN0 <--> tileSE_Empty[(j*((j-i)-1)-i)-1].photonicOutS0 if numOfNodesX%4==0;
            tileSW_North[i].photonicInN1 <--> tileSE_Empty[(j*((j-i)-1)-i)-1].photonicOutS1 if numOfNodesX%4==0;


			//
			
			tileNE_South[i].electronicOutE0 --> tileSE_Empty[(j*i+i)+1].electronicInW0 if numOfNodesX%4==0;
            tileNE_South[i].electronicOutE1 --> tileSE_Empty[(j*i+i)+1].electronicInW1 if numOfNodesX%4==0;
            tileNE_South[i].electronicInE0 <-- tileSE_Empty[(j*i+i)+1].electronicOutW0 if numOfNodesX%4==0;
            tileNE_South[i].electronicInE1 <-- tileSE_Empty[(j*i+i)+1].electronicOutW1 if numOfNodesX%4==0;

            tileNE_South[i].electronicOutS0 --> tileNW_Empty[j*(i+1)+i].electronicInN0 if numOfNodesX%4==0;
            tileNE_South[i].electronicOutS1 --> tileNW_Empty[j*(i+1)+i].electronicInN1 if numOfNodesX%4==0;
            tileNE_South[i].electronicInS0 <-- tileNW_Empty[j*(i+1)+i].electronicOutN0 if numOfNodesX%4==0;
            tileNE_South[i].electronicInS1 <-- tileNW_Empty[j*(i+1)+i].electronicOutN1 if numOfNodesX%4==0;

            tileNE_South[i].electronicOutW0 --> tileSE_Empty[j*i+i].electronicInE0 if numOfNodesX%4==0;
            tileNE_South[i].electronicOutW1 --> tileSE_Empty[j*i+i].electronicInE1 if numOfNodesX%4==0;
            tileNE_South[i].electronicInW0 <-- tileSE_Empty[j*i+i].electronicOutE0 if numOfNodesX%4==0;
            tileNE_South[i].electronicInW1 <-- tileSE_Empty[j*i+i].electronicOutE1 if numOfNodesX%4==0;

            tileNE_South[i].electronicOutN0 --> tileNW_Empty[j*i+i].electronicInS0 if numOfNodesX%4==0;
            tileNE_South[i].electronicOutN1 --> tileNW_Empty[j*i+i].electronicInS1 if numOfNodesX%4==0;
            tileNE_South[i].electronicInN0 <-- tileNW_Empty[j*i+i].electronicOutS0 if numOfNodesX%4==0;
            tileNE_South[i].electronicInN1 <-- tileNW_Empty[j*i+i].electronicOutS1 if numOfNodesX%4==0;



            tileSE_South[i].electronicOutE0 --> tileNE_Empty[(j*(i+1)-i)-1].electronicInW0 if numOfNodesX%4==0;
            tileSE_South[i].electronicOutE1 --> tileNE_Empty[(j*(i+1)-i)-1].electronicInW1 if numOfNodesX%4==0;
            tileSE_South[i].electronicInE0 <-- tileNE_Empty[(j*(i+1)-i)-1].electronicOutW0 if numOfNodesX%4==0;
            tileSE_South[i].electronicInE1 <-- tileNE_Empty[(j*(i+1)-i)-1].electronicOutW1 if numOfNodesX%4==0;

            tileSE_South[i].electronicOutS0 --> tileSW_Empty[(j*(i+2)-i)-1].electronicInN0 if numOfNodesX%4==0;
            tileSE_South[i].electronicOutS1 --> tileSW_Empty[(j*(i+2)-i)-1].electronicInN1 if numOfNodesX%4==0;
            tileSE_South[i].electronicInS0 <-- tileSW_Empty[(j*(i+2)-i)-1].electronicOutN0 if numOfNodesX%4==0;
            tileSE_South[i].electronicInS1 <-- tileSW_Empty[(j*(i+2)-i)-1].electronicOutN1 if numOfNodesX%4==0;

            tileSE_South[i].electronicOutW0 --> tileNE_Empty[(j*(i+1)-i)-2].electronicInE0 if numOfNodesX%4==0;
            tileSE_South[i].electronicOutW1 --> tileNE_Empty[(j*(i+1)-i)-2].electronicInE1 if numOfNodesX%4==0;
            tileSE_South[i].electronicInW0 <-- tileNE_Empty[(j*(i+1)-i)-2].electronicOutE0 if numOfNodesX%4==0;
            tileSE_South[i].electronicInW1 <-- tileNE_Empty[(j*(i+1)-i)-2].electronicOutE1 if numOfNodesX%4==0;

            tileSE_South[i].electronicOutN0 --> tileSW_Empty[(j*(i+1)-i)-1].electronicInS0 if numOfNodesX%4==0;
            tileSE_South[i].electronicOutN1 --> tileSW_Empty[(j*(i+1)-i)-1].electronicInS1 if numOfNodesX%4==0;
            tileSE_South[i].electronicInN0 <-- tileSW_Empty[(j*(i+1)-i)-1].electronicOutS0 if numOfNodesX%4==0;
            tileSE_South[i].electronicInN1 <-- tileSW_Empty[(j*(i+1)-i)-1].electronicOutS1 if numOfNodesX%4==0;


            tileNW_North[i].electronicOutE0 --> tileSW_Empty[(j*((j-i)-1)+i)+1].electronicInW0 if numOfNodesX%4==0;
            tileNW_North[i].electronicOutE1 --> tileSW_Empty[(j*((j-i)-1)+i)+1].electronicInW1 if numOfNodesX%4==0;
            tileNW_North[i].electronicInE0 <-- tileSW_Empty[(j*((j-i)-1)+i)+1].electronicOutW0 if numOfNodesX%4==0;
            tileNW_North[i].electronicInE1 <-- tileSW_Empty[(j*((j-i)-1)+i)+1].electronicOutW1 if numOfNodesX%4==0;

            tileNW_North[i].electronicOutS0 --> tileNE_Empty[j*((j-i)-1)+i].electronicInN0 if numOfNodesX%4==0;
            tileNW_North[i].electronicOutS1 --> tileNE_Empty[j*((j-i)-1)+i].electronicInN1 if numOfNodesX%4==0;
            tileNW_North[i].electronicInS0 <-- tileNE_Empty[j*((j-i)-1)+i].electronicOutN0 if numOfNodesX%4==0;
            tileNW_North[i].electronicInS1 <-- tileNE_Empty[j*((j-i)-1)+i].electronicOutN1 if numOfNodesX%4==0;

            tileNW_North[i].electronicOutW0 --> tileSW_Empty[j*((j-i)-1)+i].electronicInE0 if numOfNodesX%4==0;
            tileNW_North[i].electronicOutW1 --> tileSW_Empty[j*((j-i)-1)+i].electronicInE1 if numOfNodesX%4==0;
            tileNW_North[i].electronicInW0 <-- tileSW_Empty[j*((j-i)-1)+i].electronicOutE0 if numOfNodesX%4==0;
            tileNW_North[i].electronicInW1 <-- tileSW_Empty[j*((j-i)-1)+i].electronicOutE1 if numOfNodesX%4==0;

            tileNW_North[i].electronicOutN0 --> tileNE_Empty[j*((j-i)-2)+i].electronicInS0 if numOfNodesX%4==0;
            tileNW_North[i].electronicOutN1 --> tileNE_Empty[j*((j-i)-2)+i].electronicInS1 if numOfNodesX%4==0;
            tileNW_North[i].electronicInN0 <-- tileNE_Empty[j*((j-i)-2)+i].electronicOutS0 if numOfNodesX%4==0;
            tileNW_North[i].electronicInN1 <-- tileNE_Empty[j*((j-i)-2)+i].electronicOutS1 if numOfNodesX%4==0;


            tileSW_North[i].electronicOutE0 --> tileNW_Empty[(j*(j-i)-i)-1].electronicInW0 if numOfNodesX%4==0;
            tileSW_North[i].electronicOutE1 --> tileNW_Empty[(j*(j-i)-i)-1].electronicInW1 if numOfNodesX%4==0;
            tileSW_North[i].electronicInE0 <-- tileNW_Empty[(j*(j-i)-i)-1].electronicOutW0 if numOfNodesX%4==0;
            tileSW_North[i].electronicInE1 <-- tileNW_Empty[(j*(j-i)-i)-1].electronicOutW1 if numOfNodesX%4==0;

            tileSW_North[i].electronicOutS0 --> tileSE_Empty[(j*(j-i)-i)-1].electronicInN0 if numOfNodesX%4==0;
            tileSW_North[i].electronicOutS1 --> tileSE_Empty[(j*(j-i)-i)-1].electronicInN1 if numOfNodesX%4==0;
            tileSW_North[i].electronicInS0 <-- tileSE_Empty[(j*(j-i)-i)-1].electronicOutN0 if numOfNodesX%4==0;
            tileSW_North[i].electronicInS1 <-- tileSE_Empty[(j*(j-i)-i)-1].electronicOutN1 if numOfNodesX%4==0;

            tileSW_North[i].electronicOutW0 --> tileNW_Empty[(j*(j-i)-i)-2].electronicInE0 if numOfNodesX%4==0;
            tileSW_North[i].electronicOutW1 --> tileNW_Empty[(j*(j-i)-i)-2].electronicInE1 if numOfNodesX%4==0;
            tileSW_North[i].electronicInW0 <-- tileNW_Empty[(j*(j-i)-i)-2].electronicOutE0 if numOfNodesX%4==0;
            tileSW_North[i].electronicInW1 <-- tileNW_Empty[(j*(j-i)-i)-2].electronicOutE1 if numOfNodesX%4==0;

            tileSW_North[i].electronicOutN0 --> tileSE_Empty[(j*((j-i)-1)-i)-1].electronicInS0 if numOfNodesX%4==0;
            tileSW_North[i].electronicOutN1 --> tileSE_Empty[(j*((j-i)-1)-i)-1].electronicInS1 if numOfNodesX%4==0;
            tileSW_North[i].electronicInN0 <-- tileSE_Empty[(j*((j-i)-1)-i)-1].electronicOutS0 if numOfNodesX%4==0;
            tileSW_North[i].electronicInN1 <-- tileSE_Empty[(j*((j-i)-1)-i)-1].electronicOutS1 if numOfNodesX%4==0;
			
			
			
			
        }

	// Odd dimension case

        for i=1..(numOfNodesX+2>>2)-2,
        for j = (numOfNodesX>>1)..(numOfNodesX>>1)
        {
            tileNE_South[i].photonicOutE0 <--> tileSE_Empty[j*i+i].photonicInW0 if numOfNodesX%4!=0;
            tileNE_South[i].photonicOutE1 <--> tileSE_Empty[j*i+i].photonicInW1 if numOfNodesX%4!=0;
            tileNE_South[i].photonicInE0 <--> tileSE_Empty[j*i+i].photonicOutW0 if numOfNodesX%4!=0;
            tileNE_South[i].photonicInE1 <--> tileSE_Empty[j*i+i].photonicOutW1 if numOfNodesX%4!=0;

            tileNE_South[i].photonicOutS0 <--> tileNW_Empty[j*i+i].photonicInN0 if numOfNodesX%4!=0;
            tileNE_South[i].photonicOutS1 <--> tileNW_Empty[j*i+i].photonicInN1 if numOfNodesX%4!=0;
            tileNE_South[i].photonicInS0 <--> tileNW_Empty[j*i+i].photonicOutN0 if numOfNodesX%4!=0;
            tileNE_South[i].photonicInS1 <--> tileNW_Empty[j*i+i].photonicOutN1 if numOfNodesX%4!=0;

            tileNE_South[i].photonicOutW0 <--> tileSE_Empty[(j*i+i)-1].photonicInE0 if numOfNodesX%4!=0;
            tileNE_South[i].photonicOutW1 <--> tileSE_Empty[(j*i+i)-1].photonicInE1 if numOfNodesX%4!=0;
            tileNE_South[i].photonicInW0 <--> tileSE_Empty[(j*i+i)-1].photonicOutE0 if numOfNodesX%4!=0;
            tileNE_South[i].photonicInW1 <--> tileSE_Empty[(j*i+i)-1].photonicOutE1 if numOfNodesX%4!=0;

            tileNE_South[i].photonicOutN0 <--> tileNW_Empty[j*(i-1)+i].photonicInS0 if numOfNodesX%4!=0;
            tileNE_South[i].photonicOutN1 <--> tileNW_Empty[j*(i-1)+i].photonicInS1 if numOfNodesX%4!=0;
            tileNE_South[i].photonicInN0 <--> tileNW_Empty[j*(i-1)+i].photonicOutS0 if numOfNodesX%4!=0;
            tileNE_South[i].photonicInN1 <--> tileNW_Empty[j*(i-1)+i].photonicOutS1 if numOfNodesX%4!=0;



            tileSE_South[i].photonicOutE0 <--> tileNE_Empty[j*(i+1)-i].photonicInW0 if numOfNodesX%4!=0;
            tileSE_South[i].photonicOutE1 <--> tileNE_Empty[j*(i+1)-i].photonicInW1 if numOfNodesX%4!=0;
            tileSE_South[i].photonicInE0 <--> tileNE_Empty[j*(i+1)-i].photonicOutW0 if numOfNodesX%4!=0;
            tileSE_South[i].photonicInE1 <--> tileNE_Empty[j*(i+1)-i].photonicOutW1 if numOfNodesX%4!=0;

            tileSE_South[i].photonicOutS0 <--> tileSW_Empty[(j*(i+1)-i)-1].photonicInN0 if numOfNodesX%4!=0;
            tileSE_South[i].photonicOutS1 <--> tileSW_Empty[(j*(i+1)-i)-1].photonicInN1 if numOfNodesX%4!=0;
            tileSE_South[i].photonicInS0 <--> tileSW_Empty[(j*(i+1)-i)-1].photonicOutN0 if numOfNodesX%4!=0;
            tileSE_South[i].photonicInS1 <--> tileSW_Empty[(j*(i+1)-i)-1].photonicOutN1 if numOfNodesX%4!=0;

            tileSE_South[i].photonicOutW0 <--> tileNE_Empty[(j*(i+1)-i)-1].photonicInE0 if numOfNodesX%4!=0;
            tileSE_South[i].photonicOutW1 <--> tileNE_Empty[(j*(i+1)-i)-1].photonicInE1 if numOfNodesX%4!=0;
            tileSE_South[i].photonicInW0 <--> tileNE_Empty[(j*(i+1)-i)-1].photonicOutE0 if numOfNodesX%4!=0;
            tileSE_South[i].photonicInW1 <--> tileNE_Empty[(j*(i+1)-i)-1].photonicOutE1 if numOfNodesX%4!=0;

            tileSE_South[i].photonicOutN0 <--> tileSW_Empty[(j*i-i)-1].photonicInS0 if numOfNodesX%4!=0;
            tileSE_South[i].photonicOutN1 <--> tileSW_Empty[(j*i-i)-1].photonicInS1 if numOfNodesX%4!=0;
            tileSE_South[i].photonicInN0 <--> tileSW_Empty[(j*i-i)-1].photonicOutS0 if numOfNodesX%4!=0;
            tileSE_South[i].photonicInN1 <--> tileSW_Empty[(j*i-i)-1].photonicOutS1 if numOfNodesX%4!=0;


            tileNW_North[i].photonicOutE0 <--> tileSW_Empty[j*((j-i)-1)+i].photonicInW0 if numOfNodesX%4!=0;
            tileNW_North[i].photonicOutE1 <--> tileSW_Empty[j*((j-i)-1)+i].photonicInW1 if numOfNodesX%4!=0;
            tileNW_North[i].photonicInE0 <--> tileSW_Empty[j*((j-i)-1)+i].photonicOutW0 if numOfNodesX%4!=0;
            tileNW_North[i].photonicInE1 <--> tileSW_Empty[j*((j-i)-1)+i].photonicOutW1 if numOfNodesX%4!=0;

            tileNW_North[i].photonicOutS0 <--> tileNE_Empty[j*(j-i)+i].photonicInN0 if numOfNodesX%4!=0;
            tileNW_North[i].photonicOutS1 <--> tileNE_Empty[j*(j-i)+i].photonicInN1 if numOfNodesX%4!=0;
            tileNW_North[i].photonicInS0 <--> tileNE_Empty[j*(j-i)+i].photonicOutN0 if numOfNodesX%4!=0;
            tileNW_North[i].photonicInS1 <--> tileNE_Empty[j*(j-i)+i].photonicOutN1 if numOfNodesX%4!=0;

            tileNW_North[i].photonicOutW0 <--> tileSW_Empty[(j*((j-i)-1)+i)-1].photonicInE0 if numOfNodesX%4!=0;
            tileNW_North[i].photonicOutW1 <--> tileSW_Empty[(j*((j-i)-1)+i)-1].photonicInE1 if numOfNodesX%4!=0;
            tileNW_North[i].photonicInW0 <--> tileSW_Empty[(j*((j-i)-1)+i)-1].photonicOutE0 if numOfNodesX%4!=0;
            tileNW_North[i].photonicInW1 <--> tileSW_Empty[(j*((j-i)-1)+i)-1].photonicOutE1 if numOfNodesX%4!=0;

            tileNW_North[i].photonicOutN0 <--> tileNE_Empty[j*((j-i)-1)+i].photonicInS0 if numOfNodesX%4!=0;
            tileNW_North[i].photonicOutN1 <--> tileNE_Empty[j*((j-i)-1)+i].photonicInS1 if numOfNodesX%4!=0;
            tileNW_North[i].photonicInN0 <--> tileNE_Empty[j*((j-i)-1)+i].photonicOutS0 if numOfNodesX%4!=0;
            tileNW_North[i].photonicInN1 <--> tileNE_Empty[j*((j-i)-1)+i].photonicOutS1 if numOfNodesX%4!=0;


            tileSW_North[i].photonicOutE0 <--> tileNW_Empty[j*(j-i)-i].photonicInW0 if numOfNodesX%4!=0;
            tileSW_North[i].photonicOutE1 <--> tileNW_Empty[j*(j-i)-i].photonicInW1 if numOfNodesX%4!=0;
            tileSW_North[i].photonicInE0 <--> tileNW_Empty[j*(j-i)-i].photonicOutW0 if numOfNodesX%4!=0;
            tileSW_North[i].photonicInE1 <--> tileNW_Empty[j*(j-i)-i].photonicOutW1 if numOfNodesX%4!=0;

            tileSW_North[i].photonicOutS0 <--> tileSE_Empty[(j*((j-i)+1)-i)-1].photonicInN0 if numOfNodesX%4!=0;
            tileSW_North[i].photonicOutS1 <--> tileSE_Empty[(j*((j-i)+1)-i)-1].photonicInN1 if numOfNodesX%4!=0;
            tileSW_North[i].photonicInS0 <--> tileSE_Empty[(j*((j-i)+1)-i)-1].photonicOutN0 if numOfNodesX%4!=0;
            tileSW_North[i].photonicInS1 <--> tileSE_Empty[(j*((j-i)+1)-i)-1].photonicOutN1 if numOfNodesX%4!=0;

            tileSW_North[i].photonicOutW0 <--> tileNW_Empty[(j*(j-i)-i)-1].photonicInE0 if numOfNodesX%4!=0;
            tileSW_North[i].photonicOutW1 <--> tileNW_Empty[(j*(j-i)-i)-1].photonicInE1 if numOfNodesX%4!=0;
            tileSW_North[i].photonicInW0 <--> tileNW_Empty[(j*(j-i)-i)-1].photonicOutE0 if numOfNodesX%4!=0;
            tileSW_North[i].photonicInW1 <--> tileNW_Empty[(j*(j-i)-i)-1].photonicOutE1 if numOfNodesX%4!=0;

            tileSW_North[i].photonicOutN0 <--> tileSE_Empty[(j*(j-i)-i)-1].photonicInS0 if numOfNodesX%4!=0;
            tileSW_North[i].photonicOutN1 <--> tileSE_Empty[(j*(j-i)-i)-1].photonicInS1 if numOfNodesX%4!=0;
            tileSW_North[i].photonicInN0 <--> tileSE_Empty[(j*(j-i)-i)-1].photonicOutS0 if numOfNodesX%4!=0;
            tileSW_North[i].photonicInN1 <--> tileSE_Empty[(j*(j-i)-i)-1].photonicOutS1 if numOfNodesX%4!=0;
            
            //
            
            tileNE_South[i].electronicOutE0 --> tileSE_Empty[j*i+i].electronicInW0 if numOfNodesX%4!=0;
            tileNE_South[i].electronicOutE1 --> tileSE_Empty[j*i+i].electronicInW1 if numOfNodesX%4!=0;
            tileNE_South[i].electronicInE0 <-- tileSE_Empty[j*i+i].electronicOutW0 if numOfNodesX%4!=0;
            tileNE_South[i].electronicInE1 <-- tileSE_Empty[j*i+i].electronicOutW1 if numOfNodesX%4!=0;

            tileNE_South[i].electronicOutS0 --> tileNW_Empty[j*i+i].electronicInN0 if numOfNodesX%4!=0;
            tileNE_South[i].electronicOutS1 --> tileNW_Empty[j*i+i].electronicInN1 if numOfNodesX%4!=0;
            tileNE_South[i].electronicInS0 <-- tileNW_Empty[j*i+i].electronicOutN0 if numOfNodesX%4!=0;
            tileNE_South[i].electronicInS1 <-- tileNW_Empty[j*i+i].electronicOutN1 if numOfNodesX%4!=0;

            tileNE_South[i].electronicOutW0 --> tileSE_Empty[(j*i+i)-1].electronicInE0 if numOfNodesX%4!=0;
            tileNE_South[i].electronicOutW1 --> tileSE_Empty[(j*i+i)-1].electronicInE1 if numOfNodesX%4!=0;
            tileNE_South[i].electronicInW0 <-- tileSE_Empty[(j*i+i)-1].electronicOutE0 if numOfNodesX%4!=0;
            tileNE_South[i].electronicInW1 <-- tileSE_Empty[(j*i+i)-1].electronicOutE1 if numOfNodesX%4!=0;

            tileNE_South[i].electronicOutN0 --> tileNW_Empty[j*(i-1)+i].electronicInS0 if numOfNodesX%4!=0;
            tileNE_South[i].electronicOutN1 --> tileNW_Empty[j*(i-1)+i].electronicInS1 if numOfNodesX%4!=0;
            tileNE_South[i].electronicInN0 <-- tileNW_Empty[j*(i-1)+i].electronicOutS0 if numOfNodesX%4!=0;
            tileNE_South[i].electronicInN1 <-- tileNW_Empty[j*(i-1)+i].electronicOutS1 if numOfNodesX%4!=0;



            tileSE_South[i].electronicOutE0 --> tileNE_Empty[j*(i+1)-i].electronicInW0 if numOfNodesX%4!=0;
            tileSE_South[i].electronicOutE1 --> tileNE_Empty[j*(i+1)-i].electronicInW1 if numOfNodesX%4!=0;
            tileSE_South[i].electronicInE0 <-- tileNE_Empty[j*(i+1)-i].electronicOutW0 if numOfNodesX%4!=0;
            tileSE_South[i].electronicInE1 <-- tileNE_Empty[j*(i+1)-i].electronicOutW1 if numOfNodesX%4!=0;

            tileSE_South[i].electronicOutS0 --> tileSW_Empty[(j*(i+1)-i)-1].electronicInN0 if numOfNodesX%4!=0;
            tileSE_South[i].electronicOutS1 --> tileSW_Empty[(j*(i+1)-i)-1].electronicInN1 if numOfNodesX%4!=0;
            tileSE_South[i].electronicInS0 <-- tileSW_Empty[(j*(i+1)-i)-1].electronicOutN0 if numOfNodesX%4!=0;
            tileSE_South[i].electronicInS1 <-- tileSW_Empty[(j*(i+1)-i)-1].electronicOutN1 if numOfNodesX%4!=0;

            tileSE_South[i].electronicOutW0 --> tileNE_Empty[(j*(i+1)-i)-1].electronicInE0 if numOfNodesX%4!=0;
            tileSE_South[i].electronicOutW1 --> tileNE_Empty[(j*(i+1)-i)-1].electronicInE1 if numOfNodesX%4!=0;
            tileSE_South[i].electronicInW0 <-- tileNE_Empty[(j*(i+1)-i)-1].electronicOutE0 if numOfNodesX%4!=0;
            tileSE_South[i].electronicInW1 <-- tileNE_Empty[(j*(i+1)-i)-1].electronicOutE1 if numOfNodesX%4!=0;

            tileSE_South[i].electronicOutN0 --> tileSW_Empty[(j*i-i)-1].electronicInS0 if numOfNodesX%4!=0;
            tileSE_South[i].electronicOutN1 --> tileSW_Empty[(j*i-i)-1].electronicInS1 if numOfNodesX%4!=0;
            tileSE_South[i].electronicInN0 <-- tileSW_Empty[(j*i-i)-1].electronicOutS0 if numOfNodesX%4!=0;
            tileSE_South[i].electronicInN1 <-- tileSW_Empty[(j*i-i)-1].electronicOutS1 if numOfNodesX%4!=0;


            tileNW_North[i].electronicOutE0 --> tileSW_Empty[j*((j-i)-1)+i].electronicInW0 if numOfNodesX%4!=0;
            tileNW_North[i].electronicOutE1 --> tileSW_Empty[j*((j-i)-1)+i].electronicInW1 if numOfNodesX%4!=0;
            tileNW_North[i].electronicInE0 <-- tileSW_Empty[j*((j-i)-1)+i].electronicOutW0 if numOfNodesX%4!=0;
            tileNW_North[i].electronicInE1 <-- tileSW_Empty[j*((j-i)-1)+i].electronicOutW1 if numOfNodesX%4!=0;

            tileNW_North[i].electronicOutS0 --> tileNE_Empty[j*(j-i)+i].electronicInN0 if numOfNodesX%4!=0;
            tileNW_North[i].electronicOutS1 --> tileNE_Empty[j*(j-i)+i].electronicInN1 if numOfNodesX%4!=0;
            tileNW_North[i].electronicInS0 <-- tileNE_Empty[j*(j-i)+i].electronicOutN0 if numOfNodesX%4!=0;
            tileNW_North[i].electronicInS1 <-- tileNE_Empty[j*(j-i)+i].electronicOutN1 if numOfNodesX%4!=0;

            tileNW_North[i].electronicOutW0 --> tileSW_Empty[(j*((j-i)-1)+i)-1].electronicInE0 if numOfNodesX%4!=0;
            tileNW_North[i].electronicOutW1 --> tileSW_Empty[(j*((j-i)-1)+i)-1].electronicInE1 if numOfNodesX%4!=0;
            tileNW_North[i].electronicInW0 <-- tileSW_Empty[(j*((j-i)-1)+i)-1].electronicOutE0 if numOfNodesX%4!=0;
            tileNW_North[i].electronicInW1 <-- tileSW_Empty[(j*((j-i)-1)+i)-1].electronicOutE1 if numOfNodesX%4!=0;

            tileNW_North[i].electronicOutN0 --> tileNE_Empty[j*((j-i)-1)+i].electronicInS0 if numOfNodesX%4!=0;
            tileNW_North[i].electronicOutN1 --> tileNE_Empty[j*((j-i)-1)+i].electronicInS1 if numOfNodesX%4!=0;
            tileNW_North[i].electronicInN0 <-- tileNE_Empty[j*((j-i)-1)+i].electronicOutS0 if numOfNodesX%4!=0;
            tileNW_North[i].electronicInN1 <-- tileNE_Empty[j*((j-i)-1)+i].electronicOutS1 if numOfNodesX%4!=0;


            tileSW_North[i].electronicOutE0 --> tileNW_Empty[j*(j-i)-i].electronicInW0 if numOfNodesX%4!=0;
            tileSW_North[i].electronicOutE1 --> tileNW_Empty[j*(j-i)-i].electronicInW1 if numOfNodesX%4!=0;
            tileSW_North[i].electronicInE0 <-- tileNW_Empty[j*(j-i)-i].electronicOutW0 if numOfNodesX%4!=0;
            tileSW_North[i].electronicInE1 <-- tileNW_Empty[j*(j-i)-i].electronicOutW1 if numOfNodesX%4!=0;

            tileSW_North[i].electronicOutS0 --> tileSE_Empty[(j*((j-i)+1)-i)-1].electronicInN0 if numOfNodesX%4!=0;
            tileSW_North[i].electronicOutS1 --> tileSE_Empty[(j*((j-i)+1)-i)-1].electronicInN1 if numOfNodesX%4!=0;
            tileSW_North[i].electronicInS0 <-- tileSE_Empty[(j*((j-i)+1)-i)-1].electronicOutN0 if numOfNodesX%4!=0;
            tileSW_North[i].electronicInS1 <-- tileSE_Empty[(j*((j-i)+1)-i)-1].electronicOutN1 if numOfNodesX%4!=0;

            tileSW_North[i].electronicOutW0 --> tileNW_Empty[(j*(j-i)-i)-1].electronicInE0 if numOfNodesX%4!=0;
            tileSW_North[i].electronicOutW1 --> tileNW_Empty[(j*(j-i)-i)-1].electronicInE1 if numOfNodesX%4!=0;
            tileSW_North[i].electronicInW0 <-- tileNW_Empty[(j*(j-i)-i)-1].electronicOutE0 if numOfNodesX%4!=0;
            tileSW_North[i].electronicInW1 <-- tileNW_Empty[(j*(j-i)-i)-1].electronicOutE1 if numOfNodesX%4!=0;

            tileSW_North[i].electronicOutN0 --> tileSE_Empty[(j*(j-i)-i)-1].electronicInS0 if numOfNodesX%4!=0;
            tileSW_North[i].electronicOutN1 --> tileSE_Empty[(j*(j-i)-i)-1].electronicInS1 if numOfNodesX%4!=0;
            tileSW_North[i].electronicInN0 <-- tileSE_Empty[(j*(j-i)-i)-1].electronicOutS0 if numOfNodesX%4!=0;
            tileSW_North[i].electronicInN1 <-- tileSE_Empty[(j*(j-i)-i)-1].electronicOutS1 if numOfNodesX%4!=0;
            
           

        }


}

