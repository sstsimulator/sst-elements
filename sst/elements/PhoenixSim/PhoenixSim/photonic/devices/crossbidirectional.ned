// *************************************************************************
// *                                                                       *
// *  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
// *                       (c) Johnnie Chan 2008                           *
// *                                                                       *
// * file: crossbidirectional.ned                                          *
// * description:                                                          *
// *                                                                       *
// *                                                                       *
// *************************************************************************
package photonic.devices;

import photonic.devices.Cross;
import photonic.devices.Line;

module CrossBidirectional
{
    parameters:
        double PathSeparationX;
        double PathSeparationY;
        double LatencyRate_Line;
        double Latency_Cross;
        
        double PositionLeftX = default(0);
        double PositionBottomY = default(0);
        @display("b=24,24,,,,1");
    gates:
        inout photonicInN;
        inout photonicInS;
        inout photonicInE;
        inout photonicInW;
        inout photonicOutN;
        inout photonicOutS;
        inout photonicOutE;
        inout photonicOutW;
    submodules:
        Cross_NW: Cross {
            parameters:
                Latency_Cross = Latency_Cross;
                @display("p=88,88;b=54,56");
        }
        Cross_NE: Cross {
            parameters:
                Latency_Cross = Latency_Cross;
                @display("p=280,88;b=56,56");
        }
        Cross_SW: Cross {
            parameters:
                Latency_Cross = Latency_Cross;
                @display("p=88,280;b=56,56");
        }
        Cross_SE: Cross {
            parameters:
                Latency_Cross = Latency_Cross;
                @display("p=280,280;b=56,56");
        }
        Line_N: Line {
            parameters:
                Length_Line = PathSeparationX;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=184,88;b=64,8");
        }
        Line_S: Line {
            parameters:
                Length_Line = PathSeparationX;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=184,280;b=64,8");
        }
        Line_E: Line {
            parameters:
                Length_Line = PathSeparationY;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=280,184;b=8,64");
        }
        Line_W: Line {
            parameters:
                Length_Line = PathSeparationY;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=88,184;b=8,64");
        }
    connections allowunconnected:
        photonicInN <--> { @display("m=n"); } <--> Cross_NE.photonicVertIn;
        photonicInS <--> { @display("m=s"); } <--> Cross_SW.photonicVertIn;
        photonicInE <--> { @display("m=e"); } <--> Cross_NE.photonicHorizIn;
        photonicInW <--> { @display("m=w"); } <--> Cross_SW.photonicHorizIn;

        Cross_NE.photonicVertOut <--> Line_E.photonicIn;
        Cross_NE.photonicHorizOut <--> Line_N.photonicIn;
        Cross_SW.photonicVertOut <--> Line_W.photonicIn;
        Cross_SW.photonicHorizOut <--> Line_S.photonicIn;

        Line_N.photonicOut <--> Cross_NW.photonicHorizIn;
        Line_S.photonicOut <--> Cross_SE.photonicHorizIn;
        Line_E.photonicOut <--> Cross_SE.photonicVertIn;
        Line_W.photonicOut <--> Cross_NW.photonicVertIn;

        Cross_NW.photonicHorizOut <--> { @display("m=w"); } <--> photonicOutW;
        Cross_NW.photonicVertOut <--> { @display("m=n"); } <--> photonicOutN;
        Cross_SE.photonicHorizOut <--> { @display("m=e"); } <--> photonicOutE;
        Cross_SE.photonicVertOut <--> { @display("m=s"); } <--> photonicOutS;
}


module CrossBidirectional_SwapVertical
{
    parameters:
        double PathSeparationX;
        double PathSeparationY;
        double LatencyRate_Line;
        double ILRate_Line;
        double IL_Cross;
        double Latency_Cross;
    gates:
        inout photonicInN;
        inout photonicInS;
        inout photonicInE;
        inout photonicInW;
        inout photonicOutN;
        inout photonicOutS;
        inout photonicOutE;
        inout photonicOutW;
    submodules:
        Cross_NW: Cross {
            parameters:
                Latency_Cross = Latency_Cross;
                @display("p=88,88;b=54,56");
        }
        Cross_NE: Cross {
            parameters:
                Latency_Cross = Latency_Cross;
                @display("p=280,88;b=56,56");
        }
        Cross_SW: Cross {
            parameters:
                Latency_Cross = Latency_Cross;
                @display("p=88,280;b=56,56");
        }
        Cross_SE: Cross {
            parameters:
                Latency_Cross = Latency_Cross;
                @display("p=280,280;b=56,56");
        }
        Line_N: Line {
            parameters:
                Length_Line = PathSeparationX;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=184,88;b=64,8");
        }
        Line_S: Line {
            parameters:
                Length_Line = PathSeparationX;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=184,280;b=64,8");
        }
        Line_E: Line {
            parameters:
                Length_Line = PathSeparationY;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=280,184;b=8,64");
        }
        Line_W: Line {
            parameters:
                Length_Line = PathSeparationY;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=88,184;b=8,64");
        }
    connections allowunconnected:
        photonicInN <--> { @display("m=n"); } <--> Cross_NW.photonicVertIn;
        photonicInS <--> { @display("m=s"); } <--> Cross_SE.photonicVertIn;
        photonicInE <--> { @display("m=e"); } <--> Cross_NE.photonicHorizIn;
        photonicInW <--> { @display("m=w"); } <--> Cross_SW.photonicHorizIn;

        Cross_SE.photonicVertOut <--> Line_E.photonicIn;
        Cross_NE.photonicHorizOut <--> Line_N.photonicIn;
        Cross_NW.photonicVertOut <--> Line_W.photonicIn;
        Cross_SW.photonicHorizOut <--> Line_S.photonicIn;

        Line_N.photonicOut <--> Cross_NW.photonicHorizIn;
        Line_S.photonicOut <--> Cross_SE.photonicHorizIn;
        Line_E.photonicOut <--> Cross_NE.photonicVertIn;
        Line_W.photonicOut <--> Cross_SW.photonicVertIn;

        Cross_NW.photonicHorizOut <--> { @display("m=w"); } <--> photonicOutW;
        Cross_NE.photonicVertOut <--> { @display("m=n"); } <--> photonicOutN;
        Cross_SE.photonicHorizOut <--> { @display("m=e"); } <--> photonicOutE;
        Cross_SW.photonicVertOut <--> { @display("m=s"); } <--> photonicOutS;
}


