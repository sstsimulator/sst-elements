package topologies.blockingTorus;

import topologies.blockingTorus.TorusTile*;
import photonic.devices.UTurn;
import photonic.devices.UTurnBidirectional;
import electronicComponents.ElectronicChannel;


module PhotonicPlane
{
    parameters:
        int numOfNodesX;
        int numOfNodesY;
        
        int level;
        string networkPrefix;
       
        
        @display("i=planes/ph_plane");
    gates:
        inout photonicGatewayIn[];
        inout photonicGatewayOut[];

        inout photonicMemoryIn[];
        inout photonicMemoryOut[];
        
        input electronicGatewayIn[];
        output electronicGatewayOut[];

        input electronicMemoryIn[];
        output electronicMemoryOut[];
        
   


    submodules:
        tileNW[(numOfNodesX>>1)*(numOfNodesY>>1)]: TorusTileNW {
            parameters:
                
              	level = level;
            	networkPrefix = networkPrefix;
                
                numX = numOfNodesX;
                numY = numOfNodesY;
				nodeId = index*2 + floor(index*2/numOfNodesX)*numOfNodesX;
                disp_Width = (numOfNodesX>>1)+(numOfNodesX%2);
               
                @display("p=120,120,m,$disp_Width,160,160;b=50,50;o=,,1");
                
           
        		
        }

        tileNE[(numOfNodesX>>1)*(numOfNodesY>>1)]: TorusTileNE {
            parameters:
                level = level;
            networkPrefix = networkPrefix;
                numX = numOfNodesX;
                numY = numOfNodesY;
				nodeId = index*2+1 + floor((index*2+1)/numOfNodesX)*numOfNodesX;
                disp_Width = numOfNodesX>>1;
                
                @display("p=200,120,m,$disp_Width,160,160;b=50,50;o=,,1");
                      
          
        }

        tileSE[(numOfNodesX>>1)*(numOfNodesY>>1)]: TorusTileSE {
            parameters:
                level = level;
            networkPrefix = networkPrefix;
                numX = numOfNodesX;
                numY = numOfNodesY;
				nodeId = index*2+1 + floor(((index*2+1)/numOfNodesX)+1)*numOfNodesX;
                disp_Width = numOfNodesX>>1;
             
                @display("p=200,200,m,$disp_Width,160,160;b=50,50;o=,,1");
                      
          
        }

        tileSW[((numOfNodesX>>1)+numOfNodesX%2)*(numOfNodesY>>1)]: TorusTileSW {
            parameters:
                level = level;
            networkPrefix = networkPrefix;
                numX = numOfNodesX;
                numY = numOfNodesY;
				nodeId = index*2 + floor((index*2/numOfNodesX)+1)*numOfNodesX;
                disp_Width = (numOfNodesX>>1)+(numOfNodesX%2);
              
                @display("p=120,200,m,$disp_Width,160,160;b=50,50;o=,,1");
                      
        
        }

        uturnN[numOfNodesX]: UTurnBidirectional {
            parameters:
                disp_Width = 0;
                @display("p=120,70,r,80;b=50,10;o=,,1");
        }

        uturnS[numOfNodesX]: UTurnBidirectional {
            parameters:
                disp_Width = 40 + numOfNodesY*80 + 50;
                @display("p=120,$disp_Width,r,80;b=50,10;o=,,1");
        }

        uturnE[numOfNodesY]: UTurnBidirectional {
            parameters:
                disp_Width = 40 + numOfNodesX*80 + 50;
                @display("p=$disp_Width,120,c,80;b=10,50;o=,,1");
        }

        uturnW[numOfNodesY]: UTurnBidirectional {
            parameters:
                disp_Width = 0;
                @display("p=70,120,c,80;b=10,50;o=,,1");
        }

        uturnInjectionN[numOfNodesX]: UTurnBidirectional {
            parameters:
                disp_Width = 0;
                @display("p=120,70,r,80;b=50,10;o=,,1");
        }

