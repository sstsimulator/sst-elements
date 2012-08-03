// *************************************************************************
// *                                                                       *
// *  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
// *					   (c) Johnnie Chan  2008						   *
// *                                                                       *
// * file: pse1x2.ned                                                      *
// * description:                                                          *
// *                                                                       *
// *                                                                       *
// *************************************************************************
package photonic.devices;

simple PSE1x2
{
    parameters:
        double PropagationLoss;
        double PassByRingLoss;
        double PassThroughRingLoss;
        double CrossingLoss;
        double Crosstalk_Cross;
        
        double RingOn_ER_1x2;
        double RingOff_ER_1x2;
        double ThroughDelay_1x2;

		double RingStaticDissipation;
		double RingDynamicOffOn;
		double RingDynamicOnOff;

        double DropDelay_1x2;
        
        int dispValueX;
        int dispValueY;
        int dispValueZ;
		double ringDiameter = default(50); // Assumes a 50 um circular ring
        bool useRingModel = default(false);
        int numOfRings = default(1);
        
        
        
        double thermalRingTuningPower;
        double thermalTemperatureDeviation;
        
        string groupLabel = default("PSE1x2");
        int numOfPorts = 4;
        int numOfStates = 2;
        
        double SizeWidth = default(80);
        double SizeHeight = default(80);
        double PositionLeftX = default(0);
        double PositionBottomY = default(0);
        @display("i=components/pse1x2");
    gates:
        inout photonicHorizIn;
        inout photonicVertIn;
        input fromRouter;

        inout photonicHorizOut;
        inout photonicVertOut;
}


module PSE1x2_PhotoCAD
{
    parameters:
        double xCoordinate;
        double yCoordinate;
        double ringDiameter;
        string orient;
        
        double width;
        double height;
        
    gates:
        inout photonicPort[4];
        input cntrl_ringCntrl[1];
    
    submodules:
        
        P : PSE1x2 {
            ringDiameter = ringDiameter;  
	     	
	     	SizeWidth = width;
	     	SizeHeight = height;
	     	
	     	PositionLeftX = xCoordinate; 
	     	PositionBottomY = yCoordinate; 
        }
        
    connections:
        
        // orient = NE
//        P.photonicVertOut <--> photonicPort[0] if orient == "NE";
//        P.photonicHorizIn <--> photonicPort[1] if orient == "NE";
//        P.photonicVertIn <--> photonicPort[2] if orient == "NE";
//        P.photonicHorizOut <--> photonicPort[3] if orient == "NE";
//        
//         // orient = NW
//        P.photonicVertOut <--> photonicPort[0] if orient == "NW";
//        P.photonicHorizIn <--> photonicPort[2] if orient == "NW";
//        P.photonicVertIn <--> photonicPort[3] if orient == "NW";
//        P.photonicHorizOut <--> photonicPort[1] if orient == "NW";
//        
//         // orient = SE
//        P.photonicVertOut <--> photonicPort[2] if orient == "SE";
//        P.photonicHorizIn <--> photonicPort[1] if orient == "SE";
//        P.photonicVertIn <--> photonicPort[0] if orient == "SE";
//        P.photonicHorizOut <--> photonicPort[3] if orient == "SE";
//        
//         // orient = SW
//        P.photonicVertOut <--> photonicPort[2] if orient == "SW";
//        P.photonicHorizIn <--> photonicPort[3] if orient == "SW";
//        P.photonicVertIn <--> photonicPort[0] if orient == "SW";
//        P.photonicHorizOut <--> photonicPort[1] if orient == "SW";

 		P.photonicVertOut <--> photonicPort[0];
        P.photonicHorizIn <--> photonicPort[2];
        P.photonicVertIn <--> photonicPort[3];
        P.photonicHorizOut <--> photonicPort[1];
        
        P.fromRouter <-- cntrl_ringCntrl[0];
}
