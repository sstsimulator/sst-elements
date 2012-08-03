// *************************************************************************
// *                                                                       *
// *  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
// *					   (c) Johnnie Chan  2008						   *
// *                                                                       *
// * file: pse1x2.ned                                                      *
// * description:                                                          *
// *                                                                       *
// *                                                                       *
// *************************************************************************
package photonic.devices.multiring;

simple PSE1x2R3
{
    parameters:
        double PropagationLoss;
        double PassByRingLoss;
        double PassThroughRingLoss;
        double CrossingLoss;
        double Crosstalk_Cross;
		double BendingLoss;
        
        double RingOn_ER_1x2;
        double RingOff_ER_1x2;
        double ThroughDelay_1x2;

		double RingStaticDissipation;
		double RingDynamicOffOn;
		double RingDynamicOnOff;

        double DropDelay_1x2;
        
        int dispValueX;
        int dispValueY;
        int dispValueZ;
        double ringDiameter0 = default(17); // Assumes a 50 um circular ring
        double ringDiameter1 = default(17); // Assumes a 50 um circular ring
        double ringDiameter2 = default(17); // Assumes a 50 um circular ring
        
        bool useRingModel = default(false);
        int numOfRingSets = 3;
        int numOfRings = default(3);
        
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
        
        string groupLabel = default("PSE1x2R3");
        int numOfPorts = 4;
        int numOfStates = 8;
        @display("i=components/pse1x2r3");
    gates:
        inout photonicHorizIn;
        inout photonicVertIn;
        input fromRouter;

        inout photonicHorizOut;
        inout photonicVertOut;
}

