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

simple PSE2x2R3
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
        
        double ringDiameter0 = default(17); // Assumes a 50 um circular ring
        double ringDiameter1 = default(17); // Assumes a 50 um circular ring
        double ringDiameter2 = default(17); // Assumes a 50 um circular ring

		double filterBaseWavelength0;
        int numOfResonantWavelengths0;
        double filterWavelengthSpacing0;        
		double filterBaseWavelength1;
        int numOfResonantWavelengths1;
        double filterWavelengthSpacing1;
		double filterBaseWavelength2;
        int numOfResonantWavelengths2;
        double filterWavelengthSpacing2;

        
        
        
        double thermalRingTuningPower;
        double thermalTemperatureDeviation;
        int numOfRingSets = 3;
        int numOfRings = default(6);
        
        string groupLabel = default("PSE2x2R3");
        int numOfPorts = 4;
        int numOfStates = 8;
        @display("i=components/pse2x2r3");
    gates:
        inout photonicHorizIn;
        inout photonicVertIn;
        input fromRouter;

        inout photonicHorizOut;
        inout photonicVertOut;
}

