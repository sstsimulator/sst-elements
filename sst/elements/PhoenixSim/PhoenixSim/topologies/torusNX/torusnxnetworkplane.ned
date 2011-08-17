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

package topologies.torusNX;

import photonic.devices.UTurnBidirectional;
import electronicComponents.ElectronicChannel;
import topologies.torusNX.TorusNXNetworkPlane;
//
// TODO auto-generated module
//
module TorusNXNetworkPlane
{
    parameters:
        int numOfNodesX;
        int numOfNodesY;
        
        double PositionLeftX = default(0);
        double PositionBottomY = default(0);
        
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
        tileNW[(numOfNodesX>>1)*(numOfNodesY>>1)]: TorusNXTileSE_Regular  {
            parameters:
                numX = numOfNodesX;
                numY = numOfNodesY;
				nodeId = index*2 + floor(index*2/numOfNodesX)*numOfNodesX;
                disp_Width = (numOfNodesX>>1)+(numOfNodesX%2);
                @display("p=120,120,m,$disp_Width,160,160;b=50,50;o=,,1");
        }
        
        tileNE[(numOfNodesX>>1)*(numOfNodesY>>1)]: TorusNXTileNE_Regular {
            parameters:
                numX = numOfNodesX;
                numY = numOfNodesY;
				nodeId = index*2+1 + floor((index*2+1)/numOfNodesX)*numOfNodesX;
                disp_Width = numOfNodesX>>1;
                @display("p=200,120,m,$disp_Width,160,160;b=50,50;o=,,1");
        }

        tileSE[(numOfNodesX>>1)*(numOfNodesY>>1)]: TorusNXTileSW_Regular {
            parameters:
                numX = numOfNodesX;
                numY = numOfNodesY;
				nodeId = index*2+1 + floor(((index*2+1)/numOfNodesX)+1)*numOfNodesX;
                disp_Width = numOfNodesX>>1;
                @display("p=200,200,m,$disp_Width,160,160;b=50,50;o=,,1");
        }

        tileSW[((numOfNodesX>>1)+numOfNodesX%2)*(numOfNodesY>>1)]: TorusNXTileNW_Regular {
            parameters:
                numX = numOfNodesX;
                numY = numOfNodesY;
				nodeId = index*2 + floor((index*2/numOfNodesX)+1)*numOfNodesX;
                disp_Width = (numOfNodesX>>1)+(numOfNodesX%2);
                @display("p=120,200,m,$disp_Width,160,160;b=50,50;o=,,1");
        }

        uturnN[numOfNodesX]: UTurnBidirectional {
            parameters:
                TurnDistance = 350;
                disp_Width = 0;
                @display("p=120,70,r,80;b=50,10;o=,,1");
        }

        uturnS[numOfNodesX]: UTurnBidirectional {
            parameters:
                TurnDistance = 350;
                disp_Width = 40 + numOfNodesY*80 + 50;
                @display("p=120,$disp_Width,r,80;b=50,10;o=,,1");
        }

        uturnE[numOfNodesY]: UTurnBidirectional {
            parameters:
                TurnDistance = 350;
                disp_Width = 40 + numOfNodesX*80 + 50;
                @display("p=$disp_Width,120,c,80;b=10,50;o=,,1");
        }

        uturnW[numOfNodesY]: UTurnBidirectional {
            parameters:
                TurnDistance = 350;
                disp_Width = 0;
                @display("p=70,120,c,80;b=10,50;o=,,1");
        }

