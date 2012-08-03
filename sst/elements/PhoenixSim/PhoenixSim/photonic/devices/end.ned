// *************************************************************************
// *                                                                       *
// *  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
// *					   (c) Johnnie Chan  2008						   *
// *                                                                       *
// * file: end.ned                                                         *
// * description:                                                          *
// *                                                                       *
// *                                                                       *
// *************************************************************************
package photonic.devices;

simple End
{
    parameters:
        string groupLabel = default("Termination");
        int numOfPorts = 1;
        @display("b=10,10");
    gates:
        inout photonicIn;
}

