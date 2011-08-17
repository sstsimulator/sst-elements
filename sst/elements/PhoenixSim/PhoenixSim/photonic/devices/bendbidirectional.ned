// *************************************************************************
// *                                                                       *
// *  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
// *                       (c) Johnnie Chan 2008                           *
// *                                                                       *
// * file: bendbidirectional.ned                                           *
// * description:                                                          *
// *                                                                       *
// *                                                                       *
// *************************************************************************
package photonic.devices;

import photonic.devices.Bend;
import photonic.devices.Line;

module BendBidirectional
{
    parameters:
        double PathSeparation;
        double LatencyRate_Line;
        double Latency_Bend;
        
        double PositionLeftX = default(0);
        double PositionBottomY = default(0);
        @display("bgb=186,192");
    gates:
        inout photonicOuterBendIn;
        inout photonicInnerBendIn;
        inout photonicOuterBendOut;
        inout photonicInnerBendOut;
    submodules:
        Bend_Inner: Bend {
            parameters:
                Angle_Bend = 90;
                @display("p=132,132;b=26,26");
        }
        Line_OuterBendExit: Line {
            parameters:
                Length_Line = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=112,53;b=64,8");
        }
        Bend_Outer: Bend {
            parameters:
                Angle_Bend = 90;
                @display("p=53,53;b=26,26");
        }
        Line_OuterBendEnter: Line {
            parameters:
                Length_Line = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=52,112;b=8,64");
        }
    connections allowunconnected:
        photonicOuterBendIn <--> { @display("m=s"); } <--> Line_OuterBendEnter.photonicIn;
        Line_OuterBendEnter.photonicOut <--> Bend_Outer.photonicIn;
        Bend_Outer.photonicOut <--> Line_OuterBendExit.photonicIn;
        Line_OuterBendExit.photonicOut <--> { @display("m=e"); } <--> photonicOuterBendOut;
        photonicInnerBendIn <--> { @display("m=e"); } <--> Bend_Inner.photonicIn;
        Bend_Inner.photonicOut <--> { @display("m=s"); } <--> photonicInnerBendOut;
}

