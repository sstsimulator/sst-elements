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

package topologies.photonicMesh;

import chipComponents.HybridRouter;
import photonic.devices.LineBidirectional;

import electronicComponents.ElectronicChannel;


module PhotonicMeshPlane
{
    parameters:
        int numOfNodesX;
        int numOfNodesY;
        double coreSizeX;
        double coreSizeY;
        
        double switchSizeX;
        double switchSizeY;
        
        int meshTileVariant;
        
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
        // tiles are about 450x450 in size
        tile[numOfNodesX*numOfNodesY]: HybridRouter  {
            parameters:
                id = string(index) + ".";
                
                optSwitch = (meshTileVariant == 0) ? "PhotonicMeshGateway" : "PhotonicMeshGatewayCrossbar15Ring" ; // NX style
                //optSwitch = "PhotonicMeshGatewayCrossbar"; // XB style 18 ring
                
                //optSwitch = "PhotonicMeshGatewayCrossbar15Ring"; // XB style 15 ring
                elRouter = 80;
                
                level = "NET";
                numX = numOfNodesX;
                numY = numOfNodesY;
                @display("p=120,120,m,$numOfNodesX,80,80;b=50,50;o=,,1");
        }
        
        lineVert[numOfNodesX*(numOfNodesY-1)] : LineBidirectional
        {
            parameters:
                Length_Line = (meshTileVariant == 0) ? (coreSizeY - (6*switchSizeY)) : 
                	(coreSizeY - 7*(switchSizeY)); //NX style
                //Length_Line = (coreSizeY - 525); //XB style
                //Length_Line = (coreSizeY - 595); //XB15Rings style
            @display("p=120,160,m,$numOfNodesX,80,80;b=5,20;o=,,1");
        }
        lineHoriz[numOfNodesY*(numOfNodesX-1)] : LineBidirectional
        {
            parameters:
                Length_Line = (meshTileVariant == 0) ? (coreSizeX - (6*switchSizeX)) : 
                	(coreSizeY - 7*(switchSizeX));
                //Length_Line = (coreSizeX - 575); //XB style
                //Length_Line = (coreSizeX - 530); //XB15Rings style
                dispValueX = numOfNodesX - 1;
            @display("p=160,120,m,$dispValueX,80,80;b=20,5;o=,,1");
        }
        


    connections allowunconnected:
        for i=0..numOfNodesX-1, for j=0..numOfNodesY-1
        {
            tile[i*numOfNodesY + j].phIn[4] <--> photonicGatewayIn[i*numOfNodesY + j];
            tile[i*numOfNodesY + j].phOut[4] <--> photonicGatewayOut[i*numOfNodesY + j];
            tile[i*numOfNodesY + j].elIn[4] <-- electronicGatewayIn[i*numOfNodesY + j];
            tile[i*numOfNodesY + j].elOut[4] --> electronicGatewayOut[i*numOfNodesY + j];
        }
        
      
         for i=0..numOfNodesX-1, for j=0..numOfNodesY-2 {
            
            
            tile[i+j*numOfNodesX].phIn[2] <--> lineVert[i+j*numOfNodesX].photonicInA;
            lineVert[i+j*numOfNodesX].photonicOutA <--> tile[i+(j+1)*numOfNodesX].phOut[0];
            tile[i+j*numOfNodesX].phOut[2] <--> lineVert[i+j*numOfNodesX].photonicOutB;
            lineVert[i+j*numOfNodesX].photonicInB <--> tile[i+(j+1)*numOfNodesX].phIn[0];
            
            tile[i+j*numOfNodesX].elIn[2] <-- ElectronicChannel <-- tile[i+(j+1)*numOfNodesX].elOut[0];
            tile[i+j*numOfNodesX].elOut[2]  --> ElectronicChannel --> tile[i+(j+1)*numOfNodesX].elIn[0];
           
        }

        for i=0..numOfNodesX-2, for j=0..numOfNodesY-1 {
           
            
            tile[i+j*numOfNodesX].phIn[1] <--> lineHoriz[i+j*(numOfNodesX-1)].photonicInA;
            lineHoriz[i+j*(numOfNodesX-1)].photonicOutA <--> tile[(i+1)+j*numOfNodesX].phOut[3];
            tile[i+j*numOfNodesX].phOut[1] <--> lineHoriz[i+j*(numOfNodesX-1)].photonicInB;
            lineHoriz[i+j*(numOfNodesX-1)].photonicOutB <--> tile[(i+1)+j*numOfNodesX].phIn[3];
            
            tile[i+j*numOfNodesX].elIn[1] <-- ElectronicChannel <-- tile[(i+1)+j*numOfNodesX].elOut[3];
            tile[i+j*numOfNodesX].elOut[1]  --> ElectronicChannel --> tile[(i+1)+j*numOfNodesX].elIn[3];
            
        }
        
        for i=0..numOfNodesX-1
        {
            tile[i].phIn[0] <--> photonicMemoryIn[i];
            tile[i].phOut[0] <--> photonicMemoryOut[i];
            tile[i].elIn[0] <-- electronicMemoryIn[i];
            tile[i].elOut[0] --> electronicMemoryOut[i];
            
            tile[i + numOfNodesX*(numOfNodesY - 1)].phIn[2] <--> photonicMemoryIn[i + numOfNodesX*(numOfNodesY - 1)];
            tile[i + numOfNodesX*(numOfNodesY - 1)].phOut[2] <--> photonicMemoryOut[i + numOfNodesX*(numOfNodesY - 1)];
            tile[i + numOfNodesX*(numOfNodesY - 1)].elIn[2] <-- electronicMemoryIn[i + numOfNodesX*(numOfNodesY - 1)];
            tile[i + numOfNodesX*(numOfNodesY - 1)].elOut[2] --> electronicMemoryOut[i + numOfNodesX*(numOfNodesY - 1)];
        }
        
        for i=1..numOfNodesY-2
        {
            tile[i*numOfNodesX].phIn[3] <--> photonicMemoryIn[i*numOfNodesX];
            tile[i*numOfNodesX].phOut[3] <--> photonicMemoryOut[i*numOfNodesX];
            tile[i*numOfNodesX].elIn[3] <-- electronicMemoryIn[i*numOfNodesX];
            tile[i*numOfNodesX].elOut[3] --> electronicMemoryOut[i*numOfNodesX];
            
            tile[i*numOfNodesX + (numOfNodesX-1)].phIn[1] <--> photonicMemoryIn[i*numOfNodesX + (numOfNodesX-1)];
            tile[i*numOfNodesX + (numOfNodesX-1)].phOut[1] <--> photonicMemoryOut[i*numOfNodesX + (numOfNodesX-1)];
            tile[i*numOfNodesX + (numOfNodesX-1)].elIn[1] <-- electronicMemoryIn[i*numOfNodesX + (numOfNodesX-1)];
            tile[i*numOfNodesX + (numOfNodesX-1)].elOut[1] --> electronicMemoryOut[i*numOfNodesX + (numOfNodesX-1)];
        }

}



