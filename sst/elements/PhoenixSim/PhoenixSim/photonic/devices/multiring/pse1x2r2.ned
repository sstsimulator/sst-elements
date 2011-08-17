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

simple PSE1x2R2
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
        double ringDiameter = default(25); // Assumes a 50 um circular ring
        bool useRingModel = default(false);
        int numOfRings = default(2);
        
        
		double filterBaseWavelength;
        int numOfResonantWavelengths;
        double filterWavelengthSpacing;
        
        double filterBaseWavelengthAlt;
        int numOfResonantWavelengthsAlt;
        double filterWavelengthSpacingAlt;
        
        
        double thermalRingTuningPower;
        double thermalTemperatureDeviation;
        
        string groupLabel = default("PSE1x2R2");
        int numOfPorts = 4;
        int numOfStates = 4;
        @display("i=components/pse1x2r2");
    gates:
        inout photonicHorizIn;
        inout photonicVertIn;
        input fromRouter;

        inout photonicHorizOut;
        inout photonicVertOut;
}

