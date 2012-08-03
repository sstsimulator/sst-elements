// *************************************************************************
// *                                                                       *
// *  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
// *                       (c) Assaf Shacham 2006-7                        *
// *					   (c) Johnnie Chan  2008						   *
// *                                                                       *
// * file: PSE2x2.ned                                                      *
// * description:                                                          *
// *                                                                       *
// *                                                                       *
// *************************************************************************
package photonic.devices;

simple PSE2x2
{
    parameters:
        double PropagationLoss;
        double PassByRingLoss;
        double PassThroughRingLoss;
        double CrossingLoss;
        double Crosstalk_Cross;

        double RingOn_ER_2x2;
        double RingOff_ER_2x2;
        double CrossDelay_2x2;

		double RingStaticDissipation;
		double RingDynamicOffOn;
		double RingDynamicOnOff;


        double BarDelay_2x2;
        bool useRingModel = default(false);
        double ringDiameter = default(50); // Assumes a 50 um circular ring
        
        double thermalRingTuningPower;
        double thermalTemperatureDeviation;
        int numOfRings = default(2);
        
        string groupLabel = default("PSE2x2");
        int numOfPorts = 4;
        int numOfStates = 2;
        
        double SizeWidth = default(130);
        double SizeHeight = default(130);
        double PositionLeftX = default(0);
        double PositionBottomY = default(0);
        @display("i=components/pse2x2");
    gates:
        inout photonicHorizIn;
        inout photonicVertIn;
        input fromRouter;

        inout photonicHorizOut;
        inout photonicVertOut;
}

