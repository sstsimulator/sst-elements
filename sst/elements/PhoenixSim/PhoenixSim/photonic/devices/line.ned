// *************************************************************************
// *                                                                       *
// *  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
// *					   (c) Johnnie Chan  2008						   *
// *                                                                       *
// * file: line.ned                                                        *
// * description:                                                          *
// *                                                                       *
// *                                                                       *
// *************************************************************************
package photonic.devices;

simple Line
{
    parameters:

        double PropagationLoss;

        double Length_Line;
        double LatencyRate_Line;
        
        int dispValueX;
        int dispValueY;
        int dispValueZ;
       
		
		double SizeWidth = default(5);
        double SizeHeight = default(20);
		double PositionLeftX = default(0);
        double PositionBottomY = default(0);
		
        string groupLabel = default("Line");
		int numOfPorts = 2;
		@display("b=5,25,,#408080,,1");
    gates:
        inout photonicIn;

        inout photonicOut;
}

module Line_PhotoCAD {
    
	parameters:
	    
	    double Length_Line;
	    
	    double xCoordinate;
	    double yCoordinate;
	    double width;
	    double height;
	    
	gates:
	   inout photonicPort[2];     

	submodules:
	    
	    L : Line {
	     	Length_Line = Length_Line;  
	     	
	     	SizeWidth = width;
	     	SizeHeight = height;
	     	
	     	PositionLeftX = xCoordinate; 
	     	PositionBottomY = yCoordinate;    
	    }
	    
	connections:
	    L.photonicIn <--> photonicPort[0];
	    L.photonicOut <--> photonicPort[1];
    
}
