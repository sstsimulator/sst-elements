package electronicComponents;

channel ElectronicIOpad
{
    parameters:
        double clockRate_io;
        int ioChannelWidth;
        
        bool wireDoublePumping;
        
      	bool lowSwing = default(false);
        
        @display("i=old/box1");
   
   
   

}
