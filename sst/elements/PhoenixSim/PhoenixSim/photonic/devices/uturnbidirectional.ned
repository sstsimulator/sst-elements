package photonic.devices;

import photonic.devices.UTurn;
import photonic.devices.Line;

module UTurnBidirectional
{
    parameters:
        double PathSeparation;
        double TurnDistance;
        double LatencyRate_Line;
        double Latency_Bend;

        double disp_Width;
        double disp_Width2;
        
        double PositionLeftX = default(0);
        double PositionBottomY = default(0);
        @display("bgb=346,256");
    gates:
        inout photonicInnerBendIn;
        inout photonicOuterBendIn;
        inout photonicInnerBendOut;
        inout photonicOuterBendOut;

    submodules:
        Line_In: Line {
            parameters:
                Length_Line = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=76,128;b=8,40");
        }
        Line_Out: Line {
            parameters:
                Length_Line = PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                @display("p=257,128;b=8,40");
        }
        UTurnOuter: UTurn {
            parameters:
                TurnDistance = (TurnDistance+PathSeparation)+PathSeparation;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Bend = Latency_Bend;
                @display("p=164,80;b=200,8");
        }
        UTurnInner: UTurn {
            parameters:
                TurnDistance = TurnDistance;
                LatencyRate_Line = LatencyRate_Line;
                Latency_Bend = Latency_Bend;
                @display("p=166,135;b=112,10");
        }
    connections allowunconnected:
        photonicInnerBendIn <--> { @display("m=m,40,40,92,56"); } <--> UTurnInner.photonicIn;
        UTurnInner.photonicOut <--> { @display("m=m,64,0"); } <--> photonicInnerBendOut;
        photonicOuterBendIn <--> { @display("m=s"); } <--> Line_In.photonicIn;
        Line_In.photonicOut <--> UTurnOuter.photonicIn;
        UTurnOuter.photonicOut <--> Line_Out.photonicIn;
        Line_Out.photonicOut <--> { @display("m=s"); } <--> photonicOuterBendOut;
}

