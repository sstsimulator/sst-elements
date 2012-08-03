
package electronicComponents;


channel ElectronicChannel
{
    parameters:
    	double O_frequency_data;
    	double O_frequency_cntrl;
    	int electronicChannelWidth;
    	
    	bool isDataChannel = default(false);
        
        int spaceLengths = default(1);
        int routerLengths = default(0);

		double coreSizeX;
		
		bool wireDoublePumping;
		
		bool power = default(true);
		bool lowswing = default(false);
		
		bool isThroughChannel = default(false);
		
		
}

