//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package topologies.nonblockingSiliconNitride;


import electronicComponents.ElectronicChannel;
//
// TODO auto-generated module
//
module SiNPhotonicPlane
{
    parameters:
		int numOfNodesX;
        int numOfNodesY;
        
        // SiN specific parameters:
        double modulatorArrayLength; // only specifies the length of adjacent waveguides
        double detectorArrayLength;  // only specifies the length of adjacent waveguides
        
        double switchSizeX;	// X dimension of 1x2 PSE, in um
        double switchSizeY; // Y dimension of 1x2 PSE, in um
        double waveguideSpacing; // minimum size of waveguide
        double loopVertEndSpacing; // space between horizontal loops and end turns on N and S
        double loopHorizEndSpacing;  // space between vertical loops and end turns on E and W
        
        double interHorizLoopSpace; // space between horizontal loops
        double interVertLoopSpace; // space between vertical loops
        
        @display("i=planes/ph_plane");
        
    gates:
        inout photonicGatewayIn[numOfNodesX*numOfNodesY];
        inout photonicGatewayOut[numOfNodesX*numOfNodesY];

        inout photonicMemoryIn[numOfNodesX*numOfNodesY];
        inout photonicMemoryOut[numOfNodesX*numOfNodesY];
        
        input electronicGatewayIn[numOfNodesX*numOfNodesY];
        output electronicGatewayOut[numOfNodesX*numOfNodesY];

        input electronicMemoryIn[numOfNodesX*numOfNodesY];
        output electronicMemoryOut[numOfNodesX*numOfNodesY];
    submodules:
    	node[numOfNodesX*numOfNodesY]:SiNRouter
    	{
    	    parameters:
				nodeId = index;
				numOfNodesX = numOfNodesX;
				numOfNodesY = numOfNodesY;
				type = 100;
				SiN_CrosspointSizeX = switchSizeX;
				SiN_CrosspointSizeY = switchSizeY;
				
                @display("p=200,120,m,$numOfNodesX,160,160;b=50,50;o=,,1");
    	}
    	inj[numOfNodesX*numOfNodesY]:SiNInjection
    	{
    	    parameters:
				numLines = numOfNodesX;
				modulatorRow = index%numOfNodesX;
				length = modulatorArrayLength;
                @display("p=160,200,m,$numOfNodesX,160,160;b=30,30;o=,,1");
    	}
    	ej[numOfNodesX*numOfNodesY]:SiNEjection
    	{
    	    parameters:
				numLines = numOfNodesY;
				detectorColumn = numOfNodesX - 1 - int(index/numOfNodesX);
				length = detectorArrayLength;
                @display("p=120,160,m,$numOfNodesX,160,160;b=30,30;o=,,1");
    	}
    	crossW[numOfNodesX*numOfNodesY]:SiNCrossing
    	{
    	    parameters:
				switchSizeX = numOfNodesY;
				switchSizeY = numOfNodesX;
				SiN_CrosspointSizeX = waveguideSpacing;
				SiN_CrosspointSizeY = switchSizeY;
				
                @display("p=120,120,m,$numOfNodesX,160,160;b=30,30;o=,,1");
    	}
    	crossSW[numOfNodesX*numOfNodesY]:SiNCrossing
    	{
    	    parameters:
				switchSizeX = numOfNodesY;
				switchSizeY = numOfNodesX;
				SiN_CrosspointSizeX = waveguideSpacing;
				SiN_CrosspointSizeY = waveguideSpacing;
				
                @display("p=120,200,m,$numOfNodesX,160,160;b=30,30;o=,,1");
    	}
    	crossS[numOfNodesX*numOfNodesY]:SiNCrossing
    	{
    	    parameters:
				switchSizeX = numOfNodesY;
				switchSizeY = numOfNodesX;
				SiN_CrosspointSizeX = switchSizeX;
				SiN_CrosspointSizeY = waveguideSpacing;
				
                @display("p=200,200,m,$numOfNodesX,160,160;b=30,30;o=,,1");
    	}
    	turnN[numOfNodesX]:SiNTurn
    	{
            parameters:
				numTurns = numOfNodesY;
				firstLength = modulatorArrayLength;
				firstHeight = loopVertEndSpacing;
				spaceA = waveguideSpacing;
				spaceB = switchSizeX;
				spaceSide = waveguideSpacing;
				
				
                disp_Width = 0;
                @display("p=160,70,r,160;b=130,10;o=,,1");
    	}
    	
