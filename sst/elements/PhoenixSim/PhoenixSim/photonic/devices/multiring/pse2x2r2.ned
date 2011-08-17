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
package photonic.devices.multiring;

simple PSE2x2R2
{
    parameters:
        double PropagationLoss;
        double PassByRingLoss;
        double PassThroughRingLoss;
        double CrossingLoss;
        double Crosstalk_Cross;
		double BendingLoss;

        double RingOn_ER_2x2;
        double RingOff_ER_2x2;
        double CrossDelay_2x2;

		double RingStaticDissipation;
		double RingDynamicOffOn;
		double RingDynamicOnOff;


        double BarDelay_2x2;
        bool useRingModel = default(false);
        double ringDiameter = default(25); // Assumes a 50 um circular ring
        
		double filterBaseWavelength;
        int numOfResonantWavelengths;
        double filterWavelengthSpacing;
        
        double filterBaseWavelengthAlt;
        int numOfResonantWavelengthsAlt;
        double filterWavelengthSpacingAlt;
        
        
        double thermalRingTuningPower;
        double thermalTemperatureDeviation;
        int numOfRings = default(4);
        
        string groupLabel = default("PSE2x2R2");
        int numOfPorts = 4;
        int numOfStates = 4;
        @display("i=components/pse2x2r2");
    gates:
        inout photonicHorizIn;
        inout photonicVertIn;
        input fromRouter;

        inout photonicHorizOut;
        inout photonicVertOut;
}