        uturnInjectionS[numOfNodesX]: UTurnBidirectional {
            parameters:
                disp_Width = 40 + numOfNodesY*80 + 50;
                @display("p=120,$disp_Width,r,80;b=50,10;o=,,1");
        }

        uturnEjectionE[numOfNodesY]: UTurnBidirectional {
            parameters:
                PathSeparation = 0;
                disp_Width = 40 + numOfNodesX*80 + 50;
                @display("p=$disp_Width,120,c,80;b=10,50;o=,,1");
        }

        uturnEjectionW[numOfNodesY]: UTurnBidirectional {
            parameters:
                PathSeparation = 0;
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
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicETopOut --> ElectronicChannel --> tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicWTopIn if i%2==0 && j%2==0 && i!=(numOfNodesX-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicETopIn <-- ElectronicChannel <-- tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicWTopOut if i%2==0 && j%2==0 && i!=(numOfNodesX-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicEBottomOut --> ElectronicChannel --> tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicWBottomIn if i%2==0 && j%2==0 && i!=(numOfNodesX-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicEBottomIn <-- ElectronicChannel <-- tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicWBottomOut if i%2==0 && j%2==0 && i!=(numOfNodesX-1);

            // NW -> SW Main Torus
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSLeftOut --> ElectronicChannel --> tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNLeftIn if i%2==0 && j%2==0 && j!=(numOfNodesY-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSLeftIn <-- ElectronicChannel <-- tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNLeftOut if i%2==0 && j%2==0 && j!=(numOfNodesY-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSRightOut --> ElectronicChannel --> tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNRightIn if i%2==0 && j%2==0 && j!=(numOfNodesY-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSRightIn <-- ElectronicChannel <-- tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNRightOut if i%2==0 && j%2==0 && j!=(numOfNodesY-1);

            // SW -> SE Main Torus
            tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicETopOut --> ElectronicChannel --> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicWTopIn if i%2==0 && j%2!=0 && i!=(numOfNodesX-1);
            tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicETopIn <-- ElectronicChannel <-- tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicWTopOut if i%2==0 && j%2!=0 && i!=(numOfNodesX-1);
            tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicEBottomOut --> ElectronicChannel --> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicWBottomIn if i%2==0 && j%2!=0 && i!=(numOfNodesX-1);
            tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicEBottomIn <-- ElectronicChannel <-- tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicWBottomOut if i%2==0 && j%2!=0 && i!=(numOfNodesX-1);

            // NE -> SE Main Torus
            tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSLeftOut --> ElectronicChannel --> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNLeftIn if i%2!=0 && j%2==0 && j!=(numOfNodesY-1);
            tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSLeftIn <-- ElectronicChannel <-- tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNLeftOut if i%2!=0 && j%2==0 && j!=(numOfNodesY-1);
            tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSRightOut --> ElectronicChannel --> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNRightIn if i%2!=0 && j%2==0 && j!=(numOfNodesY-1);
            tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSRightIn <-- ElectronicChannel <-- tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNRightOut if i%2!=0 && j%2==0 && j!=(numOfNodesY-1);

         
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

            // SW -> NW Injection Lane
            tileSW[(i) + (j)*(numOfNodesX>>1)].photonicSLeftInjectionOut <--> tileNW[(i) + ((j) + 1)*(numOfNodesX>>1)].photonicNLeftInjectionIn;
            tileSW[(i) + (j)*(numOfNodesX>>1)].photonicSLeftInjectionIn <--> tileNW[(i) + ((j) + 1)*(numOfNodesX>>1)].photonicNLeftInjectionOut;
            tileSW[(i) + (j)*(numOfNodesX>>1)].photonicSRightInjectionOut <--> tileNW[(i) + ((j) + 1)*(numOfNodesX>>1)].photonicNRightInjectionIn;
            tileSW[(i) + (j)*(numOfNodesX>>1)].photonicSRightInjectionIn <--> tileNW[(i) + ((j) + 1)*(numOfNodesX>>1)].photonicNRightInjectionOut;

            // SE -> NE Injection Lane
            tileSE[(i) + (j)*(numOfNodesX>>1)].photonicSLeftInjectionOut <--> tileNE[(i) + ((j) + 1)*(numOfNodesX>>1)].photonicNLeftInjectionIn;
            tileSE[(i) + (j)*(numOfNodesX>>1)].photonicSLeftInjectionIn <--> tileNE[(i) + ((j) + 1)*(numOfNodesX>>1)].photonicNLeftInjectionOut;
            tileSE[(i) + (j)*(numOfNodesX>>1)].photonicSRightInjectionOut <--> tileNE[(i) + ((j) + 1)*(numOfNodesX>>1)].photonicNRightInjectionIn;
            tileSE[(i) + (j)*(numOfNodesX>>1)].photonicSRightInjectionIn <--> tileNE[(i) + ((j) + 1)*(numOfNodesX>>1)].photonicNRightInjectionOut;


			// SW -> NW Main Torus
            tileSW[(i) + (j)*(numOfNodesX>>1)].electronicSLeftOut --> ElectronicChannel --> tileNW[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNLeftIn;
            tileSW[(i) + (j)*(numOfNodesX>>1)].electronicSLeftIn <-- ElectronicChannel <-- tileNW[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNLeftOut;
            tileSW[(i) + (j)*(numOfNodesX>>1)].electronicSRightOut --> ElectronicChannel --> tileNW[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNRightIn;
            tileSW[(i) + (j)*(numOfNodesX>>1)].electronicSRightIn <-- ElectronicChannel <-- tileNW[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNRightOut;

            // SE -> NE Main Torus
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicSLeftOut --> ElectronicChannel --> tileNE[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNLeftIn;
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicSLeftIn <-- ElectronicChannel <-- tileNE[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNLeftOut;
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicSRightOut --> ElectronicChannel --> tileNE[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNRightIn;
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicSRightIn <-- ElectronicChannel <-- tileNE[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNRightOut;

            // SW -> NW Injection Lane
            tileSW[(i) + (j)*(numOfNodesX>>1)].electronicSLeftInjectionOut --> ElectronicChannel --> tileNW[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNLeftInjectionIn;
            tileSW[(i) + (j)*(numOfNodesX>>1)].electronicSLeftInjectionIn <-- ElectronicChannel <-- tileNW[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNLeftInjectionOut;
            tileSW[(i) + (j)*(numOfNodesX>>1)].electronicSRightInjectionOut --> ElectronicChannel --> tileNW[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNRightInjectionIn;
            tileSW[(i) + (j)*(numOfNodesX>>1)].electronicSRightInjectionIn <-- ElectronicChannel <-- tileNW[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNRightInjectionOut;

            // SE -> NE Injection Lane
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicSLeftInjectionOut --> ElectronicChannel --> tileNE[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNLeftInjectionIn;
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicSLeftInjectionIn <-- ElectronicChannel <-- tileNE[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNLeftInjectionOut;
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicSRightInjectionOut --> ElectronicChannel --> tileNE[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNRightInjectionIn;
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicSRightInjectionIn <-- ElectronicChannel <-- tileNE[(i) + ((j) + 1)*(numOfNodesX>>1)].electronicNRightInjectionOut;


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

            // NE -> NW Injection Lane
            tileNE[(i) + (j)*(numOfNodesX>>1)].photonicEEjectionOut <--> tileNW[(i) + 1 + (j)*(numOfNodesX>>1)].photonicWEjectionIn;
            tileNE[(i) + (j)*(numOfNodesX>>1)].photonicEEjectionIn <--> tileNW[(i) + 1 + (j)*(numOfNodesX>>1)].photonicWEjectionOut;

            // SE -> SW Injection Lane
            tileSE[(i) + (j)*(numOfNodesX>>1)].photonicEEjectionOut <--> tileSW[(i) + 1 + (j)*(numOfNodesX>>1)].photonicWEjectionIn;
            tileSE[(i) + (j)*(numOfNodesX>>1)].photonicEEjectionIn <--> tileSW[(i) + 1 + (j)*(numOfNodesX>>1)].photonicWEjectionOut;



// NE -> NW Main Torus
            tileNE[(i) + (j)*(numOfNodesX>>1)].electronicETopOut --> ElectronicChannel --> tileNW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWTopIn;
            tileNE[(i) + (j)*(numOfNodesX>>1)].electronicETopIn <-- ElectronicChannel <-- tileNW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWTopOut;
            tileNE[(i) + (j)*(numOfNodesX>>1)].electronicEBottomOut --> ElectronicChannel --> tileNW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWBottomIn;
            tileNE[(i) + (j)*(numOfNodesX>>1)].electronicEBottomIn <-- ElectronicChannel <-- tileNW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWBottomOut;

            // SE -> SW Main Torus
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicETopOut --> ElectronicChannel --> tileSW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWTopIn;
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicETopIn <-- ElectronicChannel <-- tileSW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWTopOut;
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicEBottomOut --> ElectronicChannel --> tileSW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWBottomIn;
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicEBottomIn <-- ElectronicChannel <-- tileSW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWBottomOut;

            // NE -> NW Injection Lane
            tileNE[(i) + (j)*(numOfNodesX>>1)].electronicETopEjectionOut --> ElectronicChannel --> tileNW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWTopEjectionIn;
            tileNE[(i) + (j)*(numOfNodesX>>1)].electronicEBottomEjectionOut --> ElectronicChannel --> tileNW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWBottomEjectionIn;
            tileNE[(i) + (j)*(numOfNodesX>>1)].electronicETopEjectionIn <-- ElectronicChannel <-- tileNW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWTopEjectionOut;
            tileNE[(i) + (j)*(numOfNodesX>>1)].electronicEBottomEjectionIn <-- ElectronicChannel <-- tileNW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWBottomEjectionOut;

            // SE -> SW Injection Lane
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicETopEjectionOut --> ElectronicChannel --> tileSW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWTopEjectionIn;
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicEBottomEjectionOut --> ElectronicChannel --> tileSW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWBottomEjectionIn;
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicETopEjectionIn <-- ElectronicChannel <-- tileSW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWTopEjectionOut;
            tileSE[(i) + (j)*(numOfNodesX>>1)].electronicEBottomEjectionIn <-- ElectronicChannel <-- tileSW[(i) + 1 + (j)*(numOfNodesX>>1)].electronicWBottomEjectionOut;


        }

        for i=0..numOfNodesX-1, for j=0..numOfNodesY-1 {
            // NW -> SW Injection Lane
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicSLeftInjectionOut <--> tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicNLeftInjectionIn if i%2==0 && j%2==0 && j!=(numOfNodesY-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicSLeftInjectionIn <--> tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicNLeftInjectionOut if i%2==0 && j%2==0 && j!=(numOfNodesY-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicSRightInjectionOut <--> tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicNRightInjectionIn if i%2==0 && j%2==0 && j!=(numOfNodesY-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicSRightInjectionIn <--> tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicNRightInjectionOut if i%2==0 && j%2==0 && j!=(numOfNodesY-1);


            // NE -> SE Injection Lane
            tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicSLeftInjectionOut <--> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicNLeftInjectionIn if i%2!=0 && j%2==0 && j!=(numOfNodesY-1);
            tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicSLeftInjectionIn <--> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicNLeftInjectionOut if i%2!=0 && j%2==0 && j!=(numOfNodesY-1);
            tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicSRightInjectionOut <--> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicNRightInjectionIn if i%2!=0 && j%2==0 && j!=(numOfNodesY-1);
            tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicSRightInjectionIn <--> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].photonicNRightInjectionOut if i%2!=0 && j%2==0 && j!=(numOfNodesY-1);


// NW -> SW Injection Lane
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSLeftInjectionOut --> ElectronicChannel --> tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNLeftInjectionIn if i%2==0 && j%2==0 && j!=(numOfNodesY-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSLeftInjectionIn <-- ElectronicChannel <-- tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNLeftInjectionOut if i%2==0 && j%2==0 && j!=(numOfNodesY-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSRightInjectionOut --> ElectronicChannel --> tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNRightInjectionIn if i%2==0 && j%2==0 && j!=(numOfNodesY-1);
            tileNW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSRightInjectionIn <-- ElectronicChannel <-- tileSW[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNRightInjectionOut if i%2==0 && j%2==0 && j!=(numOfNodesY-1);


            // NE -> SE Injection Lane
            tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSLeftInjectionOut --> ElectronicChannel --> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNLeftInjectionIn if i%2!=0 && j%2==0 && j!=(numOfNodesY-1);
            tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSLeftInjectionIn <-- ElectronicChannel <-- tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNLeftInjectionOut if i%2!=0 && j%2==0 && j!=(numOfNodesY-1);
            tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSRightInjectionOut --> ElectronicChannel --> tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNRightInjectionIn if i%2!=0 && j%2==0 && j!=(numOfNodesY-1);
            tileNE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicSRightInjectionIn <-- ElectronicChannel <-- tileSE[(i>>1) + (j>>1)*(numOfNodesX>>1)].electronicNRightInjectionOut if i%2!=0 && j%2==0 && j!=(numOfNodesY-1);

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
            
            
            tileNW[(i>>1)*(numOfNodesX>>1)].electronicWTopOut --> ElectronicChannel{@display("m=w;ls=red,0");} --> tileNW[(i>>1)*(numOfNodesX>>1)].electronicWBottomIn if i%2==0;
            tileNW[(i>>1)*(numOfNodesX>>1)].electronicWTopIn <-- ElectronicChannel{@display("m=w;ls=red,0");} <-- tileNW[(i>>1)*(numOfNodesX>>1)].electronicWBottomOut if i%2==0;
            
            tileSW[(i>>1)*(numOfNodesX>>1)].electronicWTopOut --> ElectronicChannel{@display("m=w;ls=red,0");} --> tileSW[(i>>1)*(numOfNodesX>>1)].electronicWBottomIn if i%2!=0;
            tileSW[(i>>1)*(numOfNodesX>>1)].electronicWTopIn <-- ElectronicChannel{@display("m=w;ls=red,0");} <-- tileSW[(i>>1)*(numOfNodesX>>1)].electronicWBottomOut if i%2!=0;
            
      
            

            tileNW[(i>>1)*(numOfNodesX>>1)].photonicWEjectionOut <--> uturnEjectionW[i].photonicInnerBendIn if i%2==0;
            tileNW[(i>>1)*(numOfNodesX>>1)].photonicWEjectionIn <--> uturnEjectionW[i].photonicInnerBendOut if i%2==0;

            tileSW[(i>>1)*(numOfNodesX>>1)].photonicWEjectionOut <--> uturnEjectionW[i].photonicInnerBendIn if i%2!=0;
            tileSW[(i>>1)*(numOfNodesX>>1)].photonicWEjectionIn <--> uturnEjectionW[i].photonicInnerBendOut if i%2!=0;
            
            tileNW[(i>>1)*(numOfNodesX>>1)].electronicWBottomEjectionOut --> ElectronicChannel{@display("m=w;ls=red,0");} --> tileNW[(i>>1)*(numOfNodesX>>1)].electronicWTopEjectionIn if i%2==0;
            tileNW[(i>>1)*(numOfNodesX>>1)].electronicWBottomEjectionIn <-- ElectronicChannel{@display("m=w;ls=red,0");} <-- tileNW[(i>>1)*(numOfNodesX>>1)].electronicWTopEjectionOut if i%2==0;

			tileSW[(i>>1)*(numOfNodesX>>1)].electronicWBottomEjectionOut --> ElectronicChannel{@display("m=w;ls=red,0");} --> tileSW[(i>>1)*(numOfNodesX>>1)].electronicWTopEjectionIn if i%2!=0;
            tileSW[(i>>1)*(numOfNodesX>>1)].electronicWBottomEjectionIn <-- ElectronicChannel{@display("m=w;ls=red,0");} <-- tileSW[(i>>1)*(numOfNodesX>>1)].electronicWTopEjectionOut if i%2!=0;

	
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
		

            tileNE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].photonicEEjectionOut <--> uturnEjectionE[i*2].photonicInnerBendIn;
            tileNE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].photonicEEjectionIn <--> uturnEjectionE[i*2].photonicInnerBendOut;

            tileSE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].photonicEEjectionOut <--> uturnEjectionE[i*2+1].photonicInnerBendIn;
            tileSE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].photonicEEjectionIn <--> uturnEjectionE[i*2+1].photonicInnerBendOut;

			tileNE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].electronicETopEjectionOut --> ElectronicChannel{@display("m=w;ls=red,0");} --> tileNE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].electronicEBottomEjectionIn;
			tileNE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].electronicETopEjectionIn <-- ElectronicChannel{@display("m=w;ls=red,0");} <-- tileNE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].electronicEBottomEjectionOut;
			
			tileSE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].electronicETopEjectionOut --> ElectronicChannel{@display("m=w;ls=red,0");} --> tileSE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].electronicEBottomEjectionIn;
			tileSE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].electronicETopEjectionIn <-- ElectronicChannel{@display("m=w;ls=red,0");} <-- tileSE[(numOfNodesX/2) - 1 + (i)*(numOfNodesX>>1)].electronicEBottomEjectionOut;

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
            
            tileNW[(i>>1)].electronicNLeftOut --> ElectronicChannel{@display("m=w;ls=red,0");} --> tileNW[(i>>1)].electronicNRightIn if i%2==0;
			tileNW[(i>>1)].electronicNLeftIn <-- ElectronicChannel{@display("m=w;ls=red,0");} <-- tileNW[(i>>1)].electronicNRightOut if i%2==0;
			
			tileNE[(i>>1)].electronicNLeftOut --> ElectronicChannel{@display("m=w;ls=red,0");} --> tileNE[(i>>1)].electronicNRightIn if i%2!=0;
			tileNE[(i>>1)].electronicNLeftIn <-- ElectronicChannel{@display("m=w;ls=red,0");} <-- tileNE[(i>>1)].electronicNRightOut if i%2!=0;
			
         	
			
            tileNW[(i>>1)].photonicNLeftInjectionOut <--> uturnInjectionN[i].photonicOuterBendIn if i%2==0;
            tileNW[(i>>1)].photonicNLeftInjectionIn <--> uturnInjectionN[i].photonicInnerBendOut if i%2==0;
            tileNW[(i>>1)].photonicNRightInjectionOut <--> uturnInjectionN[i].photonicInnerBendIn if i%2==0;
            tileNW[(i>>1)].photonicNRightInjectionIn <--> uturnInjectionN[i].photonicOuterBendOut if i%2==0;

            tileNE[(i>>1)].photonicNLeftInjectionOut <--> uturnInjectionN[i].photonicOuterBendIn if i%2!=0;
            tileNE[(i>>1)].photonicNLeftInjectionIn <--> uturnInjectionN[i].photonicInnerBendOut if i%2!=0;
            tileNE[(i>>1)].photonicNRightInjectionOut <--> uturnInjectionN[i].photonicInnerBendIn if i%2!=0;
            tileNE[(i>>1)].photonicNRightInjectionIn <--> uturnInjectionN[i].photonicOuterBendOut if i%2!=0;
            
            
            tileNW[(i>>1)].electronicNLeftInjectionOut --> ElectronicChannel{@display("m=w;ls=red,0");} --> tileNW[(i>>1)].electronicNRightInjectionIn if i%2==0;
            tileNW[(i>>1)].electronicNLeftInjectionIn <-- ElectronicChannel{@display("m=w;ls=red,0");} <-- tileNW[(i>>1)].electronicNRightInjectionOut if i%2==0;
            
            tileNE[(i>>1)].electronicNLeftInjectionOut --> ElectronicChannel{@display("m=w;ls=red,0");} --> tileNE[(i>>1)].electronicNRightInjectionIn if i%2!=0;
            tileNE[(i>>1)].electronicNLeftInjectionIn <-- ElectronicChannel{@display("m=w;ls=red,0");} <-- tileNE[(i>>1)].electronicNRightInjectionOut if i%2!=0;
            
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

			tileSW[(i) + (j - 1)*(numOfNodesX>>1)].electronicSLeftOut --> ElectronicChannel{@display("m=w;ls=red,0");} --> tileSW[(i) + (j - 1)*(numOfNodesX>>1)].electronicSRightIn;
			tileSW[(i) + (j - 1)*(numOfNodesX>>1)].electronicSLeftIn <-- ElectronicChannel{@display("m=w;ls=red,0");} <-- tileSW[(i) + (j - 1)*(numOfNodesX>>1)].electronicSRightOut;

			tileSE[(i) + (j - 1)*(numOfNodesX>>1)].electronicSLeftOut --> ElectronicChannel{@display("m=w;ls=red,0");} --> tileSE[(i) + (j - 1)*(numOfNodesX>>1)].electronicSRightIn;
			tileSE[(i) + (j - 1)*(numOfNodesX>>1)].electronicSLeftIn <-- ElectronicChannel{@display("m=w;ls=red,0");} <-- tileSE[(i) + (j - 1)*(numOfNodesX>>1)].electronicSRightOut;



            tileSW[(i) + ((numOfNodesY>>1) - 1)*(numOfNodesX>>1)].photonicSLeftInjectionOut <--> uturnInjectionS[i*2].photonicOuterBendIn;
            tileSW[(i) + ((numOfNodesY>>1) - 1)*(numOfNodesX>>1)].photonicSLeftInjectionIn <--> uturnInjectionS[i*2].photonicInnerBendOut;
            tileSW[(i) + ((numOfNodesY>>1) - 1)*(numOfNodesX>>1)].photonicSRightInjectionOut <--> uturnInjectionS[i*2].photonicInnerBendIn;
            tileSW[(i) + ((numOfNodesY>>1) - 1)*(numOfNodesX>>1)].photonicSRightInjectionIn <--> uturnInjectionS[i*2].photonicOuterBendOut;

            tileSE[(i) + ((numOfNodesY>>1) - 1)*(numOfNodesX>>1)].photonicSLeftInjectionOut <--> uturnInjectionS[i*2+1].photonicOuterBendIn;
            tileSE[(i) + ((numOfNodesY>>1) - 1)*(numOfNodesX>>1)].photonicSLeftInjectionIn <--> uturnInjectionS[i*2+1].photonicInnerBendOut;
            tileSE[(i) + ((numOfNodesY>>1) - 1)*(numOfNodesX>>1)].photonicSRightInjectionOut <--> uturnInjectionS[i*2+1].photonicInnerBendIn;
            tileSE[(i) + ((numOfNodesY>>1) - 1)*(numOfNodesX>>1)].photonicSRightInjectionIn <--> uturnInjectionS[i*2+1].photonicOuterBendOut;

			tileSW[(i) + (j - 1)*(numOfNodesX>>1)].electronicSLeftInjectionOut --> ElectronicChannel{@display("m=w;ls=red,0");} --> tileSW[(i) + (j - 1)*(numOfNodesX>>1)].electronicSRightInjectionIn;
			tileSW[(i) + (j - 1)*(numOfNodesX>>1)].electronicSLeftInjectionIn <-- ElectronicChannel{@display("m=w;ls=red,0");} <-- tileSW[(i) + (j - 1)*(numOfNodesX>>1)].electronicSRightInjectionOut;

			tileSE[(i) + (j - 1)*(numOfNodesX>>1)].electronicSLeftInjectionOut --> ElectronicChannel{@display("m=w;ls=red,0");} --> tileSE[(i) + (j - 1)*(numOfNodesX>>1)].electronicSRightInjectionIn;
			tileSE[(i) + (j - 1)*(numOfNodesX>>1)].electronicSLeftInjectionIn <-- ElectronicChannel{@display("m=w;ls=red,0");} <-- tileSE[(i) + (j - 1)*(numOfNodesX>>1)].electronicSRightInjectionOut;


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

