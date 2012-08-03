package photonic.devices;

import photonic.devices.Line;

module LineBidirectional
{
    parameters:
        double Length_Line;
        double LatencyRate_Line;
        
        
        int PositionRotate90 = default(0);
        double PositionLeftX = default(0);
        double PositionBottomY = default(0);
        
        
        double dispValueX;
        @display("bgb=157,224");
    gates:
        inout photonicInA;
        inout photonicInB;
        inout photonicOutA;
        inout photonicOutB;
    submodules:
        LineA: Line {
            parameters:
                Length_Line = Length_Line;
                LatencyRate_Line = LatencyRate_Line;
                
				SizeWidth = 5;
				SizeHeight = Length_Line;
				PositionLeftX = 0;
				PositionBottomY = 0;
                @display("p=68,116;b=8,120");
        }
        LineB: Line {
            parameters:
                Length_Line = Length_Line;
                LatencyRate_Line = LatencyRate_Line;
                
				SizeWidth = 5;
				SizeHeight = Length_Line;
				PositionLeftX = 5;
				PositionBottomY = 0;
                @display("p=108,116;b=8,120");
        }
    connections allowunconnected:
        LineA.photonicOut <--> { @display("m=n"); } <--> photonicOutA;
        photonicInB <--> { @display("m=n"); } <--> LineB.photonicIn;
        LineB.photonicOut <--> { @display("m=s"); } <--> photonicOutB;
        photonicInA <--> { @display("m=s"); } <--> LineA.photonicIn;
}

