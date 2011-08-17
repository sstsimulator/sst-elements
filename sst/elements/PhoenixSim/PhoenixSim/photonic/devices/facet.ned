package photonic.devices;

simple Facet
{
    parameters:
        double IL_Facet;
        
        string groupLabel = default("Facet");
        int numOfPorts = 2;
        
        double SizeWidth = default(1);
        double SizeHeight = default(5);
        double PositionLeftX = default(0);
        double PositionBottomY = default(0);
        @display("i=old/bwxconn");
    gates:
        inout photonicIn;

        inout photonicOut;

}



