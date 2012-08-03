
package topologies.CMesh;

//import topologies.electronicMesh.ElectronicMeshNode;
import electronicComponents.ElectronicRouter;
import electronicComponents.ElectronicChannel;



module CMeshPlane
{
    parameters:
        int numOfNodesX;
        int numOfNodesY;
        
        string networkPrefix;
        @display("i=OptNet_planes/el_plane_t");
    gates:
        input electronicGatewayIn[];
        input IOin[];
        output electronicGatewayOut[];
        output IOout[];

 	types:
        channel ElectronicChannel_2_1 extends electronicComponents.ElectronicChannel  //connections to the middle
        {
            spaceLengths = 2;
            routerLengths = 1;
        }
     

    submodules:
        node[numOfNodesX*numOfNodesY]: ElectronicRouter {
            parameters:
            	numPorts = 5;
        		numPSEports = 0;
        		type = 27;
                id = networkPrefix + string(index) + ".";
                level = "NET";
                          
                numX = numOfNodesX;
                numY = numOfNodesY;
                
            
        }


    connections allowunconnected:
        for i=0..numOfNodesX-1, for j=0..numOfNodesY-2 {
            node[i+j*numOfNodesX].portIn[2] <-- ElectronicChannel <-- node[i+(j+1)*numOfNodesX].portOut[0];
            node[i+j*numOfNodesX].portOut[2] --> ElectronicChannel --> node[i+(j+1)*numOfNodesX].portIn[0];
            
           
        }

        for i=0..numOfNodesX-2, for j=0..numOfNodesY-1 {
            node[i+j*numOfNodesX].portIn[1] <-- ElectronicChannel <-- node[(i+1)+j*numOfNodesX].portOut[3];
            node[i+j*numOfNodesX].portOut[1] --> ElectronicChannel --> node[(i+1)+j*numOfNodesX].portIn[3];
            
        }

        for i=0..numOfNodesX-1, for j=0..numOfNodesY-1 {
            node[i+j*numOfNodesX].portIn[4]  <-- ElectronicChannel {power = false;} <-- electronicGatewayIn[i+j*numOfNodesX];
            node[i+j*numOfNodesX].portOut[4]  --> ElectronicChannel {power = false;} --> electronicGatewayOut[i+j*numOfNodesX];
         
       
        }
        
        
        //express channels around the perimeter
        for i=0..numOfNodesX-3 {
         	node[i].portIn[0] <-- ElectronicChannel_2_1 <-- node[i+2].portOut[0] if (i % 4 == 0 || i % 4 == 1);
            node[i].portOut[0] --> ElectronicChannel_2_1 --> node[i+2].portIn[0] if (i % 4 == 0 || i % 4 == 1);
            
            node[i+numOfNodesX*(numOfNodesY-1)].portIn[2] <-- ElectronicChannel_2_1 <-- node[i+numOfNodesX*(numOfNodesY-1)+2].portOut[2] if (i % 4 == 0 || i % 4 == 1);
            node[i+numOfNodesX*(numOfNodesY-1)].portOut[2] --> ElectronicChannel_2_1 --> node[i+numOfNodesX*(numOfNodesY-1)+2].portIn[2] if (i % 4 == 0 || i % 4 == 1);
        }
        
        for i=0..numOfNodesY-3 {
         	node[numOfNodesX*i].portIn[3] <-- ElectronicChannel_2_1 <-- node[numOfNodesX*(i+2)].portOut[3] if (i % 4 == 0 || i % 4 == 1);
            node[numOfNodesX*i].portOut[3] --> ElectronicChannel_2_1 --> node[numOfNodesX*(i+2)].portIn[3] if (i % 4 == 0 || i % 4 == 1);
            
            node[numOfNodesX*i + numOfNodesX-1].portIn[1] <-- ElectronicChannel_2_1 <-- node[numOfNodesX*(i+2) + numOfNodesX-1].portOut[1] if (i % 4 == 0 || i % 4 == 1);
            node[numOfNodesX*i + numOfNodesX-1].portOut[1] --> ElectronicChannel_2_1 --> node[numOfNodesX*(i+2) + numOfNodesX-1].portIn[1] if (i % 4 == 0 || i % 4 == 1);
        }
        
        
        //going off chip
//        for i=0..numOfNodesX-1 {
//            IOin[i] --> node[i].portIn[5];
//            IOout[i] <-- node[i].portOut[5];
//           
//            
//
//            IOin[i+numOfNodesX] --> node[i+(numOfNodesY-1)*numOfNodesX].portIn[5];
//            IOout[i+numOfNodesX] <-- node[i+(numOfNodesY-1)*numOfNodesX].portOut[5];
//          
//        }
//
//        for j=1..numOfNodesY-2 {
//            IOin[(j-1)+numOfNodesX*2] --> node[j*numOfNodesX].portIn[5];
//            IOout[(j-1)+numOfNodesX*2] <-- node[j*numOfNodesX].portOut[5];
//          
//
//            IOin[(j-1)+numOfNodesX*2+numOfNodesY-2] --> node[j*numOfNodesX + numOfNodesX-1].portIn[5];
//            IOout[(j-1)+numOfNodesX*2+numOfNodesY-2] <-- node[j*numOfNodesX + numOfNodesX-1].portOut[5];
//          
//        }

     
}

