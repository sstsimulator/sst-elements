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
package photonic.devices;

simple SwitchingFilter
{
    parameters:
        double PropagationLoss;
        double PassByRingLoss;
        double PassThroughRingLoss;
        double CrossingLoss;
        double Crosstalk_Cross;
        
        double RingOn_ER_1x2;
        double RingOff_ER_1x2;
        double ThroughDelay_1x2;

		double RingStaticDissipation;
		double RingDynamicOffOn;
		double RingDynamicOnOff;

        double PSE1x2_pwr_switch_static;
        double PSE1x2_pwr_switch_off_on;
        double PSE1x2_pwr_switch_on_off;

        double DropDelay_1x2;
        
        int dispValueX;
        int dispValueY;
        int dispValueZ;
        
        double filterBaseWavelength;
        
        
        string groupLabel = default("PSE1x2");
        int numOfPorts = 4;
        int numOfStates = 2;
        
    gates:
        inout thruIn;
        inout In;
        inout Out;
        inout Drop;
        
        input cntrl;
      
}