    connections allowunconnected:
        // Setup Tiles
        for i=0..numOfNodesX-1, for j=0..numOfNodesY-1 {
            // NW -> NE Main Torus
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicETopOut <--> tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicWTopIn if i%2==0 && j%2==0 && i!=(numOfNodesX-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicETopIn <--> tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicWTopOut if i%2==0 && j%2==0 && i!=(numOfNodesX-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicEBottomOut <--> tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicWBottomIn if i%2==0 && j%2==0 && i!=(numOfNodesX-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicEBottomIn <--> tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicWBottomOut if i%2==0 && j%2==0 && i!=(numOfNodesX-1);

            // NW -> SW Main Torus
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicSLeftOut <--> tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicNLeftIn if i%2==0 && j%2==0 && j!=(numOfNodesY-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicSLeftIn <--> tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicNLeftOut if i%2==0 && j%2==0 && j!=(numOfNodesY-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicSRightOut <--> tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicNRightIn if i%2==0 && j%2==0 && j!=(numOfNodesY-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicSRightIn <--> tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicNRightOut if i%2==0 && j%2==0 && j!=(numOfNodesY-1);

            // SW -> SE Main Torus
            tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicETopOut <--> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicWTopIn if i%2==0 && j%2!=0 && i!=(numOfNodesX-1);
            tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicETopIn <--> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicWTopOut if i%2==0 && j%2!=0 && i!=(numOfNodesX-1);
            tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicEBottomOut <--> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicWBottomIn if i%2==0 && j%2!=0 && i!=(numOfNodesX-1);
            tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicEBottomIn <--> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicWBottomOut if i%2==0 && j%2!=0 && i!=(numOfNodesX-1);

            // NE -> SE Main Torus
            tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicSLeftOut <--> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicNLeftIn if i%2!=0 && j%2==0 && j!=(numOfNodesY-1);
            tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicSLeftIn <--> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicNLeftOut if i%2!=0 && j%2==0 && j!=(numOfNodesY-1);
            tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicSRightOut <--> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicNRightIn if i%2!=0 && j%2==0 && j!=(numOfNodesY-1);
            tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicSRightIn <--> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicNRightOut if i%2!=0 && j%2==0 && j!=(numOfNodesY-1);



            // NW -> NE Main Torus
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicETopOut --> tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicWTopIn if i%2==0 && j%2==0 && i!=(numOfNodesX-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicETopIn <-- tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicWTopOut if i%2==0 && j%2==0 && i!=(numOfNodesX-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicEBottomOut --> tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicWBottomIn if i%2==0 && j%2==0 && i!=(numOfNodesX-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicEBottomIn <-- tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicWBottomOut if i%2==0 && j%2==0 && i!=(numOfNodesX-1);

            // NW -> SW Main Torus
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSLeftOut --> tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNLeftIn if i%2==0 && j%2==0 && j!=(numOfNodesY-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSLeftIn <-- tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNLeftOut if i%2==0 && j%2==0 && j!=(numOfNodesY-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSRightOut --> tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNRightIn if i%2==0 && j%2==0 && j!=(numOfNodesY-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSRightIn <-- tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNRightOut if i%2==0 && j%2==0 && j!=(numOfNodesY-1);

            // SW -> SE Main Torus
            tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicETopOut --> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicWTopIn if i%2==0 && j%2!=0 && i!=(numOfNodesX-1);
            tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicETopIn <-- tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicWTopOut if i%2==0 && j%2!=0 && i!=(numOfNodesX-1);
            tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicEBottomOut --> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicWBottomIn if i%2==0 && j%2!=0 && i!=(numOfNodesX-1);
            tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicEBottomIn <-- tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicWBottomOut if i%2==0 && j%2!=0 && i!=(numOfNodesX-1);

            // NE -> SE Main Torus
            tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSLeftOut --> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNLeftIn if i%2!=0 && j%2==0 && j!=(numOfNodesY-1);
            tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSLeftIn <-- tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNLeftOut if i%2!=0 && j%2==0 && j!=(numOfNodesY-1);
            tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSRightOut --> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNRightIn if i%2!=0 && j%2==0 && j!=(numOfNodesY-1);
            tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSRightIn <-- tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNRightOut if i%2!=0 && j%2==0 && j!=(numOfNodesY-1);

        }
        for i=0..(numOfNodesX>>1)-1, for j=0..(numOfNodesY>>1)-2 {
            // SW -> NW Main Torus
            tileSW[(i) + (j)*(numOfNodesX>>1)].photonicSLeftOut <--> tileNW[(i) + ((j) + 1)*(numOfNodesX>>1)].photonicNLeftIn;
            tileSW[(i) + (j)*(numOfNodesX>>1)].photonicSLeftIn <--> tileNW[(i) + ((j) + 1)*(numOfNodesX>>1)].photonicNLeftOut;
            tileSW[(i) + (j)*(numOfNodesX>>1)].photonicSRightOut <--> tileNW[(i) + ((j) + 1)*(numOfNodesX>>1)].photonicNRightIn;
            tileSW[(i) + (j)*(numOfNodesX>>1)].photonicSRightIn <--> tileNW[(i) + ((j) + 1)*(numOfNodesX>>1)].photonicNRightOut;

            // SE -> NE Main Torus
            tileSE[(i) + (j)*(numOfNodesX>>1)].photonicSLeftOut <--> tileNE[(i) + ((j) + 1)*(numOfNodesX>>1)].photonicNLeftIn;
            tileSE[(i) + (j)*(numOfNodesX>>1)].photonicSLeftIn <--> tileNE[(i) + ((j) + 1)*(numOfNodesX>>1)].photonicNLeftOut;
            tileSE[(i) + (j)*(numOfNodesX>>1)].photonicSRightOut <--> tileNE[(i) + ((j) + 1)*(numOfNodesX>>1)].photonicNRightIn;
            tileSE[(i) + (j)*(numOfNodesX>>1)].photonicSRightIn <--> tileNE[(i) + ((j) + 1)*(numOfNodesX>>1)].photonicNRightOut;


			// SW -> NW Main Torus
            tileSW[(i) + (j)*(numOfNodesX>>1)].electronicSLeftOut --> tileNW[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNLeftIn;
            tileSW[(i) + (j)*(numOfNodesX>>1)].electronicSLeftIn <-- tileNW[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNLeftOut;
            tileSW[(i) + (j)*(numOfNodesX>>1)].electronicSRightOut --> tileNW[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNRightIn;
            tileSW[(i) + (j)*(numOfNodesX>>1)].electronicSRightIn <-- tileNW[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNRightOut;

            // SE -> NE Main Torus
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicSLeftOut --> tileNE[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNLeftIn;
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicSLeftIn <-- tileNE[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNLeftOut;
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicSRightOut --> tileNE[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNRightIn;
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicSRightIn <-- tileNE[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNRightOut;


        }

        for i=0..(numOfNodesX>>1)-2, for j=0..(numOfNodesY>>1)-1 {
            // NE -> NW Main Torus
            tileNE[(i) + (j)*(numOfNodesX>>1)].photonicETopOut <--> tileNW[(i) + 1 + (j)*(numOfNodesX>>1)].photonicWTopIn;
            tileNE[(i) + (j)*(numOfNodesX>>1)].photonicETopIn <--> tileNW[(i) + 1 + (j)*(numOfNodesX>>1)].photonicWTopOut;
            tileNE[(i) + (j)*(numOfNodesX>>1)].photonicEBottomOut <--> tileNW[(i) + 1 + (j)*(numOfNodesX>>1)].photonicWBottomIn;
            tileNE[(i) + (j)*(numOfNodesX>>1)].photonicEBottomIn <--> tileNW[(i) + 1 + (j)*(numOfNodesX>>1)].photonicWBottomOut;

            // SE -> SW Main Torus
            tileSE[(i) + (j)*(numOfNodesX>>1)].photonicETopOut <--> tileSW[(i) + 1 + (j)*(numOfNodesX>>1)].photonicWTopIn;
            tileSE[(i) + (j)*(numOfNodesX>>1)].photonicETopIn <--> tileSW[(i) + 1 + (j)*(numOfNodesX>>1)].photonicWTopOut;
            tileSE[(i) + (j)*(numOfNodesX>>1)].photonicEBottomOut <--> tileSW[(i) + 1 + (j)*(numOfNodesX>>1)].photonicWBottomIn;
            tileSE[(i) + (j)*(numOfNodesX>>1)].photonicEBottomIn <--> tileSW[(i) + 1 + (j)*(numOfNodesX>>1)].photonicWBottomOut;


// NE -> NW Main Torus
            tileNE[(i) + (j)*(numOfNodesX>>1)].electronicETopOut --> tileNW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWTopIn;
            tileNE[(i) + (j)*(numOfNodesX>>1)].electronicETopIn <-- tileNW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWTopOut;
            tileNE[(i) + (j)*(numOfNodesX>>1)].electronicEBottomOut --> tileNW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWBottomIn;
            tileNE[(i) + (j)*(numOfNodesX>>1)].electronicEBottomIn <-- tileNW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWBottomOut;

            // SE -> SW Main Torus
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicETopOut --> tileSW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWTopIn;
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicETopIn <-- tileSW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWTopOut;
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicEBottomOut --> tileSW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWBottomIn;
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicEBottomIn <-- tileSW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWBottomOut;

        }

        
        
       for i=0..numOfNodesX-1, for j=0..numOfNodesY-1 {
            // Gateway Connections
            photonicGatewayOut[i + j*numOfNodesX] <--> tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicGatewayOut if i%2==0 && j%2==0;
            photonicGatewayIn[i + j*numOfNodesX] <--> tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicGatewayIn if i%2==0 && j%2==0;

            photonicGatewayOut[i + j*numOfNodesX] <--> tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicGatewayOut if i%2!=0 && j%2==0;
            photonicGatewayIn[i + j*numOfNodesX] <--> tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicGatewayIn if i%2!=0 && j%2==0;

            photonicGatewayOut[i + j*numOfNodesX] <--> tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicGatewayOut if i%2==0 && j%2!=0;
            photonicGatewayIn[i + j*numOfNodesX] <--> tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicGatewayIn if i%2==0 && j%2!=0;

            photonicGatewayOut[i + j*numOfNodesX] <--> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicGatewayOut if i%2!=0 && j%2!=0;
            photonicGatewayIn[i + j*numOfNodesX] <--> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicGatewayIn if i%2!=0 && j%2!=0;
        
       
        	electronicGatewayOut[i + j*numOfNodesX] <-- ElectronicChannel{power = false;@display("m=w;ls=red,0");} <-- tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicGatewayOut if i%2==0 && j%2==0;
            electronicGatewayIn[i + j*numOfNodesX] --> ElectronicChannel{power = false;@display("m=w;ls=red,0");} --> tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicGatewayIn if i%2==0 && j%2==0;

            electronicGatewayOut[i + j*numOfNodesX] <-- ElectronicChannel{power = false;@display("m=w;ls=red,0");} <-- tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicGatewayOut if i%2!=0 && j%2==0;
            electronicGatewayIn[i + j*numOfNodesX] --> ElectronicChannel{power = false;@display("m=w;ls=red,0");} --> tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicGatewayIn if i%2!=0 && j%2==0;

            electronicGatewayOut[i + j*numOfNodesX] <-- ElectronicChannel{power = false;@display("m=w;ls=red,0");} <-- tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicGatewayOut if i%2==0 && j%2!=0;
            electronicGatewayIn[i + j*numOfNodesX] --> ElectronicChannel{power = false;@display("m=w;ls=red,0");} --> tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicGatewayIn if i%2==0 && j%2!=0;

            electronicGatewayOut[i + j*numOfNodesX] <-- ElectronicChannel{power = false;@display("m=w;ls=red,0");} <-- tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicGatewayOut if i%2!=0 && j%2!=0;
            electronicGatewayIn[i + j*numOfNodesX] --> ElectronicChannel{power = false;@display("m=w;ls=red,0");} --> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicGatewayIn if i%2!=0 && j%2!=0;
        
		 }
        




        // Setup U-Turns E and W
        for i=0..numOfNodesY-1 {
            tileNW[(i>>1)*(numOfNodesX>>1)].photonicWTopOut <--> uturnW[i].photonicOuterBendIn if i%2==0;
            tileNW[(i>>1)*(numOfNodesX>>1)].photonicWTopIn <--> uturnW[i].photonicInnerBendOut if i%2==0;
            tileNW[(i>>1)*(numOfNodesX>>1)].photonicWBottomOut <--> uturnW[i].photonicInnerBendIn if i%2==0;
            tileNW[(i>>1)*(numOfNodesX>>1)].photonicWBottomIn <--> uturnW[i].photonicOuterBendOut if i%2==0;

            tileSW[(i>>1)*(numOfNodesX>>1)].photonicWTopOut <--> uturnW[i].photonicOuterBendIn if i%2!=0;
            tileSW[(i>>1)*(numOfNodesX>>1)].photonicWTopIn <--> uturnW[i].photonicInnerBendOut if i%2!=0;
            tileSW[(i>>1)*(numOfNodesX>>1)].photonicWBottomOut <--> uturnW[i].photonicInnerBendIn if i%2!=0;
            tileSW[(i>>1)*(numOfNodesX>>1)].photonicWBottomIn <--> uturnW[i].photonicOuterBendOut if i%2!=0;
            
            
            tileNW[(i>>1)*(numOfNodesX>>1)].electronicWTopOut --> tileNW[(i>>1)*(numOfNodesX>>1)].electronicWBottomIn if i%2==0;
            tileNW[(i>>1)*(numOfNodesX>>1)].electronicWTopIn <-- tileNW[(i>>1)*(numOfNodesX>>1)].electronicWBottomOut if i%2==0;
            
            tileSW[(i>>1)*(numOfNodesX>>1)].electronicWTopOut --> tileSW[(i>>1)*(numOfNodesX>>1)].electronicWBottomIn if i%2!=0;
            tileSW[(i>>1)*(numOfNodesX>>1)].electronicWTopIn <-- tileSW[(i>>1)*(numOfNodesX>>1)].electronicWBottomOut if i%2!=0;
        }

        for i=0..(numOfNodesY>>1)-1 {
            tileNE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].photonicETopOut <--> uturnE[i*2].photonicOuterBendIn;
            tileNE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].photonicETopIn <--> uturnE[i*2].photonicInnerBendOut;
            tileNE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].photonicEBottomOut <--> uturnE[i*2].photonicInnerBendIn;
            tileNE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].photonicEBottomIn <--> uturnE[i*2].photonicOuterBendOut;

            tileSE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].photonicETopOut <--> uturnE[i*2+1].photonicOuterBendIn;
            tileSE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].photonicETopIn <--> uturnE[i*2+1].photonicInnerBendOut;
            tileSE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].photonicEBottomOut <--> uturnE[i*2+1].photonicInnerBendIn;
            tileSE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].photonicEBottomIn <--> uturnE[i*2+1].photonicOuterBendOut;
            
            tileNE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].electronicETopOut --> ElectronicChannel{@display("m=w;ls=red,0");} --> tileNE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].electronicEBottomIn;
            tileNE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].electronicETopIn <-- ElectronicChannel{@display("m=w;ls=red,0");} <-- tileNE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].electronicEBottomOut;
            
            tileSE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].electronicETopOut --> ElectronicChannel{@display("m=w;ls=red,0");} --> tileSE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].electronicEBottomIn;
            tileSE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].electronicETopIn <-- ElectronicChannel{@display("m=w;ls=red,0");} <-- tileSE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].electronicEBottomOut;
        }

        for i=0..numOfNodesX-1 {
            //Setup U-turns N and S
            tileNW[(i>>1)].photonicNLeftOut <--> uturnN[i].photonicOuterBendIn if i%2==0;
            tileNW[(i>>1)].photonicNLeftIn <--> uturnN[i].photonicInnerBendOut if i%2==0;
            tileNW[(i>>1)].photonicNRightOut <--> uturnN[i].photonicInnerBendIn if i%2==0;
            tileNW[(i>>1)].photonicNRightIn <--> uturnN[i].photonicOuterBendOut if i%2==0;

            tileNE[(i>>1)].photonicNLeftOut <--> uturnN[i].photonicOuterBendIn if i%2!=0;
            tileNE[(i>>1)].photonicNLeftIn <--> uturnN[i].photonicInnerBendOut if i%2!=0;
            tileNE[(i>>1)].photonicNRightOut <--> uturnN[i].photonicInnerBendIn if i%2!=0;
            tileNE[(i>>1)].photonicNRightIn <--> uturnN[i].photonicOuterBendOut if i%2!=0;
            
            tileNW[(i>>1)].electronicNLeftOut --> tileNW[(i>>1)].electronicNRightIn if i%2==0;
			tileNW[(i>>1)].electronicNLeftIn <-- tileNW[(i>>1)].electronicNRightOut if i%2==0;
			
			tileNE[(i>>1)].electronicNLeftOut --> tileNE[(i>>1)].electronicNRightIn if i%2!=0;
			tileNE[(i>>1)].electronicNLeftIn <-- tileNE[(i>>1)].electronicNRightOut if i%2!=0;
		}

        for i=0..(numOfNodesX>>1)-1,
        for j=(numOfNodesY>>1)..(numOfNodesY>>1)
        {
            tileSW[(i) + ((numOfNodesY>>1) - 1)*(numOfNodesX>>1)].photonicSLeftOut <--> uturnS[i*2].photonicOuterBendIn;
            tileSW[(i) + ((numOfNodesY>>1) - 1)*(numOfNodesX>>1)].photonicSLeftIn <--> uturnS[i*2].photonicInnerBendOut;
            tileSW[(i) + ((numOfNodesY>>1) - 1)*(numOfNodesX>>1)].photonicSRightOut <--> uturnS[i*2].photonicInnerBendIn;
            tileSW[(i) + ((numOfNodesY>>1) - 1)*(numOfNodesX>>1)].photonicSRightIn <--> uturnS[i*2].photonicOuterBendOut;

            tileSE[(i) + ((numOfNodesY>>1) - 1)*(numOfNodesX>>1)].photonicSLeftOut <--> uturnS[i*2+1].photonicOuterBendIn;
            tileSE[(i) + ((numOfNodesY>>1) - 1)*(numOfNodesX>>1)].photonicSLeftIn <--> uturnS[i*2+1].photonicInnerBendOut;
            tileSE[(i) + ((numOfNodesY>>1) - 1)*(numOfNodesX>>1)].photonicSRightOut <--> uturnS[i*2+1].photonicInnerBendIn;
            tileSE[(i) + ((numOfNodesY>>1) - 1)*(numOfNodesX>>1)].photonicSRightIn <--> uturnS[i*2+1].photonicOuterBendOut;

			tileSW[(i) + (j - 1)*(numOfNodesX>>1)].electronicSLeftOut  --> tileSW[(i) + (j - 1)*(numOfNodesX>>1)].electronicSRightIn;
			tileSW[(i) + (j - 1)*(numOfNodesX>>1)].electronicSLeftIn  <-- tileSW[(i) + (j - 1)*(numOfNodesX>>1)].electronicSRightOut;

			tileSE[(i) + (j - 1)*(numOfNodesX>>1)].electronicSLeftOut  --> tileSE[(i) + (j - 1)*(numOfNodesX>>1)].electronicSRightIn;
			tileSE[(i) + (j - 1)*(numOfNodesX>>1)].electronicSLeftIn  <-- tileSE[(i) + (j - 1)*(numOfNodesX>>1)].electronicSRightOut;
		}

        //Setup Memory Gateways
        for i=0..numOfNodesX-1, for j=0..numOfNodesY-1 {
            photonicMemoryIn[i + j*numOfNodesX] <--> {@display("m=w;ls=red,0");} <--> tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicMemoryIn if i%2==0 && j%2==0;
            photonicMemoryOut[i + j*numOfNodesX] <--> {@display("m=w;ls=red,0");} <--> tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicMemoryOut if i%2==0 && j%2==0;

            photonicMemoryIn[i + j*numOfNodesX] <--> {@display("m=w;ls=red,0");} <--> tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicMemoryIn if i%2!=0 && j%2==0;
            photonicMemoryOut[i + j*numOfNodesX] <--> {@display("m=w;ls=red,0");} <--> tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicMemoryOut if i%2!=0 && j%2==0;

            photonicMemoryIn[i + j*numOfNodesX] <--> {@display("m=w;ls=red,0");} <--> tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicMemoryIn if i%2==0 && j%2!=0;
            photonicMemoryOut[i + j*numOfNodesX] <--> {@display("m=w;ls=red,0");} <--> tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicMemoryOut if i%2==0 && j%2!=0;

            photonicMemoryIn[i + j*numOfNodesX] <--> {@display("m=w;ls=red,0");} <--> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicMemoryIn if i%2!=0 && j%2!=0;
            photonicMemoryOut[i + j*numOfNodesX] <--> {@display("m=w;ls=red,0");} <--> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicMemoryOut if i%2!=0 && j%2!=0;
        
        	electronicMemoryIn[i + j*numOfNodesX] --> {@display("m=w;ls=red,0");} --> tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicMemoryIn if i%2==0 && j%2==0;
            electronicMemoryOut[i + j*numOfNodesX] <-- {@display("m=w;ls=red,0");} <-- tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicMemoryOut if i%2==0 && j%2==0;

            electronicMemoryIn[i + j*numOfNodesX] --> {@display("m=w;ls=red,0");} --> tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicMemoryIn if i%2!=0 && j%2==0;
            electronicMemoryOut[i + j*numOfNodesX] <-- {@display("m=w;ls=red,0");} <-- tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicMemoryOut if i%2!=0 && j%2==0;

            electronicMemoryIn[i + j*numOfNodesX] --> {@display("m=w;ls=red,0");} --> tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicMemoryIn if i%2==0 && j%2!=0;
            electronicMemoryOut[i + j*numOfNodesX] <-- {@display("m=w;ls=red,0");} <-- tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicMemoryOut if i%2==0 && j%2!=0;

            electronicMemoryIn[i + j*numOfNodesX] --> {@display("m=w;ls=red,0");} --> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicMemoryIn if i%2!=0 && j%2!=0;
            electronicMemoryOut[i + j*numOfNodesX] <-- {@display("m=w;ls=red,0");} <-- tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicMemoryOut if i%2!=0 && j%2!=0;  
        }
}

