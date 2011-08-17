// Standard crossbar with farthest corner converted to turn

package photonic.switches;

import photonic.devices.Line;
import photonic.devices.PSE1x2;
import photonic.devices.Bend;
import photonic.devices.Cross;
//
// TODO auto-generated module
//
module CrossbarSwitch
{
    parameters:
        int numPorts;
        int numPSE;
        int variant = 0;
        
        double waveguideSpacing = default(0); //spacing above the minimum required for the pses (50um)
        
        
	gates:
	    inout in[numPorts];
	    inout out[numPorts];
	    
	    input cntrl[numPSE];
	    
	submodules:
	    switch[numPorts*numPorts-1]: PSE1x2
	    {
	        @display("p=80,80,m,$numPorts,80,80;b=20,20,,,#000000,1");
	    }
	    
	    line_horiz[numPorts*(numPorts-1)]: Line
	    {
	        parameters:
	            Length_Line = waveguideSpacing;
	            dispValueX = numPorts - 1;
	        @display("p=120,80,m,$dispValueX,80,80;b=20,5,,#408080,#000000,1");
	    }
	    line_vert[(numPorts-1)*numPorts]: Line
	    {
	        parameters:
	            Length_Line = waveguideSpacing;
	        @display("p=80,120,m,$numPorts,80,80;b=5,20,,#408080,#000000,1");
	    }
	    bend: Bend
	    {
	        parameters:
	            Angle_Bend = 90;
	            dispValueX = (numPorts-1)*80;
	            dispValueY = (numPorts-1)*80;
	        @display("p=80,80,x,$dispValueX,$dispValueY,80;b=5,5,,#408080,#000000,1");
	    }
	    

	connections allowunconnected:
	    
	    // Connect switches together
	    for i = 0 .. numPorts -1 ,
	    for j = 0 .. numPorts - 1
	    {
	        switch[j + i * numPorts].photonicHorizIn <--> line_horiz[j - 1 + i * (numPorts-1)].photonicOut if  (j!= 0) && !(j == numPorts - 1 && i == numPorts - 1);
	        switch[j + i * numPorts].photonicHorizOut <--> line_horiz[j + i * (numPorts-1)].photonicIn if j != numPorts - 1;
	        
	        switch[j + i * numPorts].photonicVertIn <--> line_vert[j + i * numPorts].photonicOut if i != numPorts - 1;
	        switch[j + i * numPorts].photonicVertOut <--> line_vert[j + (i - 1) * numPorts].photonicIn if i != 0 && !(j==numPorts - 1 && i == numPorts - 1);
	    }
	    
	    // Connect final bend optimization
	    bend.photonicIn <--> line_horiz[numPorts * (numPorts-1) - 1].photonicOut if numPorts > 1;
	    bend.photonicOut <--> line_vert[(numPorts-1) * numPorts - 1].photonicIn if numPorts > 1;
	    
	    // Connect inputs and outputs to module interface
	    for i = 0 .. numPorts - 1
	    {
	        switch[i * numPorts].photonicHorizIn <--> {@display("m=w");} <--> in[i] if !(numPorts==1 && i == numPorts -1);
	    }
	    for i = 0 .. numPorts - 1
	    {
	        switch[i].photonicVertOut <--> {@display("m=n");} <--> out[i] if !(numPorts==1 && i == numPorts -1);
	    }
	    
	    bend.photonicIn <--> {@display("m=w");} <--> in[numPorts - 1] if numPorts == 1;
	    bend.photonicOut <--> {@display("m=n");} <--> out[numPorts - 1] if numPorts == 1;
	    
	    // Connect PSEs to module interface
	    for i = 0 .. numPorts - 1,
	    for j = 0 .. numPorts - 1
	    {
	        switch[j + i * numPorts].fromRouter <-- {@display("ls=red,0");} <-- cntrl[j + i * numPorts] if j + i * numPorts != numPorts * numPorts -1;
	    }
	    
}


// Optimized Crossbar to save more rings

module SeashellSwitch
{
    parameters:
        int numPorts;
        int numPSE;
        int variant = 0;
        double waveguideSpacing;
        
	gates:
	    inout in[];
	    inout out[];
	    
	    input control[];
	    
	submodules:
	    switch[numPorts*numPorts-1]: PSE1x2
	    {
	        	parameters:
	        	    dispValueX = (numPorts - 1 -(index%numPorts))==numPorts - 1 - floor(index/numPorts)?0:30;
	        @display("p=80,80,m,$numPorts,80,80;b=$dispValueX,$dispValueX,,,#000000,1");
	    }
	   line_horiz[numPorts*(numPorts-1)]: Line
	    {
	        parameters:
	            Length_Line = waveguideSpacing;
	            dispValueX = numPorts - 1;
	        @display("p=120,80,m,$dispValueX,80,80;b=20,5,,#408080,#000000,1");
	    }
	    line_vert[(numPorts-1)*numPorts]: Line
	    {
	        parameters:
	            Length_Line = waveguideSpacing;
	        @display("p=80,120,m,$numPorts,80,80;b=5,20,,#408080,#000000,1");
	    }
	    
