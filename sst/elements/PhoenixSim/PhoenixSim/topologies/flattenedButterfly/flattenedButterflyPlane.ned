
package topologies.flattenedButterfly;

//import topologies.electronicMesh.ElectronicMeshNode;
import electronicComponents.ElectronicRouter;
import electronicComponents.ElectronicChannel;



module FlattenedButterflyPlane
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
        channel ElectronicChannel_2_0 extends electronicComponents.ElectronicChannel  //connections to the middle
        {
            spaceLengths = 2;
            routerLengths = 0;
        }
           
       
        
        channel ElectronicChannel_4_1 extends electronicComponents.ElectronicChannel  //connections to the middle
        {
            spaceLengths = 4;
            routerLengths = 1;
        }
           
       
        
          channel ElectronicChannel_6_2 extends electronicComponents.ElectronicChannel  //connections to the middle
        {
            spaceLengths = 6;
            routerLengths = 2;
        }
           
       
       
    submodules:
        node[(numOfNodesX)*(numOfNodesY)]: ElectronicRouter {
            parameters:
            	numPorts = (numOfNodesX-1) + (numOfNodesY-1) + 2;
        		numPSEports = 0;
        		type = 26;
                id = networkPrefix + string(index) + ".";
                level = "NET";
                
                numX = numOfNodesX;
                numY = numOfNodesY;
 
        }


    connections allowunconnected:
        

        //horizontal connections
        for i=0..numOfNodesX-1, for j=0..numOfNodesY-1, for q=0..numOfNodesX-1 {
            node[i+j*numOfNodesX].portOut[q] --> ElectronicChannel_2_0 --> node[q+j*numOfNodesX].portIn[i-1] if (q < i) && (i-q == 1);
          
            
            node[i+j*numOfNodesX].portOut[q-1] --> ElectronicChannel_2_0 --> node[q+j*numOfNodesX].portIn[i] if (q > i) && (q-i == 1);
            
            
            
            node[i+j*numOfNodesX].portOut[q] --> ElectronicChannel_4_1 --> node[q+j*numOfNodesX].portIn[i-1] if (q < i) && (i-q == 2);
          
            
            node[i+j*numOfNodesX].portOut[q-1] --> ElectronicChannel_4_1 --> node[q+j*numOfNodesX].portIn[i] if (q > i) && (q-i ==2);
             
            
            node[i+j*numOfNodesX].portOut[q] --> ElectronicChannel_6_2 --> node[q+j*numOfNodesX].portIn[i-1] if (q < i) && (i-q == 3);
         
            
            node[i+j*numOfNodesX].portOut[q-1] --> ElectronicChannel_6_2 --> node[q+j*numOfNodesX].portIn[i] if (q > i) && (q-i == 3);

        }
        
         //vertical connections
        for i=0..numOfNodesX-1, for j=0..numOfNodesY-1, for q=0..numOfNodesY-1 {
            node[i+j*numOfNodesX].portOut[numOfNodesX-1+q] --> ElectronicChannel_2_0 --> node[i+q*numOfNodesX].portIn[numOfNodesX-1+j-1] if (q < j) && (j-q == 1);
            
            node[i+j*numOfNodesX].portOut[numOfNodesX-1+q-1] --> ElectronicChannel_2_0 --> node[i+q*numOfNodesX].portIn[numOfNodesX-1+j] if (q > j) && (q-j == 1);
            
            
            node[i+j*numOfNodesX].portOut[numOfNodesX-1+q] --> ElectronicChannel_4_1 --> node[i+q*numOfNodesX].portIn[numOfNodesX-1+j-1] if (q < j) && (j-q == 2);
            
            node[i+j*numOfNodesX].portOut[numOfNodesX-1+q-1] --> ElectronicChannel_4_1 --> node[i+q*numOfNodesX].portIn[numOfNodesX-1+j] if (q > j) && (q-j == 2);
            
            
            node[i+j*numOfNodesX].portOut[numOfNodesX-1+q] --> ElectronicChannel_6_2 --> node[i+q*numOfNodesX].portIn[numOfNodesX-1+j-1] if (q < j) && (j-q == 3);
             
            node[i+j*numOfNodesX].portOut[numOfNodesX-1+q-1] --> ElectronicChannel_6_2 --> node[i+q*numOfNodesX].portIn[numOfNodesX-1+j] if (q > j) && (q-j == 3);
            
        }
       

       // for i=0..numOfNodesX*2-1, for j=0..numOfNodesY*2-1 {
           // node[(i>>1)+(j>>1)*numOfNodesX].portIn[6] <-- ElectronicChannel{power = false;} <-- electronicGatewayIn[i+(j)*numOfNodesX*2] if (j%2 == 0) && (i%2 == 0);
           // node[(i>>1)+(j>>1)*numOfNodesX].portOut[6] --> ElectronicChannel{power = false;} --> electronicGatewayOut[i+j*numOfNodesX*2] if (j%2 == 0) && (i%2 == 0);
            
            
            //node[(i>>1)+(j>>1)*numOfNodesX].portIn[7] <-- ElectronicChannel{power = false;} <-- electronicGatewayIn[i+(j)*numOfNodesX*2] if (j%2 == 0) && (i%2 == 1);
            //node[(i>>1)+(j>>1)*numOfNodesX].portOut[7] --> ElectronicChannel{power = false;} --> electronicGatewayOut[i+j*numOfNodesX*2] if (j%2 == 0) && (i%2 == 1);
              
            //node[(i>>1)+(j>>1)*numOfNodesX].portIn[8] <-- ElectronicChannel{power = false;} <-- electronicGatewayIn[i+(j)*numOfNodesX*2] if (j%2 == 1) && (i%2 == 0);
            //node[(i>>1)+(j>>1)*numOfNodesX].portOut[8] --> ElectronicChannel{power = false;} --> electronicGatewayOut[i+(j)*numOfNodesX*2] if (j%2 == 1) && (i%2 == 0);
              
            //node[(i>>1)+(j>>1)*numOfNodesX].portIn[9] <-- ElectronicChannel{power = false;} <-- electronicGatewayIn[i+(j)*numOfNodesX*2] if (j%2 == 1) && (i%2 == 1);
            //node[(i>>1)+(j>>1)*numOfNodesX].portOut[9] --> ElectronicChannel{power = false;} --> electronicGatewayOut[i+(j)*numOfNodesX*2] if (j%2 == 1) && (i%2 == 1);
           
       // }
       
       for i=0..numOfNodesX-1, for j=0..numOfNodesY-1 {
           node[(i)+(j)*numOfNodesX].portIn[numOfNodesX + numOfNodesY-2] <-- ElectronicChannel{power = false;} <-- electronicGatewayIn[i+(j)*numOfNodesX];
           node[(i)+(j)*numOfNodesX].portOut[numOfNodesX + numOfNodesY-2] --> ElectronicChannel{power = false;} --> electronicGatewayOut[i+j*numOfNodesX];
            

       }
       

        //going off chip
        for i=0..numOfNodesX-1 {
            IOin[i] --> node[i].portIn[numOfNodesX + numOfNodesY - 1];
            IOout[i] <-- node[i].portOut[numOfNodesX + numOfNodesY - 1];
           
            

            IOin[i+numOfNodesX] --> node[i+(numOfNodesY-1)*numOfNodesX].portIn[numOfNodesX + numOfNodesY - 1];
            IOout[i+numOfNodesX] <-- node[i+(numOfNodesY-1)*numOfNodesX].portOut[numOfNodesX + numOfNodesY - 1];
            
        }

        for j=1..numOfNodesY-2 {
            IOin[(j-1)+numOfNodesX*2] --> node[j*numOfNodesX].portIn[numOfNodesX + numOfNodesY - 1];
            IOout[(j-1)+numOfNodesX*2] <-- node[j*numOfNodesX].portOut[numOfNodesX + numOfNodesY - 1];
           

            IOin[(j-1)+numOfNodesX*2+numOfNodesY-2] --> node[j*numOfNodesX + numOfNodesX-1].portIn[numOfNodesX + numOfNodesY - 1];
            IOout[(j-1)+numOfNodesX*2+numOfNodesY-2] <-- node[j*numOfNodesX + numOfNodesX-1].portOut[numOfNodesX + numOfNodesY - 1];
          
        }


}

