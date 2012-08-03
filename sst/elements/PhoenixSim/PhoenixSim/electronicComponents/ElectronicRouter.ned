package electronicComponents;


simple RouterCrossbar
{
    parameters:
		int numPorts;
		double clockRate;
		int electronicChannelWidth;
		
		bool autounblock = default(true);
		
		bool isDataPlane = default(false);
		bool isProcPlane = default(false);

        @display("i=abstract/switch");
    gates:
        input in[numPorts];

        output out[numPorts];

        inout cntrl;
}

//Router Arbiter types - this is what differentiates specific routers (besides the port count and pse cntrl)
//common-
// 0 - nonblocking4x4
// 1 - gateway control


// blocking torus - 
// 10 - ejection router
// 11 - injection router
// 12 - gateway router


// electronic mesh
// 20 - regular mesh node

// square root
// 30 - square root node


simple RouterArbiter
{
    parameters:
        string id;
        
        string level;
        string networkProfile;
        //int numOfRoutingLevels;
        
        int processorConcentration;
        string addressTranslator;
        
        int routerMaxGrants;
        
        int numOfNodesX;
        int numOfNodesY;
        
        
        int numX;
        int numY;
		int type;
		int numPorts;
		int numPSE;
		
		int switchVariant;
		
		
		int routerVirtualChannels;
		int electronicChannelWidth;
	
		double clockRate;
		int routerBufferSize;

        @display("i=block/cogwheel");
    gates:
        inout req[numPorts];

        inout XbarCntrl;
		output pseCntrl[numPSE];
		
        input ackIn[numPorts];
         
}

simple RouterInport
{
    parameters:
        int routerVirtualChannels;
        double clockRate;
		int electronicChannelWidth;
		int routerBufferSize;
		//double packetTimeout;
		int myId;

        @display("i=block/arrival");
    gates:
        input in;

        inout req;

		output ackOut;

        output toXbar;
}

module ElectronicRouter
{
    parameters:
        string id;
        
        string level;
        
         int numOfNodesX;
        int numOfNodesY;
        int processorConcentration;
        
        int numPorts;    //in plane
        int numPSEports;
        int type;
        double O_frequency_cntrl;
		int electronicChannelWidth;
		int routerBufferSize;
		
		int numX;
		int numY;

		int switchVariant = default(0);
		int dispValueX;
		int dispValueY;
        @display("i=abstract/router");
    gates:
        input portIn[numPorts];
        output portOut[numPorts];

        output toPSE[numPSEports];
        


    submodules:

        xbar: RouterCrossbar {
            parameters:
				numPorts = numPorts;
                @display("p=133,152");
                clockRate = O_frequency_cntrl;
          
        }

        arb: RouterArbiter {
			 parameters:
			     id = id;
			     type = type;
			     
			     level = level;
			     
			     clockRate = O_frequency_cntrl;
			     
			     numPorts = numPorts;
			     numPSE = numPSEports;
			     
			     numX = numX;
			     numY = numY;
			     
			     switchVariant = switchVariant;
			     
			 
			     @display("p=94,53");
        }

		stat : RouterStat {
		   parameters:
		       numPorts = numPorts; 
		       clockRate = O_frequency_cntrl;  
		}
		
		ackMux[numPorts] : mux {
		 	gates:
		 	    in[2];   
		}
		
		
        in[numPorts]: RouterInport {
			myId = index;
            @display("p=55,152");
            clockRate = O_frequency_cntrl;
        }

    connections allowunconnected:
        for i=0..numPorts-1 {
    
            in[i].toXbar --> xbar.in[i];
            xbar.out[i] --> ackMux[i].in[0];
            
            in[i].ackOut --> ackMux[i].in[1];
     
            ackMux[i].out --> portOut[i];

            portIn[i] --> in[i].in;

            arb.req[i] <--> in[i].req;
            
        }

        arb.XbarCntrl <--> xbar.cntrl;

		for i=0..numPSEports-1 {
		    toPSE[i] <-- arb.pseCntrl[i];
		}

}