	    line_horiz_extend[min(numPorts,numPorts)-1]: Line
	    {
	        parameters:
	            Length_Line = (index+1)*50 + waveguideSpacing;
	        	dispValueX = (numPorts-1)*80 - 20 - index*20;
	            dispValueY = (numPorts-1)*80 + index*40 + 40;
	            dispValueZ = 60 + index * 120;
	        @display("p=80,80,x,$dispValueX,$dispValueY,80;b=$dispValueZ,5,,#408080,#000000,1");
	    }
	    line_vert_extend[min(numPorts,numPorts)-1]: Line
	    {
	        parameters:
	            Length_Line = (index+1)*50 + waveguideSpacing;
	            dispValueX = (numPorts-1)*80 + index*40 + 40;
	            dispValueY = (numPorts-1)*80 + 20 - index*20;
	            dispValueZ = 60 + index * 120;
	        @display("p=80,40,x,$dispValueX,$dispValueY;b=5,$dispValueZ,,#408080,#000000,1");
	    }
	    
	    line_horiz_stub[max(0,min(numPorts,numPorts)-2)]: Line
	    {
	        parameters:
	            Length_Line = index*5;
	            dispValueX = (numPorts)*80 + 55 -(min(numPorts,numPorts)-2-index)*20;// - (max(0,min(numPorts,numPorts)-2)-1-index)*20;
	            dispValueY = (numPorts-3)*80 - index * 80;
	            dispValueZ = 30 + index * 40;
	        @display("p=40,80,x,$dispValueX,$dispValueY;b=$dispValueZ,5,,#408080,#000000,1");
	    }
	    line_vert_stub[max(0,min(numPorts,numPorts)-2)]: Line
	    {
	        parameters:
	            Length_Line = index*5;
	            dispValueX = (numPorts-3)*80 - index * 80;
	            dispValueY = (numPorts)*80 - (max(0,min(numPorts,numPorts)-2)-1-index)*20 + 45;
	            dispValueZ = 30 + index * 40;
	        @display("p=80,40,x,$dispValueX,$dispValueY;b=5,$dispValueZ,,#408080,#000000,1");
	    }
	    bend[min(numPorts,numPorts)]: Bend
	    {
	        parameters:
	            Angle_Bend = 90;
	            dispValueX = (numPorts-1)*80 + index*40;
	            dispValueY = (numPorts-1)*80 + index*40;
	        @display("p=80,80,x,$dispValueX,$dispValueY,80;b=5,5,,#408080,#000000,1");
	    }
	    
	    bend_bottom[min(numPorts,numPorts)-1]: Bend
	    {
	        parameters:
	            Angle_Bend = 90;
	           	dispValueX = (numPorts-1)*80 - 80 - index * 80;
	            dispValueY = (numPorts-1)*80 + index*40 + 40;
	        @display("p=80,80,x,$dispValueX,$dispValueY,80;b=5,5,,#408080,#000000,1");
	    }
	    
	    bend_right[min(numPorts,numPorts)-1]: Bend
	    {
	        parameters:
	            Angle_Bend = 90;
	            dispValueX = (numPorts-1)*80 + index*40 + 40;
	            dispValueY = (numPorts-1)*80 - 80 - index * 80;
	        @display("p=80,80,x,$dispValueX,$dispValueY,80;b=5,5,,#408080,#000000,1");
	    }
	    
	    cross[min(numPorts,numPorts) - 1]: Cross
	    {
	        parameters:
	            dispValueX = (numPorts - 2 - index) * 80;
	            dispValueY = (numPorts - 2 - index) * 80;
	        @display("p=80,80,x,$dispValueX,$dispValueY,80;b=5,5,,#408080,#000000,1");
	    }
	    

