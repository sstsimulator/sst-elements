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

package topologies.photonicTDM;

import photonic.devices.Line;
import topologies.ModulatorBank;
import topologies.DetectorBank;

import topologies.Gateway;

module PhotonicMeshTDMPlane
{
    parameters:
        int numOfNodesX;
        int numOfNodesY;
        double coreSizeX;
        double coreSizeY;
        
      	string tdmSwitch;
        
        @display("i=planes/ph_plane");
    gates:
        inout photonicIn[numOfNodesX*numOfNodesY];
        inout photonicOut[numOfNodesX*numOfNodesY];
        
      
      
    submodules:
        // tiles are about 450x450 in size
        tile[numOfNodesX*numOfNodesY]: <tdmSwitch> like TDM_SWITCH  {
            parameters:
                id = index;
            gates:
               photonicPort[10];
        }
        
       // ModBank[numOfNodesX*numOfNodesY]: ModulatorBank {
       //     @display("p=143,277");
       // }

       // DetBank[numOfNodesX*numOfNodesY]: DetectorBank {
       //     @display("p=351,102;is=vl");
       // }
       
     
        
        lineVert[2*numOfNodesX*(numOfNodesY-1)] : Line
        {
            parameters:
                Length_Line = coreSizeY - tile[0].sizeY;
            
        }
        lineHoriz[2*numOfNodesY*(numOfNodesX-1)] : Line
        {
            parameters:
                Length_Line = coreSizeX - tile[0].sizeX;
              
               
        }
        


    connections allowunconnected:
        for i=0..numOfNodesX-1, for j=0..numOfNodesY-1
        {
            //DetBank[i*numOfNodesY + j].electronicOut <--> photonicGatewayOut[i*numOfNodesY + j];
           // ModBank[i*numOfNodesY + j].electronicIn <--> photonicGatewayIn[i*numOfNodesY + j];
            
            //DetBank[i*numOfNodesY + j].networkIn <--> tile[i*numOfNodesY + j].photonicPort[2];
            //ModBank[i*numOfNodesY + j].networkOut <--> tile[i*numOfNodesY + j].photonicPort[1];
            
         
            photonicOut[i*numOfNodesY + j] <--> tile[i*numOfNodesY + j].photonicPort[1];
            photonicIn[i*numOfNodesY + j] <--> tile[i*numOfNodesY + j].photonicPort[0];
           
        }
        
        //horizontals
       for i=0..numOfNodesX-2, for j=0..numOfNodesY-1, for k=numOfNodesY*(numOfNodesX-1)..numOfNodesY*(numOfNodesX-1)
        {
            tile[i + j*numOfNodesX].photonicPort[5] <--> lineHoriz[i + j*(numOfNodesX-1)].photonicIn;
            lineHoriz[i + j*(numOfNodesX-1)].photonicOut <--> tile[i+1 + j*numOfNodesX].photonicPort[8];
            tile[i + j*numOfNodesX].photonicPort[4] <--> lineHoriz[k+ i + j*(numOfNodesX-1)].photonicOut;
             lineHoriz[k + i + j*(numOfNodesX-1)].photonicIn <--> tile[i+1 + j*numOfNodesX].photonicPort[9];
        }
        
         //verticals
       for i=0..numOfNodesX-1, for j=0..numOfNodesY-2, for k=numOfNodesX*(numOfNodesY-1)..numOfNodesX*(numOfNodesY-1)
        {
            tile[i + j*numOfNodesX].photonicPort[7] <--> lineVert[i + j*numOfNodesX].photonicIn;
            lineVert[i + j*numOfNodesX].photonicOut <--> tile[i + (j+1)*numOfNodesX].photonicPort[2];
            tile[i + j*numOfNodesX].photonicPort[6] <--> lineVert[numOfNodesX*(numOfNodesY-1)+ i + j*numOfNodesX].photonicOut;
             lineVert[numOfNodesX*(numOfNodesY-1) + i + j*numOfNodesX].photonicIn <--> tile[i + (j+1)*numOfNodesX].photonicPort[3];
        }
        
      
        

}
