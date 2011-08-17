package photonic.devices;

simple Join
{
    parameters:
    	string groupLabel = default("Join");
        int numOfPorts = 3;
        @display("i=components/pse2x2");
    gates:
        inout photonicIn1;
        inout photonicIn2;

        inout photonicOut;
}
