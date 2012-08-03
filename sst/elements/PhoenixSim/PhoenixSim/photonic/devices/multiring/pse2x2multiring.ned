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

simple PSE2x2MultiRing
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
        
        double ringDiameter = default(50);
        double ringDiameter0 = default(ringDiameter); 
        double ringDiameter1 = default(ringDiameter); 
        double ringDiameter2 = default(ringDiameter);
        double ringDiameter3 = default(ringDiameter); 
        double ringDiameter4 = default(ringDiameter); 
        double ringDiameter5 = default(ringDiameter);
        double ringDiameter6 = default(ringDiameter); 
        double ringDiameter7 = default(ringDiameter); 
        double ringDiameter8 = default(ringDiameter);
        double ringDiameter9 = default(ringDiameter); 

		double filterBaseWavelength0;
        int numOfResonantWavelengths0;
        double filterWavelengthSpacing0;        
		double filterBaseWavelength1;
        int numOfResonantWavelengths1;
        double filterWavelengthSpacing1;
		double filterBaseWavelength2;
        int numOfResonantWavelengths2;
        double filterWavelengthSpacing2;
		double filterBaseWavelength3;
        int numOfResonantWavelengths3;
        double filterWavelengthSpacing3;        
		double filterBaseWavelength4;
        int numOfResonantWavelengths4;
        double filterWavelengthSpacing4;
		double filterBaseWavelength5;
        int numOfResonantWavelengths5;
        double filterWavelengthSpacing5;
		double filterBaseWavelength6;
        int numOfResonantWavelengths6;
        double filterWavelengthSpacing6;        
		double filterBaseWavelength7;
        int numOfResonantWavelengths7;
        double filterWavelengthSpacing7;
		double filterBaseWavelength8;
        int numOfResonantWavelengths8;
        double filterWavelengthSpacing8;
		double filterBaseWavelength9;
        int numOfResonantWavelengths9;
        double filterWavelengthSpacing9;    
        

        
        
        
        double thermalRingTuningPower;
        double thermalTemperatureDeviation;
        int numOfRingSets = default(3);
        int numOfRings = 2*numOfRingSets;
        string groupLabel = default("PSE2x2MultiRing");
        int numOfPorts = 4;
        int numOfStates = pow(2,numOfRingSets);
        @display("i=components/pse2x2r3");
    gates:
        inout photonicHorizIn;
        inout photonicVertIn;
        input fromRouter;

        inout photonicHorizOut;
        inout photonicVertOut;
}

