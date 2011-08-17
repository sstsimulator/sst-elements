// *************************************************************************
// *                                                                       *
// *  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
// *					   (c) Johnnie Chan  2008						   *
// *                                                                       *
// * file: cross.ned                                                       *
// * description:                                                          *
// *                                                                       *
// *                                                                       *
// *************************************************************************
package photonic.devices;

simple Cross
{
    parameters:
        double PropagationLoss;
        double CrossingLoss;
        double Latency_Cross;
        double Crosstalk_Cross;
        
        int dispValueX;
        int dispValueY;
        
        
        double SizeWidth = default(50);
        double SizeHeight = default(50);
        double PositionLeftX = default(0);
        double PositionBottomY = default(0);
        
        
        string groupLabel = default("Cross");
        int numOfPorts = 4;
        @display("b=50,50,,,,1");
    gates:
        inout photonicHorizIn;
        inout photonicVertIn;
        
        inout photonicHorizOut;
        inout photonicVertOut;
}


module Cross_PhotoCAD 
{
 	parameters:
 	    
 	    double xCoordinate;
 	    double yCoordinate;
 	    double width;
 	    double height;
 	    
 	gates:
 		inout photonicPort[4];       
    
    submodules:
        C : Cross {
	     	
	     	SizeWidth = width;
	     	SizeHeight = height;
	     	
	     	PositionLeftX = xCoordinate; 
	     	PositionBottomY = yCoordinate; 
        }
        
    connections:
        
        C.photonicVertIn <--> photonicPort[0];
        C.photonicHorizIn <--> photonicPort[1];
        C.photonicVertOut <--> photonicPort[2];
        C.photonicHorizOut <--> photonicPort[3];
    
}