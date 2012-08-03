// *************************************************************************
// *                                                                       *
// *  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
// *					   (c) Johnnie Chan  2008						   *
// *                                                                       *
// * file: bend.ned                                                        *
// * description:                                                          *
// *                                                                       *
// *                                                                       *
// *************************************************************************

package photonic.devices;

simple Bend
{
    parameters:

        double PropagationLoss;
		double BendingLoss;
		
        double Latency_Bend;
        double Angle_Bend = default(90);
        
        int dispValueX;
        int dispValueY;
		
        double SizeWidth = default(5);
        double SizeHeight = default(5);
		double PositionLeftX = default(0);
        double PositionBottomY = default(0);
		
        string groupLabel = default("Bend");
        int numOfPorts = 2;
        @display("b=10,10,,,,1");
    gates:
        inout photonicIn;

        inout photonicOut;
}

module Bend_PhotoCAD 
{
 	parameters:
 	    
 	    double xCoordinate;
 	    double yCoordinate;
 	    double Angle_Bend;
 	    
 	    double width;
 	    double height;
 	    
 	gates:
 		inout photonicPort[2];       
    
    submodules:
        B : Bend {
            Angle_Bend = Angle_Bend;

	     	
	     	SizeWidth = width;
	     	SizeHeight = height;
	     	
	     	PositionLeftX = xCoordinate; 
	     	PositionBottomY = yCoordinate; 
        }
        
    connections:
        
        B.photonicIn <--> photonicPort[0];
        B.photonicOut <--> photonicPort[1];
     
    
}
