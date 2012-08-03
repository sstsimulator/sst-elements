package processingPlane;



simple Cache
{
    parameters:
        int cacheCapacity;        //in bytes
        int cacheType;           //0 = direct mapped, 2 = 2-way, 4 = 4-way, 8 = 8-way, 100 = fully associative (buffer)

		int procMsgOverhead;
		double clockRate;
		 
        @display("i=block/table2");
    gates:
        inout procReq;
        inout nicReq;
}