	connections allowunconnected:
	    // Connect switches together
	    for i = 0 .. numPorts -1 ,
	    for j = 0 .. numPorts - 1
	    {
	        switch[j + i * numPorts].photonicHorizIn <--> line_horiz[j - 1 + i * (numPorts-1)].photonicOut if  (j!= 0) && !(j == numPorts - 1 && i == numPorts - 1) && !(numPorts - 1 - j==numPorts - 1 - i);
	        switch[j + i * numPorts].photonicHorizOut <--> line_horiz[j + i * (numPorts-1)].photonicIn if j != numPorts - 1 && !(numPorts - 1 - j==numPorts - 1 - i);
	        
	        switch[j + i * numPorts].photonicVertIn <--> line_vert[j + i * numPorts].photonicOut if i != numPorts - 1 && !(numPorts - 1 - j==numPorts - 1 - i);
	        switch[j + i * numPorts].photonicVertOut <--> line_vert[j + (i - 1) * numPorts].photonicIn if i != 0 && !(j==numPorts - 1 && i == numPorts - 1) && !(numPorts - 1 - j==numPorts - 1 - i);
	    
	    	cross[min(numPorts,numPorts) - 2 - ((numPorts<numPorts)?i:j)].photonicHorizIn <--> line_horiz[j - 1 + i * (numPorts-1)].photonicOut if (numPorts - 1 -j==numPorts - 1 - i) && (j!= 0) && !(j == numPorts - 1 && i == numPorts - 1);
	    	cross[min(numPorts,numPorts) - 2 - ((numPorts<numPorts)?i:j)].photonicHorizOut <--> line_horiz[j + i * (numPorts-1)].photonicIn if (numPorts - 1 -j==numPorts - 1 - i) && j != numPorts - 1 && !(j == numPorts - 1 && i == numPorts - 1);
	    	cross[min(numPorts,numPorts) - 2 - ((numPorts<numPorts)?i:j)].photonicVertIn <--> line_vert[j + i * numPorts].photonicOut if (numPorts - 1 -j==numPorts - 1 - i) && i != numPorts - 1 && !(j==numPorts - 1 && i == numPorts - 1);
	    	cross[min(numPorts,numPorts) - 2 - ((numPorts<numPorts)?i:j)].photonicVertOut <--> line_vert[j + (i - 1) * numPorts].photonicIn if (numPorts - 1 -j==numPorts - 1 - i) && i != 0 && !(j==numPorts - 1 && i == numPorts - 1);
	    }
	    
	    // Connect final bend optimization
	    bend[0].photonicIn <--> line_horiz[numPorts * (numPorts-1) - 1].photonicOut if numPorts > 1;
	    bend[0].photonicOut <--> line_vert[(numPorts-1) * numPorts - 1].photonicIn if numPorts > 1;
	    
	    // Connect inputs and outputs to module interface
	    for i = 0 .. numPorts - 1
	    {
	        switch[i * numPorts].photonicHorizIn <--> {@display("m=w");} <--> in[i] if !(numPorts==1 && i == numPorts -1)  && !(numPorts - i == numPorts);
	    
	    	cross[min(numPorts,numPorts) - 2].photonicHorizIn <--> {@display("m=w");} <--> in[i] if (numPorts - i == numPorts) && numPorts <= numPorts && !(numPorts==1 && i == numPorts -1);
	    }
	    for i = 0 .. numPorts - 1
	    {
	        switch[i].photonicVertOut <--> {@display("m=n");} <--> out[i] if !(numPorts==1 && i == numPorts -1) && !(numPorts - i == numPorts);
	    	
	    	cross[min(numPorts,numPorts) - 2].photonicVertOut <--> {@display("m=n");} <--> out[i] if (numPorts - i == numPorts) && numPorts >= numPorts && !(numPorts==1 && i == numPorts -1);
	    }
	    
	    bend[0].photonicIn <--> {@display("m=w");} <--> in[numPorts - 1] if numPorts == 1;
	    bend[0].photonicOut <--> {@display("m=n");} <--> out[numPorts - 1] if numPorts == 1;
	    
	    // Connect PSEs to module interface
	    for i = 0 .. numPorts - 1,
	    for j = 0 .. numPorts - 1
	    {
	        switch[j + i * numPorts].fromRouter <-- {@display("ls=red,0");} <-- control[j + i * numPorts] if i!= numPorts - 1 && j != numPorts -1;
	    }
	    
	    
	    // Connect edge of crossbar to loop to feed around
	   	
	   	switch[numPorts*(numPorts-1) - 1].photonicHorizOut <--> bend_right[0].photonicIn if min(numPorts,numPorts)>=2;
	   	switch[numPorts*numPorts - 2].photonicVertIn <--> bend_bottom[0].photonicOut if min(numPorts,numPorts)>=2;
	   
	    for i = 0 .. min(numPorts,numPorts) - 3
	    {
	        switch[(numPorts-min(numPorts,numPorts)+i+1)*numPorts - 1].photonicHorizOut <--> line_horiz_stub[min(numPorts,numPorts) - 3 - i].photonicIn;
	    	line_horiz_stub[min(numPorts,numPorts) - 3 - i].photonicOut <--> bend_right[min(numPorts,numPorts) - 2 - i].photonicIn;
	    }
	    for i = 0 .. min(numPorts,numPorts) - 3
	    {
	        switch[numPorts*numPorts - min(numPorts,numPorts) + i].photonicVertIn <--> line_vert_stub[min(numPorts,numPorts) - 3 - i].photonicOut;
	    	line_vert_stub[min(numPorts,numPorts) - 3 - i].photonicIn <--> bend_bottom[min(numPorts,numPorts) - 2 - i].photonicOut;
	    }
	    
	    for i = 0 .. min(numPorts,numPorts) - 2
	    {
	        bend_right[i].photonicOut <--> line_vert_extend[i].photonicIn;
	        line_vert_extend[i].photonicOut <--> bend[i+1].photonicIn;
	        bend[i+1].photonicOut <--> line_horiz_extend[i].photonicIn;
	        line_horiz_extend[i].photonicOut <--> bend_bottom[i].photonicIn;
	    }

	    
}