    	turnS[numOfNodesX]:SiNTurn
    	{
            parameters:
				numTurns = numOfNodesY;
				firstLength = modulatorArrayLength;
				firstHeight = loopVertEndSpacing;
				spaceA = waveguideSpacing;
				spaceB = switchSizeX;
				spaceSide = waveguideSpacing;
                disp_Width = 40 + numOfNodesY*80*2 + 50;
                @display("p=160,$disp_Width,r,160;b=130,10;o=,,1");
    	}
		turnE[numOfNodesY]:SiNTurn
    	{
            parameters:
				numTurns = numOfNodesX;
				firstLength = detectorArrayLength;
				firstHeight = loopHorizEndSpacing;
				spaceA = switchSizeY;
				spaceB = waveguideSpacing;
				spaceSide = waveguideSpacing;
                disp_Width = 40 + numOfNodesX*80*2 + 50;
                @display("p=$disp_Width,160,c,160;b=10,130;o=,,1");
    	}
    	
    	turnW[numOfNodesY]:SiNTurn
    	{
            parameters:
				numTurns = numOfNodesX;
				firstLength = detectorArrayLength;
				firstHeight = loopHorizEndSpacing;
				spaceA = switchSizeY;
				spaceB = waveguideSpacing;
				spaceSide = waveguideSpacing;
                disp_Width = 0;
                @display("p=70,160,c,160;b=10,130;o=,,1");
    	}
    	
    	lineVertOutNode[numOfNodesX * (numOfNodesY -1) * 2] : SiNLine
    	{
            parameters:
                numLines = numOfNodesY;
                length = interHorizLoopSpace;
                disp_Width = numOfNodesX * 2;
                @display("p=120,240,m,$disp_Width,80,160;b=30,15;o=,,1");
    	}
		lineVertInNode[numOfNodesX * numOfNodesY] : SiNLine
    	{
            parameters:
                numLines = numOfNodesY;
                length = detectorArrayLength;
                @display("p=200,160,m,$numOfNodesX,160,160;b=30,15;o=,,1");
    	}
		lineHorizOutNode[(numOfNodesX-1) * numOfNodesY * 2] : SiNLine
    	{
            parameters:
                numLines = numOfNodesX;
                length = interVertLoopSpace;
                disp_Width = numOfNodesX - 1;
                @display("p=240,120,m,$disp_Width,160,80;b=15,30;o=,,1");
    	}
		lineHorizInNode[numOfNodesX * numOfNodesY] : SiNLine
    	{
            parameters:
                numLines = numOfNodesX;
                length = modulatorArrayLength;
                @display("p=160,120,m,$numOfNodesX,160,160;b=15,30;o=,,1");
    	}
    	
	connections allowunconnected:
	    
	    //electronic connections
	    
	    //horizontal connections
        for i = 0 .. numOfNodesX - 1, for j = 0 .. numOfNodesY - 1, for q = 0 .. numOfNodesX - 1 
        {
            node[i+j*numOfNodesX].electronicHorizontalOut[q] --> ElectronicChannel --> node[q+j*numOfNodesX].electronicHorizontalIn[i-1] if (q < i);
            node[i+j*numOfNodesX].electronicHorizontalOut[q-1] --> ElectronicChannel --> node[q+j*numOfNodesX].electronicHorizontalIn[i] if (q > i);
        }
        
         //vertical connections
        for i = 0 .. numOfNodesX - 1, for j = 0 .. numOfNodesY - 1, for q = 0 .. numOfNodesY - 1 
        {
            node[i+j*numOfNodesX].electronicVerticalOut[q] --> ElectronicChannel --> node[i+q*numOfNodesX].electronicVerticalIn[j-1] if (q < j);
            node[i+j*numOfNodesX].electronicVerticalOut[q-1] --> ElectronicChannel --> node[i+q*numOfNodesX].electronicVerticalIn[j] if (q > j);
        }

       //gateway connections
       for i = 0 .. numOfNodesX * numOfNodesY - 1
       {
           node[i].electronicGatewayIn <-- ElectronicChannel{power = false;} <-- electronicGatewayIn[i];
           node[i].electronicGatewayOut --> ElectronicChannel{power = false;} --> electronicGatewayOut[i];
       }
              
              
       //photonic connections
       
       //gates to processors
       for i = 0 .. numOfNodesX * numOfNodesY - 1
       {           
           inj[i].dataIn <--> photonicGatewayIn[i];
           ej[i].dataOut <--> photonicGatewayOut[i];
       }
       
       //switch node connections
       for i = 0 .. numOfNodesX - 1, for j = 0 .. numOfNodesY - 1, for k = 0 .. numOfNodesX - 1
       {
           node[i + j * numOfNodesX].photonicHorizontalIn[k] <--> lineHorizInNode[i + j * numOfNodesX].portA[k];
           node[i + j * numOfNodesX].photonicHorizontalOut[k] <--> lineHorizOutNode[i + j * (numOfNodesX-1) * 2].portB[k] if i != numOfNodesX - 1;
           node[i + j * numOfNodesX].photonicHorizontalOut[k] <-->  turnE[j].portA[numOfNodesX - 1 - k] if i == numOfNodesX - 1;
       }
       
