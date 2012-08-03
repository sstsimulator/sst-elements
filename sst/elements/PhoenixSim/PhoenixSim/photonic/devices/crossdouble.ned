// *************************************************************************
// *                                                                       *
// *  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
// *                       (c) Johnnie Chan 2008                           *
// *                                                                       *
// * file: crossdouble.ned                                                 *
// * description:                                                          *
// *                                                                       *
// *                                                                       *
// *************************************************************************
package photonic.devices;

import photonic.devices.Cross;
import photonic.devices.Line;

module CrossDouble
{
    parameters:
        double PathSeparation;
        double LatencyRate_Line;
        double Latency_Cross;
        
        double PositionLeftX = default(0);
        double PositionBottomY = default(0);
    gates:
        inout photonicInA;
        inout photonicInB;
        inout photonicInSide;
        inout photonicOutA;
        inout photonicOutB;
        inout photonicOutSide;
    submodules:
        Cross_E: Cross {
            parameters:
                Latency_Cross = Latency_Cross;
                @display("p=88,88;b=54,56");
        }
        Cross_W: Cross {
            parameters:
                Latency_Cross = Latency_Cross;
                @display("p=280,88;b=56,56");
        }
        Line_Center: Line {
            parameters:
                Length_Line = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=88,184;b=8,64");
        }
    connections allowunconnected:
        photonicInB <--> { @display("m=n"); } <--> Cross_E.photonicVertIn;
        photonicInA <--> { @display("m=s"); } <--> Cross_W.photonicVertIn;
        photonicInSide <--> { @display("m=e"); } <--> Cross_E.photonicHorizIn;

        Cross_E.photonicHorizOut <--> Line_Center.photonicIn;

        Line_Center.photonicOut <--> Cross_W.photonicHorizIn;

        Cross_W.photonicHorizOut <--> { @display("m=w"); } <--> photonicOutSide;
        Cross_W.photonicVertOut <--> { @display("m=n"); } <--> photonicOutA;
        Cross_E.photonicVertOut <--> { @display("m=s"); } <--> photonicOutB;
}

