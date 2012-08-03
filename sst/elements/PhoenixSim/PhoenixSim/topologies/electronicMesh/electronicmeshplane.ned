
package topologies.electronicMesh;

//import topologies.electronicMesh.ElectronicMeshNode;
import electronicComponents.ElectronicRouter;
import electronicComponents.ElectronicChannel;



module ElectronicMeshPlane
{
    parameters:
        int numOfNodesX;
        int numOfNodesY;
        
        string networkPrefix;
        @display("i=OptNet_planes/el_plane_t");
    gates:
        input electronicGatewayIn[numOfNodesX*numOfNodesY];
        input IOin[];
        output electronicGatewayOut[numOfNodesX*numOfNodesY];
        output IOout[];

     

    submodules:
        node[numOfNodesX*numOfNodesY]: ElectronicRouter {
            parameters:
            	numPorts = 5;
        		numPSEports = 0;
        		type = 20;
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

        //going off chip
        for i=0..numOfNodesX-1 {
            IOin[i] --> node[i].portIn[0];
            IOout[i] <-- node[i].portOut[0];
          
            

            IOin[i+numOfNodesX] --> node[i+(numOfNodesY-1)*numOfNodesX].portIn[2];
            IOout[i+numOfNodesX] <-- node[i+(numOfNodesY-1)*numOfNodesX].portOut[2];
            
        }

        for j=1..numOfNodesY-2 {
            IOin[(j-1)+numOfNodesX*2] --> node[j*numOfNodesX].portIn[3];
            IOout[(j-1)+numOfNodesX*2] <-- node[j*numOfNodesX].portOut[3];
           

            IOin[(j-1)+numOfNodesX*2+numOfNodesY-2] --> node[j*numOfNodesX + numOfNodesX-1].portIn[1];
            IOout[(j-1)+numOfNodesX*2+numOfNodesY-2] <-- node[j*numOfNodesX + numOfNodesX-1].portOut[1];
        }
}

module ElectronicMeshPlane_ConcBuffer
{
    parameters:
        int numOfNodesX;
        int numOfNodesY;
        
        int processorConcentration;
        
        string networkPrefix;
        @display("i=OptNet_planes/el_plane_t");
    gates:
        input electronicGatewayIn[numOfNodesX*numOfNodesY*processorConcentration];
        input IOin[];
        output electronicGatewayOut[numOfNodesX*numOfNodesY*processorConcentration];
        output IOout[];

     

    submodules:
        node[numOfNodesX*numOfNodesY]: ElectronicRouter {
            parameters:
            	numPorts = 4 + processorConcentration;
        		numPSEports = 0;
        		type = 20;
                id = networkPrefix + string(index) + ".";
                level = "NET";
                switchVariant = 1;
                          
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

        for i=0..numOfNodesX-1, for j=0..numOfNodesY-1, for k=0..processorConcentration-1 {
            node[i+j*numOfNodesX].portIn[4+k]  <-- ElectronicChannel {power = false;} <-- electronicGatewayIn[k+i*processorConcentration+j*numOfNodesX*processorConcentration];
            node[i+j*numOfNodesX].portOut[4+k]  --> ElectronicChannel {power = false;} --> electronicGatewayOut[k+i*processorConcentration+j*numOfNodesX*processorConcentration];
         
       
        }

        //going off chip
        for i=0..numOfNodesX-1 {
            IOin[i] --> node[i].portIn[0];
            IOout[i] <-- node[i].portOut[0];
          
            

            IOin[i+numOfNodesX] --> node[i+(numOfNodesY-1)*numOfNodesX].portIn[2];
            IOout[i+numOfNodesX] <-- node[i+(numOfNodesY-1)*numOfNodesX].portOut[2];
            
        }

        for j=1..numOfNodesY-2 {
            IOin[(j-1)+numOfNodesX*2] --> node[j*numOfNodesX].portIn[3];
            IOout[(j-1)+numOfNodesX*2] <-- node[j*numOfNodesX].portOut[3];
           

            IOin[(j-1)+numOfNodesX*2+numOfNodesY-2] --> node[j*numOfNodesX + numOfNodesX-1].portIn[1];
            IOout[(j-1)+numOfNodesX*2+numOfNodesY-2] <-- node[j*numOfNodesX + numOfNodesX-1].portOut[1];
        }
}