       for i = 0 .. numOfNodesX - 1, for j = 0 .. numOfNodesY - 1, for k = 0 .. numOfNodesY - 1, for Z = numOfNodesX*2 .. numOfNodesX*2
       {
           node[i + j * numOfNodesX].photonicVerticalIn[k] <--> lineVertInNode[i + j * numOfNodesX].portA[k];
           node[i + j * numOfNodesX].photonicVerticalOut[k] <--> lineVertOutNode[(i*2+1) + (j-1) * Z].portB[k] if j != 0;
           node[i + j * numOfNodesX].photonicVerticalOut[k] <--> turnN[i].portB[k] if j == 0;
       }
       
       //west crossing connections
       for i = 0 .. numOfNodesX - 1, for j = 0 .. numOfNodesY - 1, for k = 0 .. numOfNodesX - 1
       {
           crossW[i + j * numOfNodesX].horizontalIn[k] <--> lineHorizOutNode[(i-1) + j * (numOfNodesX-1) * 2].portA[k] if i != 0;
           crossW[i + j * numOfNodesX].horizontalIn[k] <--> turnW[j].portA[numOfNodesX - 1 - k] if i == 0;
           crossW[i + j * numOfNodesX].horizontalOut[k] <--> lineHorizInNode[i + j * numOfNodesX].portB[k];
       }
       
       for i = 0 .. numOfNodesX - 1, for j = 0 .. numOfNodesY - 1, for k = 0 .. numOfNodesY - 1
       {
           crossW[i + j * numOfNodesX].verticalIn[k] <--> ej[i + j * numOfNodesX].portA[k];
           crossW[i + j * numOfNodesX].verticalOut[k] <--> lineVertOutNode[(i*2) + (j-1) * numOfNodesX * 2].portB[k] if j != 0;
           crossW[i + j * numOfNodesX].verticalOut[k] <--> turnN[i].portA[numOfNodesY - 1 - k] if j == 0;
       }
       //south west crossing connections
       for i = 0 .. numOfNodesX - 1, for j = 0 .. numOfNodesY - 1, for k = 0 .. numOfNodesX - 1, for Z = (numOfNodesX-1) .. (numOfNodesX-1)
       {
           crossSW[i + j * numOfNodesX].horizontalIn[k] <--> lineHorizOutNode[(i-1) + (j * 2 + 1) * Z].portA[k] if i != 0;
           crossSW[i + j * numOfNodesX].horizontalIn[k] <--> turnW[j].portB[k] if i == 0; 
           crossSW[i + j * numOfNodesX].horizontalOut[k] <--> inj[i + j * numOfNodesX].portB[k];
       }
       
       for i = 0 .. numOfNodesX - 1, for j = 0 .. numOfNodesY - 1, for k = 0 .. numOfNodesY - 1
       {
           crossSW[i + j * numOfNodesX].verticalIn[k] <--> lineVertOutNode[i*2 + j * numOfNodesX * 2].portA[k] if j != numOfNodesY - 1;
           crossSW[i + j * numOfNodesX].verticalIn[k] <--> turnS[i].portA[numOfNodesY - 1 - k] if j == numOfNodesY - 1;
           crossSW[i + j * numOfNodesX].verticalOut[k] <--> ej[i + j * numOfNodesX].portB[k];
       }
       //south crossing connections
       for i = 0 .. numOfNodesX - 1, for j = 0 .. numOfNodesY - 1, for k = 0 .. numOfNodesX - 1
       {
           crossS[i + j * numOfNodesX].horizontalIn[k] <--> inj[i + j * numOfNodesX].portA[k];
           crossS[i + j * numOfNodesX].horizontalOut[k] <--> lineHorizOutNode[i + (j*2+1) * (numOfNodesX-1)].portB[k] if i != numOfNodesX - 1;
           crossS[i + j * numOfNodesX].horizontalOut[k] <--> turnE[j].portB[k] if i == numOfNodesX - 1;
       }
       
       for i = 0 .. numOfNodesX - 1, for j = 0 .. numOfNodesY - 1, for k = 0 .. numOfNodesY - 1
       {
           crossS[i + j * numOfNodesX].verticalIn[k] <--> lineVertOutNode[(i*2+1) + j * (numOfNodesX*2)].portA[k] if j != numOfNodesY - 1;
           crossS[i + j * numOfNodesX].verticalIn[k] <--> turnS[i].portB[k] if j == numOfNodesY - 1;
           crossS[i + j * numOfNodesX].verticalOut[k] <--> lineVertInNode[i + j * numOfNodesX].portB[k];
       }
}
