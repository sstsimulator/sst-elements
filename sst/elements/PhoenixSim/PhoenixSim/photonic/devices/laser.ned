package photonic.devices;

simple Laser
{
    parameters:
        double LaserPower;
        double Wavelength;
        double LaserEfficiency;
        
        string groupLabel = default("Laser");
        int numOfPorts = 1;
        @display("b=10,10");
	gates:
	    inout	photonicOut;
	    input   electronicIn;
	    output  electronicOut;
}
        