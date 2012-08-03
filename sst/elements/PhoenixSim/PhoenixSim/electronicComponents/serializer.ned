package electronicComponents;


simple Serializer
{
    parameters:
        int dispValueX;
        int dispValueY;
        
        int numWiresIn;
        double rateIn;
        
        int numWiresOut;
        double rateOut;
        
        
    gates:
        input in[];
        output out;
}

simple Parallelizer
{
    parameters:
        int dispValueX;
        int dispValueY;
        
        int numWiresIn;
        double rateIn;
        
        int numWiresOut;
        double rateOut;
    gates:
        input in;
        output out[];
}
