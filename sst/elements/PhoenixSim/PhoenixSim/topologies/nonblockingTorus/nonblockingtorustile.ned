package topologies.nonblockingTorus;


// Trying to make it more scalable

import photonic.devices.CrossBidirectional;
import photonic.devices.LineBidirectional;
import topologies.Gateway;
import topologies.nonblockingTorus.NonblockingTorusGatewayRouter;
import topologies.nonblockingTorus.NonblockingTorusNodeRouter;
import photonic.devices.Shift;

import electronicComponents.ElectronicChannel;
import chipComponents.HybridRouter;


module NonblockingTorusTileNE_Empty
{
    parameters:
        int nodeId;
        
        int numIF;
		int numX;
		
        double PathSeparation;
        double coreSizeX;
        double coreSizeY;
        int numOfWavelengthChannels;

        int dispValueX;
        int dispValueY;
        int dispValueZ;

    gates:
        inout photonicInN0;
        inout photonicInN1;
        inout photonicInS0;
        inout photonicInS1;
        inout photonicInE0;
        inout photonicInE1;
        inout photonicInW0;
        inout photonicInW1;

        inout photonicOutN0;
        inout photonicOutN1;
        inout photonicOutS0;
        inout photonicOutS1;
        inout photonicOutE0;
        inout photonicOutE1;
        inout photonicOutW0;
        inout photonicOutW1;


        input electronicInN0;
        input electronicInN1;
        input electronicInS0;
        input electronicInS1;
        input electronicInE0;
        input electronicInE1;
        input electronicInW0;
        input electronicInW1;

        output electronicOutN0;
        output electronicOutN1;
        output electronicOutS0;
        output electronicOutS1;
        output electronicOutE0;
        output electronicOutE1;
        output electronicOutW0;
        output electronicOutW1;

       

    submodules:
        Sw4x4_NE: NonblockingTorusNodeRouter {
            parameters:
                nodeId = nodeId;
                numX = numX;
                 type = (numIF > 1) ? 52 : 50;
                @display("p=376,200;b=174,174,,,,1");
        }
        Cross_NW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=136,200;b=64,64,,,,1");
        }
        Cross_SE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=384,416;b=64,64,,,,1");
        }
        Cross_SW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=152,416;b=64,64,,,,1");
        }
        Shift_N: Shift {
            parameters:
                Shift = 80;
                @display("p=401,85;b=54,4,,#408080,,1");
        }
        Shift_S: Shift {
            parameters:
                Shift = 80;
                @display("p=353,301;b=54,4,,#408080,,1");
        }
        Shift_E: Shift {
            parameters:
                Shift = 80;
                @display("p=481,165;b=4,54,,#408080,,1");
        }
        Shift_W: Shift {
            parameters:
                Shift = 80;
                @display("p=273,221;b=4,54,,#408080,,1");
        }
        Line_VertTopLeft: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,132;b=10,44,,#408080,,1");
        }
        Line_VertTopRight: LineBidirectional {
            parameters:
                Length_Line = 138;
                @display("p=136,132;b=10,44,,#408080,,1");
        }
        Line_VertMidLeft: LineBidirectional {
            parameters:
                Length_Line = 258+(coreSizeY-1034);
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_VertMidRight: LineBidirectional {
            parameters:
                Length_Line = 138+(coreSizeY-1034);
                @display("p=377,340;b=12,44,,#408080,,1");
        }
        Line_VertBottomLeft: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_VertBottomRight: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_HorizRightTop: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=448,424;b=28,4,,#408080,,1");
        }
        Line_HorizRightBottom: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=448,424;b=28,4,,#408080,,1");
        }
        Line_HorizMidTop: LineBidirectional {
            parameters:
                Length_Line = coreSizeX-620;
                @display("p=224,208;b=74,4,,#408080,,1");
        }
        Line_HorizMidBottom: LineBidirectional {
            parameters:
                Length_Line = 120+(coreSizeX-620);
                @display("p=224,208;b=74,4,,#408080,,1");
        }
        Line_HorizLeftTop: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=273,421;b=90,10,,#408080,,1");
        }
        Line_HorizLeftBottom: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=273,421;b=90,10,,#408080,,1");
        }
    connections allowunconnected:
        Cross_NW.photonicOutN <--> Line_VertTopLeft.photonicInA;
        Cross_NW.photonicOutS <--> Line_VertMidLeft.photonicInB;
        Cross_NW.photonicOutE <--> Line_HorizMidTop.photonicInB;
        Cross_NW.photonicOutW <--> Line_HorizLeftTop.photonicInA;

        Line_VertTopLeft.photonicOutB <--> Cross_NW.photonicInN;
        Line_VertMidLeft.photonicOutA <--> Cross_NW.photonicInS;
        Line_HorizMidTop.photonicOutA <--> Cross_NW.photonicInE;
        Line_HorizLeftTop.photonicOutB <--> Cross_NW.photonicInW;

        Cross_SE.photonicOutN <--> Line_VertMidRight.photonicInA;
        Cross_SE.photonicOutS <--> Line_VertBottomRight.photonicInB;
        Cross_SE.photonicOutE <--> Line_HorizRightBottom.photonicInB;
        Cross_SE.photonicOutW <--> Line_HorizMidBottom.photonicInA;

        Line_VertMidRight.photonicOutB <--> Cross_SE.photonicInN;
        Line_VertBottomRight.photonicOutA <--> Cross_SE.photonicInS;
        Line_HorizRightBottom.photonicOutA <--> Cross_SE.photonicInE;
        Line_HorizMidBottom.photonicOutB <--> Cross_SE.photonicInW;

        Cross_SW.photonicOutN <--> Line_VertMidLeft.photonicInA;
        Cross_SW.photonicOutS <--> Line_VertBottomLeft.photonicInB;
        Cross_SW.photonicOutE <--> Line_HorizMidBottom.photonicInB;
        Cross_SW.photonicOutW <--> Line_HorizLeftBottom.photonicInA;

        Line_VertMidLeft.photonicOutB <--> Cross_SW.photonicInN;
        Line_VertBottomLeft.photonicOutA <--> Cross_SW.photonicInS;
        Line_HorizMidBottom.photonicOutA <--> Cross_SW.photonicInE;
        Line_HorizLeftBottom.photonicOutB <--> Cross_SW.photonicInW;


        Sw4x4_NE.photonicOutN <--> Shift_N.photonicInA;
        Sw4x4_NE.photonicOutS <--> Shift_S.photonicInB;
        Sw4x4_NE.photonicOutE <--> Shift_E.photonicInB;
        Sw4x4_NE.photonicOutW <--> Shift_W.photonicInA;

        Shift_N.photonicOutB <--> Sw4x4_NE.photonicInN;
        Shift_S.photonicOutA <--> Sw4x4_NE.photonicInS;
        Shift_E.photonicOutA <--> Sw4x4_NE.photonicInE;
        Shift_W.photonicOutB <--> Sw4x4_NE.photonicInW;


        Shift_N.photonicOutA <--> Line_VertTopRight.photonicInA;
        Line_VertTopRight.photonicOutB <--> Shift_N.photonicInB;
        Shift_S.photonicOutB <--> Line_VertMidRight.photonicInB;
        Line_VertMidRight.photonicOutA <--> Shift_S.photonicInA;
        Shift_E.photonicOutB <--> Line_HorizRightTop.photonicInB;
        Line_HorizRightTop.photonicOutA <--> Shift_E.photonicInA;
        Shift_W.photonicOutA <--> Line_HorizMidTop.photonicInA;
        Line_HorizMidTop.photonicOutB <--> Shift_W.photonicInB;


        Line_VertTopLeft.photonicOutA <--> photonicOutN0;
        photonicInN0 <--> Line_VertTopLeft.photonicInB;
        Line_VertTopRight.photonicOutA <--> photonicOutN1;
        photonicInN1 <--> Line_VertTopRight.photonicInB;

        Line_VertBottomLeft.photonicOutB <--> photonicOutS0;
        photonicInS0 <--> Line_VertBottomLeft.photonicInA;
        Line_VertBottomRight.photonicOutB <--> photonicOutS1;
        photonicInS1 <--> Line_VertBottomRight.photonicInA;

        Line_HorizRightTop.photonicOutB <--> photonicOutE0;
        photonicInE0 <--> Line_HorizRightTop.photonicInA;
        Line_HorizRightBottom.photonicOutB <--> photonicOutE1;
        photonicInE1 <--> Line_HorizRightBottom.photonicInA;

        Line_HorizLeftTop.photonicOutA <--> photonicOutW0;
        photonicInW0 <--> Line_HorizLeftTop.photonicInB;
        Line_HorizLeftBottom.photonicOutA <--> photonicOutW1;
        photonicInW1 <--> Line_HorizLeftBottom.photonicInB;

        electronicInN0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutS0;
        electronicInN1 --> { @display("ls=red,0"); } --> Sw4x4_NE.electronicInN;
        Sw4x4_NE.electronicOutS --> { @display("ls=red,0"); } --> electronicOutS1;
        electronicInS0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutN0;
        electronicInS1 --> { @display("ls=red,0"); } --> Sw4x4_NE.electronicInS;
        Sw4x4_NE.electronicOutN --> { @display("ls=red,0"); } --> electronicOutN1;
        electronicInE0 --> { @display("ls=red,0"); } --> Sw4x4_NE.electronicInE;
        Sw4x4_NE.electronicOutW --> { @display("ls=red,0"); } --> electronicOutW0;
        electronicInE1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutW1;
        electronicInW0 --> { @display("ls=red,0"); } --> Sw4x4_NE.electronicInW;
        Sw4x4_NE.electronicOutE --> { @display("ls=red,0"); } --> electronicOutE0;
        electronicInW1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutE1;

      
}





module NonblockingTorusTileNW_Empty
{
    // Minimum Tile Size = 500um x 500um
    parameters:
        int nodeId;
 int numIF;
 int numX;
        double PathSeparation;
        double coreSizeX;
        double coreSizeY;
        int numOfWavelengthChannels;

        int dispValueX;
        double dispValueY;
        int dispValueZ;
    gates:
        inout photonicInN0;
        inout photonicInN1;
        inout photonicInS0;
        inout photonicInS1;
        inout photonicInE0;
        inout photonicInE1;
        inout photonicInW0;
        inout photonicInW1;

        inout photonicOutN0;
        inout photonicOutN1;
        inout photonicOutS0;
        inout photonicOutS1;
        inout photonicOutE0;
        inout photonicOutE1;
        inout photonicOutW0;
        inout photonicOutW1;


        input electronicInN0;
        input electronicInN1;
        input electronicInS0;
        input electronicInS1;
        input electronicInE0;
        input electronicInE1;
        input electronicInW0;
        input electronicInW1;

        output electronicOutN0;
        output electronicOutN1;
        output electronicOutS0;
        output electronicOutS1;
        output electronicOutE0;
        output electronicOutE1;
        output electronicOutW0;
        output electronicOutW1;

     

    submodules:
        Sw4x4_NW: NonblockingTorusNodeRouter {
            parameters:
                nodeId = nodeId;
                numX=numX;
                 type = (numIF > 1) ? 52 : 50;
                @display("p=176,184;b=174,174,,,,1");
        }
        Cross_NE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=400,160;b=64,64,,,,1");
        }
        Cross_SE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=376,376;b=64,64,,,,1");
        }
        Cross_SW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=160,384;b=64,64,,,,1");
        }
        Shift_N: Shift {
            parameters:
                Shift = 80;
                @display("p=201,69;b=54,4,,#408080,,1");
        }
        Shift_S: Shift {
            parameters:
                Shift = 80;
                @display("p=153,285;b=54,4,,#408080,,1");
        }
        Shift_E: Shift {
            parameters:
                Shift = 80;
                @display("p=281,141;b=4,54,,#408080,,1");
        }
        Shift_W: Shift {
            parameters:
                Shift = 80;
                @display("p=73,205;b=4,54,,#408080,,1");
        }
        Line_VertTopLeft: LineBidirectional {
            parameters:
                Length_Line = 138;
                @display("p=136,132;b=10,44,,#408080,,1");
        }
        Line_VertTopRight: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=400,76;b=10,44,,#408080,,1");
        }
        Line_VertMidLeft: LineBidirectional {
            parameters:
                Length_Line = 138+(coreSizeY-1034);
                @display("p=168,317;b=12,42,,#408080,,1");
        }
        Line_VertMidRight: LineBidirectional {
            parameters:
                Length_Line = 258+(coreSizeY-1034);
                @display("p=377,340;b=12,44,,#408080,,1");
        }
        Line_VertBottomLeft: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=168,461;b=12,42,,#408080,,1");
        }
        Line_VertBottomRight: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=376,449;b=12,66,,#408080,,1");
        }
        Line_HorizRightTop: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=456,160;b=28,4,,#408080,,1");
        }
        Line_HorizRightBottom: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=448,384;b=28,4,,#408080,,1");
        }
        Line_HorizMidTop: LineBidirectional {
            parameters:
                Length_Line = coreSizeX-620;
                @display("p=328,152;b=74,4,,#408080,,1");
        }
        Line_HorizMidBottom: LineBidirectional {
            parameters:
                Length_Line = 120+(coreSizeX-620);
                @display("p=272,384;b=74,4,,#408080,,1");
        }
        Line_HorizLeftTop: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=41,173;b=26,10,,#408080,,1");
        }
        Line_HorizLeftBottom: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=65,413;b=90,10,,#408080,,1");
        }
    connections allowunconnected:
        Cross_NE.photonicOutN <--> Line_VertTopRight.photonicInA;
        Cross_NE.photonicOutS <--> Line_VertMidRight.photonicInB;
        Cross_NE.photonicOutE <--> Line_HorizRightTop.photonicInB;
        Cross_NE.photonicOutW <--> Line_HorizMidTop.photonicInA;

        Line_VertTopRight.photonicOutB <--> Cross_NE.photonicInN;
        Line_VertMidRight.photonicOutA <--> Cross_NE.photonicInS;
        Line_HorizRightTop.photonicOutA <--> { @display("m=,0,0"); } <--> Cross_NE.photonicInE;
        Line_HorizMidTop.photonicOutB <--> Cross_NE.photonicInW;

        Cross_SE.photonicOutN <--> Line_VertMidRight.photonicInA;
        Cross_SE.photonicOutS <--> Line_VertBottomRight.photonicInB;
        Cross_SE.photonicOutE <--> Line_HorizRightBottom.photonicInB;
        Cross_SE.photonicOutW <--> Line_HorizMidBottom.photonicInA;

        Line_VertMidRight.photonicOutB <--> Cross_SE.photonicInN;
        Line_VertBottomRight.photonicOutA <--> Cross_SE.photonicInS;
        Line_HorizRightBottom.photonicOutA <--> Cross_SE.photonicInE;
        Line_HorizMidBottom.photonicOutB <--> Cross_SE.photonicInW;

        Cross_SW.photonicOutN <--> Line_VertMidLeft.photonicInA;
        Cross_SW.photonicOutS <--> Line_VertBottomLeft.photonicInB;
        Cross_SW.photonicOutE <--> Line_HorizMidBottom.photonicInB;
        Cross_SW.photonicOutW <--> Line_HorizLeftBottom.photonicInA;

        Line_VertMidLeft.photonicOutB <--> Cross_SW.photonicInN;
        Line_VertBottomLeft.photonicOutA <--> Cross_SW.photonicInS;
        Line_HorizMidBottom.photonicOutA <--> Cross_SW.photonicInE;
        Line_HorizLeftBottom.photonicOutB <--> Cross_SW.photonicInW;


        Sw4x4_NW.photonicOutN <--> Shift_N.photonicInA;
        Sw4x4_NW.photonicOutS <--> Shift_S.photonicInB;
        Sw4x4_NW.photonicOutE <--> Shift_E.photonicInB;
        Sw4x4_NW.photonicOutW <--> Shift_W.photonicInA;

        Shift_N.photonicOutB <--> Sw4x4_NW.photonicInN;
        Shift_S.photonicOutA <--> Sw4x4_NW.photonicInS;
        Shift_E.photonicOutA <--> Sw4x4_NW.photonicInE;
        Shift_W.photonicOutB <--> Sw4x4_NW.photonicInW;


        Shift_N.photonicOutA <--> Line_VertTopLeft.photonicInA;
        Line_VertTopLeft.photonicOutB <--> Shift_N.photonicInB;
        Shift_S.photonicOutB <--> Line_VertMidLeft.photonicInB;
        Line_VertMidLeft.photonicOutA <--> Shift_S.photonicInA;
        Shift_E.photonicOutB <--> Line_HorizMidTop.photonicInB;
        Line_HorizMidTop.photonicOutA <--> Shift_E.photonicInA;
        Shift_W.photonicOutA <--> Line_HorizLeftTop.photonicInA;
        Line_HorizLeftTop.photonicOutB <--> Shift_W.photonicInB;


        Line_VertTopLeft.photonicOutA <--> photonicOutN0;
        photonicInN0 <--> Line_VertTopLeft.photonicInB;
        Line_VertTopRight.photonicOutA <--> photonicOutN1;
        photonicInN1 <--> Line_VertTopRight.photonicInB;

        Line_VertBottomLeft.photonicOutB <--> photonicOutS0;
        photonicInS0 <--> Line_VertBottomLeft.photonicInA;
        Line_VertBottomRight.photonicOutB <--> photonicOutS1;
        photonicInS1 <--> Line_VertBottomRight.photonicInA;

        Line_HorizRightTop.photonicOutB <--> photonicOutE0;
        photonicInE0 <--> Line_HorizRightTop.photonicInA;
        Line_HorizRightBottom.photonicOutB <--> photonicOutE1;
        photonicInE1 <--> Line_HorizRightBottom.photonicInA;

        Line_HorizLeftTop.photonicOutA <--> photonicOutW0;
        photonicInW0 <--> Line_HorizLeftTop.photonicInB;
        Line_HorizLeftBottom.photonicOutA <--> photonicOutW1;
        photonicInW1 <--> Line_HorizLeftBottom.photonicInB;

        electronicInN0 --> { @display("ls=red,0"); } --> Sw4x4_NW.electronicInN;
        Sw4x4_NW.electronicOutS --> { @display("ls=red,0"); } --> electronicOutS0;
        electronicInN1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutS1;
        electronicInS0 --> { @display("ls=red,0"); } --> Sw4x4_NW.electronicInS;
        Sw4x4_NW.electronicOutN --> { @display("ls=red,0"); } --> electronicOutN0;
        electronicInS1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutN1;
        electronicInE0 --> { @display("ls=red,0"); } --> Sw4x4_NW.electronicInE;
        Sw4x4_NW.electronicOutW --> { @display("ls=red,0"); } --> electronicOutW0;
        electronicInE1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutW1;
        electronicInW0 --> { @display("ls=red,0"); } --> Sw4x4_NW.electronicInW;
        Sw4x4_NW.electronicOutE --> { @display("ls=red,0"); } --> electronicOutE0;
        electronicInW1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutE1;

      
}







module NonblockingTorusTileSE_Empty
{
    // Minimum Tile Size = 500um x 500um
    parameters:
        int nodeId;
 int numIF;
 int numX;
        double PathSeparation;
        double coreSizeX;
        double coreSizeY;
        int numOfWavelengthChannels;

        int dispValueX;
        double dispValueY;
        int dispValueZ;
    gates:
        inout photonicInN0;
        inout photonicInN1;
        inout photonicInS0;
        inout photonicInS1;
        inout photonicInE0;
        inout photonicInE1;
        inout photonicInW0;
        inout photonicInW1;

        inout photonicOutN0;
        inout photonicOutN1;
        inout photonicOutS0;
        inout photonicOutS1;
        inout photonicOutE0;
        inout photonicOutE1;
        inout photonicOutW0;
        inout photonicOutW1;


        input electronicInN0;
        input electronicInN1;
        input electronicInS0;
        input electronicInS1;
        input electronicInE0;
        input electronicInE1;
        input electronicInW0;
        input electronicInW1;

        output electronicOutN0;
        output electronicOutN1;
        output electronicOutS0;
        output electronicOutS1;
        output electronicOutE0;
        output electronicOutE1;
        output electronicOutW0;
        output electronicOutW1;

      

    submodules:
        Sw4x4_SE: NonblockingTorusNodeRouter {
            parameters:
                nodeId = nodeId;
                numX=numX;
                 type = (numIF > 1) ? 52 : 50;
                @display("p=384,392;b=174,174,,,,1");
        }
        Cross_NW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=184,160;b=64,64,,,,1");
        }
        Cross_NE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=408,144;b=64,64,,,,1");
        }
        Cross_SW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=152,416;b=64,64,,,,1");
        }
        Shift_N: Shift {
            parameters:
                Shift = 80;
                @display("p=409,277;b=54,4,,#408080,,1");
        }
        Shift_S: Shift {
            parameters:
                Shift = 80;
                @display("p=361,493;b=54,4,,#408080,,1");
        }
        Shift_E: Shift {
            parameters:
                Shift = 80;
                @display("p=489,357;b=4,54,,#408080,,1");
        }
        Shift_W: Shift {
            parameters:
                Shift = 80;
                @display("p=281,413;b=4,54,,#408080,,1");
        }
        Line_VertTopLeft: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=184,76;b=10,44,,#408080,,1");
        }
        Line_VertTopRight: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=400,68;b=10,44,,#408080,,1");
        }
        Line_VertMidLeft: LineBidirectional {
            parameters:
                Length_Line = 258+(coreSizeY-1034);
                @display("p=160,281;b=12,66,,#408080,,1");
        }
        Line_VertMidRight: LineBidirectional {
            parameters:
                Length_Line = 138+(coreSizeY-1034);
                @display("p=401,236;b=12,44,,#408080,,1");
        }
        Line_VertBottomLeft: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=152,513;b=12,66,,#408080,,1");
        }
        Line_VertBottomRight: LineBidirectional {
            parameters:
                Length_Line = 138;
                @display("p=392,545;b=12,66,,#408080,,1");
        }
        Line_HorizRightTop: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=512,140;b=60,12,,#408080,,1");
        }
        Line_HorizRightBottom: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=536,372;b=44,12,,#408080,,1");
        }
        Line_HorizMidTop: LineBidirectional {
            parameters:
                Length_Line = 120+(coreSizeX-620);
                @display("p=296,152;b=74,4,,#408080,,1");
        }
        Line_HorizMidBottom: LineBidirectional {
            parameters:
                Length_Line = coreSizeX-620;
                @display("p=232,400;b=74,4,,#408080,,1");
        }
        Line_HorizLeftTop: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=89,157;b=90,10,,#408080,,1");
        }
        Line_HorizLeftBottom: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=73,405;b=90,10,,#408080,,1");
        }
    connections allowunconnected:
        Cross_NW.photonicOutN <--> Line_VertTopLeft.photonicInA;
        Cross_NW.photonicOutS <--> Line_VertMidLeft.photonicInB;
        Cross_NW.photonicOutE <--> Line_HorizMidTop.photonicInB;
        Cross_NW.photonicOutW <--> Line_HorizLeftTop.photonicInA;

        Line_VertTopLeft.photonicOutB <--> Cross_NW.photonicInN;
        Line_VertMidLeft.photonicOutA <--> Cross_NW.photonicInS;
        Line_HorizMidTop.photonicOutA <--> Cross_NW.photonicInE;
        Line_HorizLeftTop.photonicOutB <--> Cross_NW.photonicInW;

        Cross_NE.photonicOutN <--> Line_VertTopRight.photonicInA;
        Cross_NE.photonicOutS <--> Line_VertMidRight.photonicInB;
        Cross_NE.photonicOutE <--> Line_HorizRightTop.photonicInB;
        Cross_NE.photonicOutW <--> Line_HorizMidTop.photonicInA;

        Line_VertTopRight.photonicOutB <--> Cross_NE.photonicInN;
        Line_VertMidRight.photonicOutA <--> Cross_NE.photonicInS;
        Line_HorizRightTop.photonicOutA <--> Cross_NE.photonicInE;
        Line_HorizMidTop.photonicOutB <--> Cross_NE.photonicInW;

        Cross_SW.photonicOutN <--> Line_VertMidLeft.photonicInA;
        Cross_SW.photonicOutS <--> Line_VertBottomLeft.photonicInB;
        Cross_SW.photonicOutE <--> Line_HorizMidBottom.photonicInB;
        Cross_SW.photonicOutW <--> Line_HorizLeftBottom.photonicInA;

        Line_VertMidLeft.photonicOutB <--> Cross_SW.photonicInN;
        Line_VertBottomLeft.photonicOutA <--> Cross_SW.photonicInS;
        Line_HorizMidBottom.photonicOutA <--> Cross_SW.photonicInE;
        Line_HorizLeftBottom.photonicOutB <--> Cross_SW.photonicInW;


        Sw4x4_SE.photonicOutN <--> Shift_N.photonicInA;
        Sw4x4_SE.photonicOutS <--> Shift_S.photonicInB;
        Sw4x4_SE.photonicOutE <--> Shift_E.photonicInB;
        Sw4x4_SE.photonicOutW <--> Shift_W.photonicInA;

        Shift_N.photonicOutB <--> Sw4x4_SE.photonicInN;
        Shift_S.photonicOutA <--> Sw4x4_SE.photonicInS;
        Shift_E.photonicOutA <--> Sw4x4_SE.photonicInE;
        Shift_W.photonicOutB <--> Sw4x4_SE.photonicInW;


        Shift_N.photonicOutA <--> Line_VertMidRight.photonicInA;
        Line_VertMidRight.photonicOutB <--> Shift_N.photonicInB;
        Shift_S.photonicOutB <--> Line_VertBottomRight.photonicInB;
        Line_VertBottomRight.photonicOutA <--> Shift_S.photonicInA;
        Shift_E.photonicOutB <--> Line_HorizRightBottom.photonicInB;
        Line_HorizRightBottom.photonicOutA <--> Shift_E.photonicInA;
        Shift_W.photonicOutA <--> Line_HorizMidBottom.photonicInA;
        Line_HorizMidBottom.photonicOutB <--> Shift_W.photonicInB;


        Line_VertTopLeft.photonicOutA <--> photonicOutN0;
        photonicInN0 <--> Line_VertTopLeft.photonicInB;
        Line_VertTopRight.photonicOutA <--> photonicOutN1;
        photonicInN1 <--> Line_VertTopRight.photonicInB;

        Line_VertBottomLeft.photonicOutB <--> photonicOutS0;
        photonicInS0 <--> Line_VertBottomLeft.photonicInA;
        Line_VertBottomRight.photonicOutB <--> photonicOutS1;
        photonicInS1 <--> Line_VertBottomRight.photonicInA;

        Line_HorizRightTop.photonicOutB <--> photonicOutE0;
        photonicInE0 <--> Line_HorizRightTop.photonicInA;
        Line_HorizRightBottom.photonicOutB <--> photonicOutE1;
        photonicInE1 <--> Line_HorizRightBottom.photonicInA;

        Line_HorizLeftTop.photonicOutA <--> photonicOutW0;
        photonicInW0 <--> { @display("m=,,,100,0"); } <--> Line_HorizLeftTop.photonicInB;
        Line_HorizLeftBottom.photonicOutA <--> photonicOutW1;
        photonicInW1 <--> Line_HorizLeftBottom.photonicInB;


        electronicInN0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutS0;
        electronicInN1 --> { @display("ls=red,0"); } --> Sw4x4_SE.electronicInN;
        Sw4x4_SE.electronicOutS --> { @display("ls=red,0"); } --> electronicOutS1;
        electronicInS0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutN0;
        electronicInS1 --> { @display("ls=red,0"); } --> Sw4x4_SE.electronicInS;
        Sw4x4_SE.electronicOutN --> { @display("ls=red,0"); } --> electronicOutN1;
        electronicInE0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutW0;
        electronicInE1 --> { @display("ls=red,0"); } --> Sw4x4_SE.electronicInE;
        Sw4x4_SE.electronicOutW --> { @display("ls=red,0"); } --> electronicOutW1;
        electronicInW0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutE0;
        electronicInW1 --> { @display("ls=red,0"); } --> Sw4x4_SE.electronicInW;
        Sw4x4_SE.electronicOutE --> { @display("ls=red,0"); } --> electronicOutE1;

      
}





module NonblockingTorusTileSW_Empty
{
    // Minimum Tile Size = 500um x 500um
    parameters:
        int nodeId;
 int numIF;
 int numX;
        double PathSeparation;
        double coreSizeX;
        double coreSizeY;
        int numOfWavelengthChannels;

        int dispValueX;
        double dispValueY;
        int dispValueZ;
        @display("bgb=564,570");
    gates:
        inout photonicInN0;
        inout photonicInN1;
        inout photonicInS0;
        inout photonicInS1;
        inout photonicInE0;
        inout photonicInE1;
        inout photonicInW0;
        inout photonicInW1;

        inout photonicOutN0;
        inout photonicOutN1;
        inout photonicOutS0;
        inout photonicOutS1;
        inout photonicOutE0;
        inout photonicOutE1;
        inout photonicOutW0;
        inout photonicOutW1;


        input electronicInN0;
        input electronicInN1;
        input electronicInS0;
        input electronicInS1;
        input electronicInE0;
        input electronicInE1;
        input electronicInW0;
        input electronicInW1;

        output electronicOutN0;
        output electronicOutN1;
        output electronicOutS0;
        output electronicOutS1;
        output electronicOutE0;
        output electronicOutE1;
        output electronicOutW0;
        output electronicOutW1;

      

    submodules:
        Sw4x4_SW: NonblockingTorusNodeRouter {
            parameters:
                nodeId = nodeId;
                numX=numX;
                 type = (numIF > 1) ? 52 : 50;
                @display("p=176,376;b=174,174,,,,1");
        }
        Cross_NE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=416,144;b=64,64,,,,1");
        }
        Cross_SE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=416,408;b=64,64,,,,1");
        }
        Cross_NW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=152,152;b=64,64,,,,1");
        }
        Shift_N: Shift {
            parameters:
                Shift = 80;
                @display("p=201,261;b=54,4,,#408080,,1");
        }
        Shift_S: Shift {
            parameters:
                Shift = 80;
                @display("p=153,477;b=54,4,,#408080,,1");
        }
        Shift_E: Shift {
            parameters:
                Shift = 80;
                @display("p=281,341;b=4,54,,#408080,,1");
        }
        Shift_W: Shift {
            parameters:
                Shift = 80;
                @display("p=73,397;b=4,54,,#408080,,1");
        }
        Line_VertTopLeft: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=152,76;b=10,44,,#408080,,1");
        }
        Line_VertTopRight: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=416,68;b=10,44,,#408080,,1");
        }
        Line_VertMidLeft: LineBidirectional {
            parameters:
                Length_Line = 138+(coreSizeY-1034);
                @display("p=152,233;b=12,66,,#408080,,1");
        }
        Line_VertMidRight: LineBidirectional {
            parameters:
                Length_Line = 258+(coreSizeY-1034);
                @display("p=417,276;b=12,44,,#408080,,1");
        }
        Line_VertBottomLeft: LineBidirectional {
            parameters:
                Length_Line = 138;
                @display("p=176,529;b=12,66,,#408080,,1");
        }
        Line_VertBottomRight: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=424,505;b=12,66,,#408080,,1");
        }
        Line_HorizRightTop: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=492,137;b=52,18,,#408080,,1");
        }
        Line_HorizRightBottom: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=500,404;b=52,12,,#408080,,1");
        }
        Line_HorizMidTop: LineBidirectional {
            parameters:
                Length_Line = 120+(coreSizeX-620);
                @display("p=288,160;b=74,4,,#408080,,1");
        }
        Line_HorizMidBottom: LineBidirectional {
            parameters:
                Length_Line = coreSizeX-620;
                @display("p=336,360;b=74,4,,#408080,,1");
        }
        Line_HorizLeftTop: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=65,141;b=90,10,,#408080,,1");
        }
        Line_HorizLeftBottom: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=9,381;b=90,10,,#408080,,1");
        }
    connections allowunconnected:
        Cross_NE.photonicOutN <--> Line_VertTopRight.photonicInA;
        Cross_NE.photonicOutS <--> Line_VertMidRight.photonicInB;
        Cross_NE.photonicOutE <--> Line_HorizRightTop.photonicInB;
        Cross_NE.photonicOutW <--> Line_HorizMidTop.photonicInA;

        Line_VertTopRight.photonicOutB <--> Cross_NE.photonicInN;
        Line_VertMidRight.photonicOutA <--> Cross_NE.photonicInS;
        Line_HorizRightTop.photonicOutA <--> Cross_NE.photonicInE;
        Line_HorizMidTop.photonicOutB <--> Cross_NE.photonicInW;

        Cross_SE.photonicOutN <--> Line_VertMidRight.photonicInA;
        Cross_SE.photonicOutS <--> Line_VertBottomRight.photonicInB;
        Cross_SE.photonicOutE <--> Line_HorizRightBottom.photonicInB;
        Cross_SE.photonicOutW <--> Line_HorizMidBottom.photonicInA;

        Line_VertMidRight.photonicOutB <--> Cross_SE.photonicInN;
        Line_VertBottomRight.photonicOutA <--> Cross_SE.photonicInS;
        Line_HorizRightBottom.photonicOutA <--> Cross_SE.photonicInE;
        Line_HorizMidBottom.photonicOutB <--> Cross_SE.photonicInW;

        Cross_NW.photonicOutN <--> Line_VertTopLeft.photonicInA;
        Cross_NW.photonicOutS <--> Line_VertMidLeft.photonicInB;
        Cross_NW.photonicOutE <--> Line_HorizMidTop.photonicInB;
        Cross_NW.photonicOutW <--> Line_HorizLeftTop.photonicInA;

        Line_VertTopLeft.photonicOutB <--> Cross_NW.photonicInN;
        Line_VertMidLeft.photonicOutA <--> Cross_NW.photonicInS;
        Line_HorizMidTop.photonicOutA <--> Cross_NW.photonicInE;
        Line_HorizLeftTop.photonicOutB <--> Cross_NW.photonicInW;


        Sw4x4_SW.photonicOutN <--> Shift_N.photonicInA;
        Sw4x4_SW.photonicOutS <--> Shift_S.photonicInB;
        Sw4x4_SW.photonicOutE <--> Shift_E.photonicInB;
        Sw4x4_SW.photonicOutW <--> Shift_W.photonicInA;

        Shift_N.photonicOutB <--> Sw4x4_SW.photonicInN;
        Shift_S.photonicOutA <--> Sw4x4_SW.photonicInS;
        Shift_E.photonicOutA <--> Sw4x4_SW.photonicInE;
        Shift_W.photonicOutB <--> Sw4x4_SW.photonicInW;


        Shift_N.photonicOutA <--> Line_VertMidLeft.photonicInA;
        Line_VertMidLeft.photonicOutB <--> Shift_N.photonicInB;
        Shift_S.photonicOutB <--> Line_VertBottomLeft.photonicInB;
        Line_VertBottomLeft.photonicOutA <--> Shift_S.photonicInA;
        Shift_E.photonicOutB <--> Line_HorizMidBottom.photonicInB;
        Line_HorizMidBottom.photonicOutA <--> { @display("m=,100,16"); } <--> Shift_E.photonicInA;
        Shift_W.photonicOutA <--> Line_HorizLeftBottom.photonicInA;
        Line_HorizLeftBottom.photonicOutB <--> Shift_W.photonicInB;


        Line_VertTopLeft.photonicOutA <--> photonicOutN0;
        photonicInN0 <--> Line_VertTopLeft.photonicInB;
        Line_VertTopRight.photonicOutA <--> photonicOutN1;
        photonicInN1 <--> Line_VertTopRight.photonicInB;

        Line_VertBottomLeft.photonicOutB <--> photonicOutS0;
        photonicInS0 <--> Line_VertBottomLeft.photonicInA;
        Line_VertBottomRight.photonicOutB <--> photonicOutS1;
        photonicInS1 <--> Line_VertBottomRight.photonicInA;

        Line_HorizRightTop.photonicOutB <--> photonicOutE0;
        photonicInE0 <--> Line_HorizRightTop.photonicInA;
        Line_HorizRightBottom.photonicOutB <--> photonicOutE1;
        photonicInE1 <--> Line_HorizRightBottom.photonicInA;

        Line_HorizLeftTop.photonicOutA <--> photonicOutW0;
        photonicInW0 <--> Line_HorizLeftTop.photonicInB;
        Line_HorizLeftBottom.photonicOutA <--> photonicOutW1;
        photonicInW1 <--> Line_HorizLeftBottom.photonicInB;

        electronicInN0 --> { @display("ls=red,0"); } --> Sw4x4_SW.electronicInN;
        Sw4x4_SW.electronicOutS --> { @display("ls=red,0"); } --> electronicOutS0;
        electronicInN1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutS1;
        electronicInS0 --> { @display("ls=red,0"); } --> Sw4x4_SW.electronicInS;
        Sw4x4_SW.electronicOutN --> { @display("ls=red,0"); } --> electronicOutN0;
        electronicInS1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutN1;
        electronicInE0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutW0;
        electronicInE1 --> { @display("ls=red,0"); } --> Sw4x4_SW.electronicInE;
        Sw4x4_SW.electronicOutW --> { @display("ls=red,0"); } --> electronicOutW1;
        electronicInW0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutE0;
        electronicInW1 --> { @display("ls=red,0"); } --> Sw4x4_SW.electronicInW;
        Sw4x4_SW.electronicOutE --> { @display("ls=red,0"); } --> electronicOutE1;

       
}



//
//
//
//   Filled Nodes, North Side
//
//

module NonblockingTorusTileNE_PopulatedNorth
{
    // Minimum Tile Size = 500um x 500um
    parameters:
        int nodeId;
        
        int numIF;
        int numX;
        
        bool useIOplane;

        double PathSeparation;
        double coreSizeX;
        double coreSizeY;
        int numOfWavelengthChannels;

        int dispValueX;
        double dispValueY;
        int dispValueZ;
    gates:

        inout electronicGatewayIn;
        inout electronicGatewayOut;

        inout photonicMemoryIn;
        inout photonicMemoryOut;

        inout photonicInN0;
        inout photonicInN1;
        inout photonicInS0;
        inout photonicInS1;
        inout photonicInE0;
        inout photonicInE1;
        inout photonicInW0;
        inout photonicInW1;

        inout photonicOutN0;
        inout photonicOutN1;
        inout photonicOutS0;
        inout photonicOutS1;
        inout photonicOutE0;
        inout photonicOutE1;
        inout photonicOutW0;
        inout photonicOutW1;

        input electronicInN0;
        input electronicInN1;
        input electronicInS0;
        input electronicInS1;
        input electronicInE0;
        input electronicInE1;
        input electronicInW0;
        input electronicInW1;

        output electronicOutN0;
        output electronicOutN1;
        output electronicOutS0;
        output electronicOutS1;
        output electronicOutE0;
        output electronicOutE1;
        output electronicOutW0;
        output electronicOutW1;

     

        input electronicControlInMemory;
        input electronicControlInProc;

        output electronicControlOutMemory;
        output electronicControlOutProc;

     

    submodules:

 		GW: HybridRouter {
     
                 parameters:
                id = string(floor(nodeId/numX)*2+((nodeId%numX)<(numX>>1)?0:1)) + ".";
			
				optSwitch = (useIOplane ? "Gateway" : "Gateway_NoMem");
				
				
				elRouter = 1;
				
				level = "NET";
				
				numX = numX;
				numY = 2;
				 
        }
        
        
        GWSwitch: NonblockingTorusGatewayRouter {
            parameters:
                nodeId = nodeId;
                numX=numX;
                 type = (numIF > 1) ? 53 : 51;
                @display("p=238,525;b=58,26,,,,1");
        }
        Sw4x4_NE: NonblockingTorusNodeRouter {
            parameters:
                nodeId = nodeId;
                numX=numX;
                 type = (numIF > 1) ? 52 : 50;
                @display("p=197,187;b=174,174,,,,1");
        }
        Cross_NW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=136,200;b=64,64,,,,1");
        }
        Cross_SE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=384,416;b=64,64,,,,1");
        }
        Cross_SW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=152,416;b=64,64,,,,1");
        }
        Shift_N: Shift {
            parameters:
                Shift = 80;
                @display("p=401,85;b=54,4,,#408080,,1");
        }
        Shift_S: Shift {
            parameters:
                Shift = 80;
                @display("p=353,301;b=54,4,,#408080,,1");
        }
        Shift_E: Shift {
            parameters:
                Shift = 80;
                @display("p=481,165;b=4,54,,#408080,,1");
        }
        Shift_W: Shift {
            parameters:
                Shift = 80;
                @display("p=273,221;b=4,54,,#408080,,1");
        }
        Line_VertTopLeft: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,132;b=10,44,,#408080,,1");
        }
        Line_VertTopRight: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=377,27;b=10,44,,#408080,,1");
        }
        Line_VertMidLeft: LineBidirectional {
            parameters:
                Length_Line = 258+(coreSizeY-1034);
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_VertMidRight: LineBidirectional {
            parameters:
                Length_Line = 138+(coreSizeY-1034);
                @display("p=377,340;b=12,44,,#408080,,1");
        }
        Line_VertBottomLeft: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_VertBottomRight: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_HorizRightTop: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=448,424;b=28,4,,#408080,,1");
        }
        Line_HorizRightBottom: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=448,424;b=28,4,,#408080,,1");
        }
        Line_HorizMidTop: LineBidirectional {
            parameters:
                Length_Line = coreSizeX-620;
                @display("p=224,208;b=74,4,,#408080,,1");
        }
        Line_HorizMidBottom: LineBidirectional {
            parameters:
                Length_Line = 120+(coreSizeX-620);
                @display("p=273,416;b=74,4,,#408080,,1");
        }
        Line_HorizLeftTop: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=54,200;b=90,10,,#408080,,1");
        }
        Line_HorizLeftBottom: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=54,416;b=90,10,,#408080,,1");
        }
    connections allowunconnected:
        Cross_NW.photonicOutN <--> Line_VertTopLeft.photonicInA;
        Cross_NW.photonicOutS <--> Line_VertMidLeft.photonicInB;
        Cross_NW.photonicOutE <--> Line_HorizMidTop.photonicInB;
        Cross_NW.photonicOutW <--> Line_HorizLeftTop.photonicInA;

        Line_VertTopLeft.photonicOutB <--> Cross_NW.photonicInN;
        Line_VertMidLeft.photonicOutA <--> Cross_NW.photonicInS;
        Line_HorizMidTop.photonicOutA <--> Cross_NW.photonicInE;
        Line_HorizLeftTop.photonicOutB <--> Cross_NW.photonicInW;

        Cross_SE.photonicOutN <--> Line_VertMidRight.photonicInA;
        Cross_SE.photonicOutS <--> Line_VertBottomRight.photonicInB;
        Cross_SE.photonicOutE <--> Line_HorizRightBottom.photonicInB;
        Cross_SE.photonicOutW <--> Line_HorizMidBottom.photonicInA;

        Line_VertMidRight.photonicOutB <--> Cross_SE.photonicInN;
        Line_VertBottomRight.photonicOutA <--> Cross_SE.photonicInS;
        Line_HorizRightBottom.photonicOutA <--> Cross_SE.photonicInE;
        Line_HorizMidBottom.photonicOutB <--> Cross_SE.photonicInW;

        Cross_SW.photonicOutN <--> Line_VertMidLeft.photonicInA;
        Cross_SW.photonicOutS <--> Line_VertBottomLeft.photonicInB;
        Cross_SW.photonicOutE <--> Line_HorizMidBottom.photonicInB;
        Cross_SW.photonicOutW <--> Line_HorizLeftBottom.photonicInA;

        Line_VertMidLeft.photonicOutB <--> Cross_SW.photonicInN;
        Line_VertBottomLeft.photonicOutA <--> Cross_SW.photonicInS;
        Line_HorizMidBottom.photonicOutA <--> Cross_SW.photonicInE;
        Line_HorizLeftBottom.photonicOutB <--> Cross_SW.photonicInW;


        Sw4x4_NE.photonicOutN <--> Shift_N.photonicInA;
        Sw4x4_NE.photonicOutS <--> Shift_S.photonicInB;
        Sw4x4_NE.photonicOutE <--> Shift_E.photonicInB;
        Sw4x4_NE.photonicOutW <--> Shift_W.photonicInA;

        Shift_N.photonicOutB <--> Sw4x4_NE.photonicInN;
        Shift_S.photonicOutA <--> Sw4x4_NE.photonicInS;
        Shift_E.photonicOutA <--> Sw4x4_NE.photonicInE;
        Shift_W.photonicOutB <--> Sw4x4_NE.photonicInW;


        Shift_S.photonicOutB <--> Line_VertMidRight.photonicInB;
        Line_VertMidRight.photonicOutA <--> Shift_S.photonicInA;
        Shift_E.photonicOutB <--> Line_HorizRightTop.photonicInB;
        Line_HorizRightTop.photonicOutA <--> Shift_E.photonicInA;
        Shift_W.photonicOutA <--> Line_HorizMidTop.photonicInA;
        Line_HorizMidTop.photonicOutB <--> Shift_W.photonicInB;


        Line_VertTopLeft.photonicOutA <--> photonicOutN0;
        photonicInN0 <--> Line_VertTopLeft.photonicInB;
        Line_VertTopRight.photonicOutA <--> photonicOutN1;
        photonicInN1 <--> Line_VertTopRight.photonicInB;

        Line_VertBottomLeft.photonicOutB <--> photonicOutS0;
        photonicInS0 <--> Line_VertBottomLeft.photonicInA;
        Line_VertBottomRight.photonicOutB <--> photonicOutS1;
        photonicInS1 <--> Line_VertBottomRight.photonicInA;

        Line_HorizRightTop.photonicOutB <--> photonicOutE0;
        photonicInE0 <--> Line_HorizRightTop.photonicInA;
        Line_HorizRightBottom.photonicOutB <--> photonicOutE1;
        photonicInE1 <--> Line_HorizRightBottom.photonicInA;

        Line_HorizLeftTop.photonicOutA <--> photonicOutW0;
        photonicInW0 <--> Line_HorizLeftTop.photonicInB;
        Line_HorizLeftBottom.photonicOutA <--> photonicOutW1;
        photonicInW1 <--> Line_HorizLeftBottom.photonicInB;

        GWSwitch.photonicOutInjectionN <--> Line_VertTopRight.photonicInA;
        GWSwitch.photonicOutInjectionS <--> Shift_N.photonicInB;
        Shift_N.photonicOutA <--> GWSwitch.photonicInEjection;

        GWSwitch.photonicOutGateway <--> GW.phIn[1];
        GW.phOut[1] <--> GWSwitch.photonicInGateway;

		
    	electronicGatewayIn <--> { @display("ls=,0"); } <--> GW.phIn[0];
    	GW.phOut[0] <--> { @display("ls=,0"); } <--> electronicGatewayOut;
    	
    	electronicControlInProc --> { @display("ls=,0"); } --> GW.elIn[0];
    	electronicControlOutProc <-- { @display("ls=,0"); } <-- GW.elOut[0];
    	
		

        photonicMemoryIn <--> { @display("ls=,0"); } <--> GW.phIn[2];
        GW.phOut[2] <--> { @display("ls=,0"); } <--> photonicMemoryOut;

        electronicControlInMemory --> { @display("ls=,0"); } --> GW.elIn[2];
       

        electronicControlOutMemory <-- { @display("ls=,0"); } <-- GW.elOut[2];


        GWSwitch.electronicOutGateway --> ElectronicChannel --> GW.elIn[1];
        GWSwitch.electronicInGateway <-- ElectronicChannel <-- GW.elOut[1];
       

        electronicInN0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutS0;
        electronicInN1 --> { @display("ls=red,0"); } --> GWSwitch.electronicInInjectionN;
        GWSwitch.electronicOutInjectionS --> ElectronicChannel { @display("ls=red,0"); } --> Sw4x4_NE.electronicInN;
        Sw4x4_NE.electronicOutS --> { @display("ls=red,0"); } --> electronicOutS1;
        electronicInS0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutN0;
        electronicInS1 --> { @display("ls=red,0"); } --> Sw4x4_NE.electronicInS;
        Sw4x4_NE.electronicOutN --> ElectronicChannel { @display("ls=red,0"); } --> GWSwitch.electronicInInjectionS;
        GWSwitch.electronicOutInjectionN --> electronicOutN1;
        electronicInE0 --> { @display("ls=red,0"); } --> Sw4x4_NE.electronicInE;
        Sw4x4_NE.electronicOutW --> { @display("ls=red,0"); } --> electronicOutW0;
        electronicInE1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutW1;
        electronicInW0 --> { @display("ls=red,0"); } --> Sw4x4_NE.electronicInW;
        Sw4x4_NE.electronicOutE --> { @display("ls=red,0"); } --> electronicOutE0;
        electronicInW1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutE1;


    


}





module NonblockingTorusTileNW_PopulatedNorth
{
    // Minimum Tile Size = 500um x 500um
    parameters:
        int nodeId;
        
        int numIF;
        int numX;
        
        bool useIOplane;

        double PathSeparation;
        double coreSizeX;
        double coreSizeY;
        int numOfWavelengthChannels;

        int dispValueX;
        double dispValueY;
        int dispValueZ;
    gates:

        inout electronicGatewayIn;
        inout electronicGatewayOut;

        inout photonicMemoryIn;
        inout photonicMemoryOut;

        inout photonicInN0;
        inout photonicInN1;
        inout photonicInS0;
        inout photonicInS1;
        inout photonicInE0;
        inout photonicInE1;
        inout photonicInW0;
        inout photonicInW1;

        inout photonicOutN0;
        inout photonicOutN1;
        inout photonicOutS0;
        inout photonicOutS1;
        inout photonicOutE0;
        inout photonicOutE1;
        inout photonicOutW0;
        inout photonicOutW1;



        input electronicInN0;
        input electronicInN1;
        input electronicInS0;
        input electronicInS1;
        input electronicInE0;
        input electronicInE1;
        input electronicInW0;
        input electronicInW1;

        output electronicOutN0;
        output electronicOutN1;
        output electronicOutS0;
        output electronicOutS1;
        output electronicOutE0;
        output electronicOutE1;
        output electronicOutW0;
        output electronicOutW1;

     


        input electronicControlInMemory;
        input electronicControlInProc;

        output electronicControlOutMemory;
        output electronicControlOutProc;

    submodules:

        GW: HybridRouter {
     
                 parameters:
                id = string(floor(nodeId/numX)*2+((nodeId%numX)<(numX>>1)?0:1)) + ".";
			
					optSwitch = (useIOplane ? "Gateway" : "Gateway_NoMem");
				
				
				elRouter = 1;
				
				level = "NET";
				
				numX = numX;
				numY = 2;
				 
        }
        GWSwitch: NonblockingTorusGatewayRouter {
            parameters:
                nodeId = nodeId;
                numX=numX;
                 type = (numIF > 1) ? 53 : 51;
                @display("p=238,525;b=58,26,,,,1");
        }
        Sw4x4_NW: NonblockingTorusNodeRouter {
            parameters:
                nodeId = nodeId;
                numX=numX;
                 type = (numIF > 1) ? 52 : 50;
                @display("p=197,187;b=174,174,,,,1");
        }
        Cross_NE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=493,142;b=64,64,,,,1");
        }
        Cross_SE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=384,416;b=64,64,,,,1");
        }
        Cross_SW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=152,416;b=64,64,,,,1");
        }
        Shift_N: Shift {
            parameters:
                Shift = 80;
                @display("p=222,72;b=54,4,,#408080,,1");
        }
        Shift_S: Shift {
            parameters:
                Shift = 80;
                @display("p=174,288;b=54,4,,#408080,,1");
        }
        Shift_E: Shift {
            parameters:
                Shift = 80;
                @display("p=302,152;b=4,54,,#408080,,1");
        }
        Shift_W: Shift {
            parameters:
                Shift = 80;
                @display("p=94,208;b=4,54,,#408080,,1");
        }
        Line_VertTopLeft: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=136,132;b=10,44,,#408080,,1");
        }
        Line_VertTopRight: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=515,52;b=10,44,,#408080,,1");
        }
        Line_VertMidLeft: LineBidirectional {
            parameters:
                Length_Line = 138+(coreSizeY-1034);
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_VertMidRight: LineBidirectional {
            parameters:
                Length_Line = 258+(coreSizeY-1034);
                @display("p=377,340;b=12,44,,#408080,,1");
        }
        Line_VertBottomLeft: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_VertBottomRight: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_HorizRightTop: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=448,424;b=28,4,,#408080,,1");
        }
        Line_HorizRightBottom: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=448,424;b=28,4,,#408080,,1");
        }
        Line_HorizMidTop: LineBidirectional {
            parameters:
                Length_Line = coreSizeX-620;
                @display("p=224,208;b=74,4,,#408080,,1");
        }
        Line_HorizMidBottom: LineBidirectional {
            parameters:
                Length_Line = 120+(coreSizeX-620);
                @display("p=369,164;b=74,4,,#408080,,1");
        }
        Line_HorizLeftTop: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=273,421;b=90,10,,#408080,,1");
        }
        Line_HorizLeftBottom: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=51,277;b=90,10,,#408080,,1");
        }
    connections allowunconnected:
        Cross_NE.photonicOutN <--> Line_VertTopRight.photonicInA;
        Cross_NE.photonicOutS <--> Line_VertMidRight.photonicInB;
        Cross_NE.photonicOutE <--> Line_HorizRightTop.photonicInB;
        Cross_NE.photonicOutW <--> Line_HorizMidTop.photonicInA;

        Line_VertTopRight.photonicOutB <--> Cross_NE.photonicInN;
        Line_VertMidRight.photonicOutA <--> Cross_NE.photonicInS;
        Line_HorizRightTop.photonicOutA <--> Cross_NE.photonicInE;
        Line_HorizMidTop.photonicOutB <--> Cross_NE.photonicInW;

        Cross_SE.photonicOutN <--> Line_VertMidRight.photonicInA;
        Cross_SE.photonicOutS <--> Line_VertBottomRight.photonicInB;
        Cross_SE.photonicOutE <--> Line_HorizRightBottom.photonicInB;
        Cross_SE.photonicOutW <--> Line_HorizMidBottom.photonicInA;

        Line_VertMidRight.photonicOutB <--> Cross_SE.photonicInN;
        Line_VertBottomRight.photonicOutA <--> Cross_SE.photonicInS;
        Line_HorizRightBottom.photonicOutA <--> Cross_SE.photonicInE;
        Line_HorizMidBottom.photonicOutB <--> Cross_SE.photonicInW;

        Cross_SW.photonicOutN <--> Line_VertMidLeft.photonicInA;
        Cross_SW.photonicOutS <--> Line_VertBottomLeft.photonicInB;
        Cross_SW.photonicOutE <--> Line_HorizMidBottom.photonicInB;
        Cross_SW.photonicOutW <--> Line_HorizLeftBottom.photonicInA;

        Line_VertMidLeft.photonicOutB <--> Cross_SW.photonicInN;
        Line_VertBottomLeft.photonicOutA <--> Cross_SW.photonicInS;
        Line_HorizMidBottom.photonicOutA <--> Cross_SW.photonicInE;
        Line_HorizLeftBottom.photonicOutB <--> Cross_SW.photonicInW;


        Sw4x4_NW.photonicOutN <--> Shift_N.photonicInA;
        Sw4x4_NW.photonicOutS <--> Shift_S.photonicInB;
        Sw4x4_NW.photonicOutE <--> Shift_E.photonicInB;
        Sw4x4_NW.photonicOutW <--> Shift_W.photonicInA;

        Shift_N.photonicOutB <--> Sw4x4_NW.photonicInN;
        Shift_S.photonicOutA <--> Sw4x4_NW.photonicInS;
        Shift_E.photonicOutA <--> Sw4x4_NW.photonicInE;
        Shift_W.photonicOutB <--> Sw4x4_NW.photonicInW;


        Shift_S.photonicOutB <--> Line_VertMidLeft.photonicInB;
        Line_VertMidLeft.photonicOutA <--> Shift_S.photonicInA;
        Shift_E.photonicOutB <--> Line_HorizMidTop.photonicInB;
        Line_HorizMidTop.photonicOutA <--> Shift_E.photonicInA;
        Shift_W.photonicOutA <--> Line_HorizLeftTop.photonicInA;
        Line_HorizLeftTop.photonicOutB <--> Shift_W.photonicInB;


        Line_VertTopLeft.photonicOutA <--> photonicOutN0;
        photonicInN0 <--> Line_VertTopLeft.photonicInB;
        Line_VertTopRight.photonicOutA <--> photonicOutN1;
        photonicInN1 <--> Line_VertTopRight.photonicInB;

        Line_VertBottomLeft.photonicOutB <--> photonicOutS0;
        photonicInS0 <--> Line_VertBottomLeft.photonicInA;
        Line_VertBottomRight.photonicOutB <--> photonicOutS1;
        photonicInS1 <--> Line_VertBottomRight.photonicInA;

        Line_HorizRightTop.photonicOutB <--> photonicOutE0;
        photonicInE0 <--> Line_HorizRightTop.photonicInA;
        Line_HorizRightBottom.photonicOutB <--> photonicOutE1;
        photonicInE1 <--> Line_HorizRightBottom.photonicInA;

        Line_HorizLeftTop.photonicOutA <--> photonicOutW0;
        photonicInW0 <--> Line_HorizLeftTop.photonicInB;
        Line_HorizLeftBottom.photonicOutA <--> photonicOutW1;
        photonicInW1 <--> Line_HorizLeftBottom.photonicInB;


        GWSwitch.photonicOutInjectionN <--> Line_VertTopLeft.photonicInA;
        GWSwitch.photonicOutInjectionS <--> Shift_N.photonicInB;
        Shift_N.photonicOutA <--> GWSwitch.photonicInEjection;

        GWSwitch.photonicOutGateway <--> GW.phIn[1];
        GW.phOut[1] <--> GWSwitch.photonicInGateway;

		
    	electronicGatewayIn <--> { @display("ls=,0"); } <--> GW.phIn[0];
    	GW.phOut[0] <--> { @display("ls=,0"); } <--> electronicGatewayOut;
    	
    	electronicControlInProc --> { @display("ls=,0"); } --> GW.elIn[0];
    	electronicControlOutProc <-- { @display("ls=,0"); } <-- GW.elOut[0];
        
		
        photonicMemoryIn <--> { @display("ls=,0"); } <--> GW.phIn[2];
        GW.phOut[2] <--> { @display("ls=,0"); } <--> photonicMemoryOut;

        electronicControlInMemory --> { @display("ls=,0"); } --> GW.elIn[2];
  
        electronicControlOutMemory <-- { @display("ls=,0"); } <-- GW.elOut[2];
      



        GWSwitch.electronicOutGateway --> ElectronicChannel --> GW.elIn[1];
        GWSwitch.electronicInGateway <-- ElectronicChannel <-- GW.elOut[1];
     

        electronicInN0 --> { @display("ls=red,0"); } --> GWSwitch.electronicInInjectionN;
        GWSwitch.electronicOutInjectionS --> ElectronicChannel { @display("ls=red,0"); } --> Sw4x4_NW.electronicInN;
        Sw4x4_NW.electronicOutS --> { @display("ls=red,0"); } --> electronicOutS0;
        electronicInN1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutS1;
        electronicInS0 --> { @display("ls=red,0"); } --> Sw4x4_NW.electronicInS;
        Sw4x4_NW.electronicOutN --> ElectronicChannel { @display("ls=red,0"); } --> GWSwitch.electronicInInjectionS;
        GWSwitch.electronicOutInjectionN --> electronicOutN0;
        electronicInS1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutN1;
        electronicInE0 --> { @display("ls=red,0"); } --> Sw4x4_NW.electronicInE;
        Sw4x4_NW.electronicOutW --> { @display("ls=red,0"); } --> electronicOutW0;
        electronicInE1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutW1;
        electronicInW0 --> { @display("ls=red,0"); } --> Sw4x4_NW.electronicInW;
        Sw4x4_NW.electronicOutE --> { @display("ls=red,0"); } --> electronicOutE0;
        electronicInW1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutE1;

      
}







module NonblockingTorusTileSE_PopulatedNorth
{
    // Minimum Tile Size = 500um x 500um
    parameters:
        int nodeId;

  		int numIF;
  		int numX;
  		
  		bool useIOplane;
   
        double PathSeparation;
        double coreSizeX;
        double coreSizeY;
        int numOfWavelengthChannels;

        int dispValueX;
        double dispValueY;
        int dispValueZ;
    gates:

        inout electronicGatewayIn;
        inout electronicGatewayOut;

        inout photonicMemoryIn;
        inout photonicMemoryOut;

        inout photonicInN0;
        inout photonicInN1;
        inout photonicInS0;
        inout photonicInS1;
        inout photonicInE0;
        inout photonicInE1;
        inout photonicInW0;
        inout photonicInW1;

        inout photonicOutN0;
        inout photonicOutN1;
        inout photonicOutS0;
        inout photonicOutS1;
        inout photonicOutE0;
        inout photonicOutE1;
        inout photonicOutW0;
        inout photonicOutW1;



        input electronicInN0;
        input electronicInN1;
        input electronicInS0;
        input electronicInS1;
        input electronicInE0;
        input electronicInE1;
        input electronicInW0;
        input electronicInW1;

        output electronicOutN0;
        output electronicOutN1;
        output electronicOutS0;
        output electronicOutS1;
        output electronicOutE0;
        output electronicOutE1;
        output electronicOutW0;
        output electronicOutW1;

       

        input electronicControlInMemory;
        input electronicControlInProc;

        output electronicControlOutMemory;
        output electronicControlOutProc;

       
    submodules:

        GW: HybridRouter {
     
                 parameters:
                id = string(floor(nodeId/numX)*2+((nodeId%numX)<(numX>>1)?0:1)) + ".";
			
					optSwitch = (useIOplane ? "Gateway" : "Gateway_NoMem");
				
				
				elRouter = 1;
				
				level = "NET";
				
				numX = numX;
				numY = 2;

        }
       GWSwitch: NonblockingTorusGatewayRouter {
            parameters:
                nodeId = nodeId;
                numX=numX;
                 type = (numIF > 1) ? 53 : 51;
                @display("p=238,525;b=58,26,,,,1");
        }
        Sw4x4_SE: NonblockingTorusNodeRouter {
            parameters:
                nodeId = nodeId;
                numX=numX;
                 type = (numIF > 1) ? 52 : 50;
                @display("p=197,187;b=174,174,,,,1");
        }
        Cross_NW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=136,200;b=64,64,,,,1");
        }
        Cross_NE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=384,416;b=64,64,,,,1");
        }
        Cross_SW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=152,416;b=64,64,,,,1");
        }
        Shift_N: Shift {
            parameters:
                Shift = 80;
                @display("p=401,85;b=54,4,,#408080,,1");
        }
        Shift_S: Shift {
            parameters:
                Shift = 80;
                @display("p=353,301;b=54,4,,#408080,,1");
        }
        Shift_E: Shift {
            parameters:
                Shift = 80;
                @display("p=481,165;b=4,54,,#408080,,1");
        }
        Shift_W: Shift {
            parameters:
                Shift = 80;
                @display("p=273,221;b=4,54,,#408080,,1");
        }
        Line_VertTopLeft: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,132;b=10,44,,#408080,,1");
        }
        Line_VertTopRight: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,132;b=10,44,,#408080,,1");
        }
        Line_VertMidLeft: LineBidirectional {
            parameters:
                Length_Line = 258+(coreSizeY-1034);
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_VertMidRight: LineBidirectional {
            parameters:
                Length_Line = 0+(coreSizeY-1034);
                @display("p=377,340;b=12,44,,#408080,,1");
        }
        Line_VertBottomLeft: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_VertBottomRight: LineBidirectional {
            parameters:
                Length_Line = 138;
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_HorizRightTop: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=448,424;b=28,4,,#408080,,1");
        }
        Line_HorizRightBottom: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=448,424;b=28,4,,#408080,,1");
        }
        Line_HorizMidTop: LineBidirectional {
            parameters:
                Length_Line = 120+(coreSizeX-620);
                @display("p=224,208;b=74,4,,#408080,,1");
        }
        Line_HorizMidBottom: LineBidirectional {
            parameters:
                Length_Line = coreSizeX-620;
                @display("p=224,208;b=74,4,,#408080,,1");
        }
        Line_HorizLeftTop: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=273,421;b=90,10,,#408080,,1");
        }
        Line_HorizLeftBottom: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=273,421;b=90,10,,#408080,,1");
        }
    connections allowunconnected:
        Cross_NW.photonicOutN <--> Line_VertTopLeft.photonicInA;
        Cross_NW.photonicOutS <--> Line_VertMidLeft.photonicInB;
        Cross_NW.photonicOutE <--> Line_HorizMidTop.photonicInB;
        Cross_NW.photonicOutW <--> Line_HorizLeftTop.photonicInA;

        Line_VertTopLeft.photonicOutB <--> Cross_NW.photonicInN;
        Line_VertMidLeft.photonicOutA <--> Cross_NW.photonicInS;
        Line_HorizMidTop.photonicOutA <--> Cross_NW.photonicInE;
        Line_HorizLeftTop.photonicOutB <--> Cross_NW.photonicInW;

        Cross_NE.photonicOutN <--> Line_VertTopRight.photonicInA;
        Cross_NE.photonicOutS <--> Line_VertMidRight.photonicInB;
        Cross_NE.photonicOutE <--> Line_HorizRightTop.photonicInB;
        Cross_NE.photonicOutW <--> Line_HorizMidTop.photonicInA;

        Line_VertTopRight.photonicOutB <--> Cross_NE.photonicInN;
        Line_VertMidRight.photonicOutA <--> Cross_NE.photonicInS;
        Line_HorizRightTop.photonicOutA <--> Cross_NE.photonicInE;
        Line_HorizMidTop.photonicOutB <--> Cross_NE.photonicInW;

        Cross_SW.photonicOutN <--> Line_VertMidLeft.photonicInA;
        Cross_SW.photonicOutS <--> Line_VertBottomLeft.photonicInB;
        Cross_SW.photonicOutE <--> Line_HorizMidBottom.photonicInB;
        Cross_SW.photonicOutW <--> Line_HorizLeftBottom.photonicInA;

        Line_VertMidLeft.photonicOutB <--> Cross_SW.photonicInN;
        Line_VertBottomLeft.photonicOutA <--> Cross_SW.photonicInS;
        Line_HorizMidBottom.photonicOutA <--> Cross_SW.photonicInE;
        Line_HorizLeftBottom.photonicOutB <--> Cross_SW.photonicInW;


        Sw4x4_SE.photonicOutN <--> Shift_N.photonicInA;
        Sw4x4_SE.photonicOutS <--> Shift_S.photonicInB;
        Sw4x4_SE.photonicOutE <--> Shift_E.photonicInB;
        Sw4x4_SE.photonicOutW <--> Shift_W.photonicInA;

        Shift_N.photonicOutB <--> Sw4x4_SE.photonicInN;
        Shift_S.photonicOutA <--> Sw4x4_SE.photonicInS;
        Shift_E.photonicOutA <--> Sw4x4_SE.photonicInE;
        Shift_W.photonicOutB <--> Sw4x4_SE.photonicInW;


        Shift_S.photonicOutB <--> Line_VertBottomRight.photonicInB;
        Line_VertBottomRight.photonicOutA <--> Shift_S.photonicInA;
        Shift_E.photonicOutB <--> Line_HorizRightBottom.photonicInB;
        Line_HorizRightBottom.photonicOutA <--> Shift_E.photonicInA;
        Shift_W.photonicOutA <--> Line_HorizMidBottom.photonicInA;
        Line_HorizMidBottom.photonicOutB <--> Shift_W.photonicInB;


        Line_VertTopLeft.photonicOutA <--> photonicOutN0;
        photonicInN0 <--> Line_VertTopLeft.photonicInB;
        Line_VertTopRight.photonicOutA <--> photonicOutN1;
        photonicInN1 <--> Line_VertTopRight.photonicInB;

        Line_VertBottomLeft.photonicOutB <--> photonicOutS0;
        photonicInS0 <--> Line_VertBottomLeft.photonicInA;
        Line_VertBottomRight.photonicOutB <--> photonicOutS1;
        photonicInS1 <--> Line_VertBottomRight.photonicInA;

        Line_HorizRightTop.photonicOutB <--> photonicOutE0;
        photonicInE0 <--> Line_HorizRightTop.photonicInA;
        Line_HorizRightBottom.photonicOutB <--> photonicOutE1;
        photonicInE1 <--> Line_HorizRightBottom.photonicInA;

        Line_HorizLeftTop.photonicOutA <--> photonicOutW0;
        photonicInW0 <--> Line_HorizLeftTop.photonicInB;
        Line_HorizLeftBottom.photonicOutA <--> photonicOutW1;
        photonicInW1 <--> Line_HorizLeftBottom.photonicInB;


        GWSwitch.photonicOutInjectionN <--> Line_VertMidRight.photonicInA;
        GWSwitch.photonicOutInjectionS <--> Shift_N.photonicInB;
        Shift_N.photonicOutA <--> GWSwitch.photonicInEjection;

        GWSwitch.photonicOutGateway <--> GW.phIn[1];
    	GW.phOut[1] <--> GWSwitch.photonicInGateway;
    
    	
    	electronicGatewayIn <--> { @display("ls=,0"); } <--> GW.phIn[0];
    	GW.phOut[0] <--> { @display("ls=,0"); } <--> electronicGatewayOut;
    	
    	electronicControlInProc --> { @display("ls=,0"); } --> GW.elIn[0];
    	electronicControlOutProc <-- { @display("ls=,0"); } <-- GW.elOut[0];
    	
    	
    	photonicMemoryIn <--> { @display("ls=,0"); } <--> GW.phIn[2];
    	GW.phOut[2] <--> { @display("ls=,0"); } <--> photonicMemoryOut;
    
    	electronicControlInMemory --> { @display("ls=,0"); } --> GW.elIn[2];
    
    	electronicControlOutMemory <-- { @display("ls=,0"); } <-- GW.elOut[2];
      
    
    
    
    	GWSwitch.electronicOutGateway --> ElectronicChannel --> GW.elIn[1];
    	GWSwitch.electronicInGateway <-- ElectronicChannel <-- GW.elOut[1];
         


        electronicInN0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutS0;
        electronicInN1 --> { @display("ls=red,0"); } --> GWSwitch.electronicInInjectionN;
        GWSwitch.electronicOutInjectionS --> ElectronicChannel { @display("ls=red,0"); } --> Sw4x4_SE.electronicInN;
        Sw4x4_SE.electronicOutS --> { @display("ls=red,0"); } --> electronicOutS1;
        electronicInS0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutN0;
        electronicInS1 --> { @display("ls=red,0"); } --> Sw4x4_SE.electronicInS;
        Sw4x4_SE.electronicOutN --> ElectronicChannel { @display("ls=red,0"); } --> GWSwitch.electronicInInjectionS;
        GWSwitch.electronicOutInjectionN --> electronicOutN1;
        electronicInE0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutW0;
        electronicInE1 --> { @display("ls=red,0"); } --> Sw4x4_SE.electronicInE;
        Sw4x4_SE.electronicOutW --> { @display("ls=red,0"); } --> electronicOutW1;
        electronicInW0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutE0;
        electronicInW1 --> { @display("ls=red,0"); } --> Sw4x4_SE.electronicInW;
        Sw4x4_SE.electronicOutE --> { @display("ls=red,0"); } --> electronicOutE1;

      

}





module NonblockingTorusTileSW_PopulatedNorth
{
    // Minimum Tile Size = 500um x 500um
    parameters:
        int nodeId;
   		int numIF;
   		int numX;
   		
   		bool useIOplane;
   		
        double PathSeparation;
        double coreSizeX;
        double coreSizeY;
        int numOfWavelengthChannels;

        int dispValueX;
        double dispValueY;
        int dispValueZ;
    gates:

        inout electronicGatewayIn;
        inout electronicGatewayOut;

        inout photonicMemoryIn;
        inout photonicMemoryOut;

        inout photonicInN0;
        inout photonicInN1;
        inout photonicInS0;
        inout photonicInS1;
        inout photonicInE0;
        inout photonicInE1;
        inout photonicInW0;
        inout photonicInW1;

        inout photonicOutN0;
        inout photonicOutN1;
        inout photonicOutS0;
        inout photonicOutS1;
        inout photonicOutE0;
        inout photonicOutE1;
        inout photonicOutW0;
        inout photonicOutW1;


        input electronicInN0;
        input electronicInN1;
        input electronicInS0;
        input electronicInS1;
        input electronicInE0;
        input electronicInE1;
        input electronicInW0;
        input electronicInW1;

        output electronicOutN0;
        output electronicOutN1;
        output electronicOutS0;
        output electronicOutS1;
        output electronicOutE0;
        output electronicOutE1;
        output electronicOutW0;
        output electronicOutW1;

       
        input electronicControlInMemory;
        input electronicControlInProc;

        output electronicControlOutMemory;
        output electronicControlOutProc;

    
    submodules:

        GW: HybridRouter {
     
                 parameters:
                id = string(floor(nodeId/numX)*2+((nodeId%numX)<(numX>>1)?0:1)) + ".";
			
					optSwitch = (useIOplane ? "Gateway" : "Gateway_NoMem");
				
				
				elRouter = 1;
				
				level = "NET";
				
				numX = numX;
				numY = 2;

        }
        GWSwitch: NonblockingTorusGatewayRouter {
            parameters:
                nodeId = nodeId;
                numX=numX;
                 type = (numIF > 1) ? 53 : 51;
                @display("p=238,525;b=58,26,,,,1");
        }
        Sw4x4_SW: NonblockingTorusNodeRouter {
            parameters:
                nodeId = nodeId;
                numX=numX;
                 type = (numIF > 1) ? 52 : 50;
                @display("p=197,187;b=174,174,,,,1");
        }
        Cross_NE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=136,200;b=64,64,,,,1");
        }
        Cross_SE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=384,416;b=64,64,,,,1");
        }
        Cross_NW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=152,416;b=64,64,,,,1");
        }
        Shift_N: Shift {
            parameters:
                Shift = 80;
                @display("p=401,85;b=54,4,,#408080,,1");
        }
        Shift_S: Shift {
            parameters:
                Shift = 80;
                @display("p=353,301;b=54,4,,#408080,,1");
        }
        Shift_E: Shift {
            parameters:
                Shift = 80;
                @display("p=481,165;b=4,54,,#408080,,1");
        }
        Shift_W: Shift {
            parameters:
                Shift = 80;
                @display("p=273,221;b=4,54,,#408080,,1");
        }
        Line_VertTopLeft: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,132;b=10,44,,#408080,,1");
        }
        Line_VertTopRight: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,132;b=10,44,,#408080,,1");
        }
        Line_VertMidLeft: LineBidirectional {
            parameters:
                Length_Line = 0+(coreSizeY-1034);
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_VertMidRight: LineBidirectional {
            parameters:
                Length_Line = 258+(coreSizeY-1034);
                @display("p=377,340;b=12,44,,#408080,,1");
        }
        Line_VertBottomLeft: LineBidirectional {
            parameters:
                Length_Line = 138;
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_VertBottomRight: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_HorizRightTop: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=448,424;b=28,4,,#408080,,1");
        }
        Line_HorizRightBottom: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=448,424;b=28,4,,#408080,,1");
        }
        Line_HorizMidTop: LineBidirectional {
            parameters:
                Length_Line = 120+(coreSizeX-620);
                @display("p=224,208;b=74,4,,#408080,,1");
        }
        Line_HorizMidBottom: LineBidirectional {
            parameters:
                Length_Line = coreSizeX-620;
                @display("p=224,208;b=74,4,,#408080,,1");
        }
        Line_HorizLeftTop: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=273,421;b=90,10,,#408080,,1");
        }
        Line_HorizLeftBottom: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=273,421;b=90,10,,#408080,,1");
        }
    connections allowunconnected:
        Cross_NE.photonicOutN <--> Line_VertTopRight.photonicInA;
        Cross_NE.photonicOutS <--> Line_VertMidRight.photonicInB;
        Cross_NE.photonicOutE <--> Line_HorizRightTop.photonicInB;
        Cross_NE.photonicOutW <--> Line_HorizMidTop.photonicInA;

        Line_VertTopRight.photonicOutB <--> Cross_NE.photonicInN;
        Line_VertMidRight.photonicOutA <--> Cross_NE.photonicInS;
        Line_HorizRightTop.photonicOutA <--> Cross_NE.photonicInE;
        Line_HorizMidTop.photonicOutB <--> Cross_NE.photonicInW;

        Cross_SE.photonicOutN <--> Line_VertMidRight.photonicInA;
        Cross_SE.photonicOutS <--> Line_VertBottomRight.photonicInB;
        Cross_SE.photonicOutE <--> Line_HorizRightBottom.photonicInB;
        Cross_SE.photonicOutW <--> Line_HorizMidBottom.photonicInA;

        Line_VertMidRight.photonicOutB <--> Cross_SE.photonicInN;
        Line_VertBottomRight.photonicOutA <--> Cross_SE.photonicInS;
        Line_HorizRightBottom.photonicOutA <--> Cross_SE.photonicInE;
        Line_HorizMidBottom.photonicOutB <--> Cross_SE.photonicInW;

        Cross_NW.photonicOutN <--> Line_VertTopLeft.photonicInA;
        Cross_NW.photonicOutS <--> Line_VertMidLeft.photonicInB;
        Cross_NW.photonicOutE <--> Line_HorizMidTop.photonicInB;
        Cross_NW.photonicOutW <--> Line_HorizLeftTop.photonicInA;

        Line_VertTopLeft.photonicOutB <--> Cross_NW.photonicInN;
        Line_VertMidLeft.photonicOutA <--> Cross_NW.photonicInS;
        Line_HorizMidTop.photonicOutA <--> Cross_NW.photonicInE;
        Line_HorizLeftTop.photonicOutB <--> Cross_NW.photonicInW;


        Sw4x4_SW.photonicOutN <--> Shift_N.photonicInA;
        Sw4x4_SW.photonicOutS <--> Shift_S.photonicInB;
        Sw4x4_SW.photonicOutE <--> Shift_E.photonicInB;
        Sw4x4_SW.photonicOutW <--> Shift_W.photonicInA;

        Shift_N.photonicOutB <--> Sw4x4_SW.photonicInN;
        Shift_S.photonicOutA <--> Sw4x4_SW.photonicInS;
        Shift_E.photonicOutA <--> Sw4x4_SW.photonicInE;
        Shift_W.photonicOutB <--> Sw4x4_SW.photonicInW;


        Shift_S.photonicOutB <--> Line_VertBottomLeft.photonicInB;
        Line_VertBottomLeft.photonicOutA <--> Shift_S.photonicInA;
        Shift_E.photonicOutB <--> Line_HorizMidBottom.photonicInB;
        Line_HorizMidBottom.photonicOutA <--> Shift_E.photonicInA;
        Shift_W.photonicOutA <--> Line_HorizLeftBottom.photonicInA;
        Line_HorizLeftBottom.photonicOutB <--> Shift_W.photonicInB;


        Line_VertTopLeft.photonicOutA <--> photonicOutN0;
        photonicInN0 <--> Line_VertTopLeft.photonicInB;
        Line_VertTopRight.photonicOutA <--> photonicOutN1;
        photonicInN1 <--> Line_VertTopRight.photonicInB;

        Line_VertBottomLeft.photonicOutB <--> photonicOutS0;
        photonicInS0 <--> Line_VertBottomLeft.photonicInA;
        Line_VertBottomRight.photonicOutB <--> photonicOutS1;
        photonicInS1 <--> Line_VertBottomRight.photonicInA;

        Line_HorizRightTop.photonicOutB <--> photonicOutE0;
        photonicInE0 <--> Line_HorizRightTop.photonicInA;
        Line_HorizRightBottom.photonicOutB <--> photonicOutE1;
        photonicInE1 <--> Line_HorizRightBottom.photonicInA;

        Line_HorizLeftTop.photonicOutA <--> photonicOutW0;
        photonicInW0 <--> Line_HorizLeftTop.photonicInB;
        Line_HorizLeftBottom.photonicOutA <--> photonicOutW1;
        photonicInW1 <--> Line_HorizLeftBottom.photonicInB;


        GWSwitch.photonicOutInjectionN <--> Line_VertMidLeft.photonicInA;
        GWSwitch.photonicOutInjectionS <--> Shift_N.photonicInB;
        Shift_N.photonicOutA <--> GWSwitch.photonicInEjection;

        GWSwitch.photonicOutGateway <--> GW.phIn[1];
    	GW.phOut[1] <--> GWSwitch.photonicInGateway;
    
    	
    	electronicGatewayIn <--> { @display("ls=,0"); } <--> GW.phIn[0];
    	GW.phOut[0] <--> { @display("ls=,0"); } <--> electronicGatewayOut;
    	
    	electronicControlInProc --> { @display("ls=,0"); } --> GW.elIn[0];
    	electronicControlOutProc <-- { @display("ls=,0"); } <-- GW.elOut[0];
    	
    	
    	photonicMemoryIn <--> { @display("ls=,0"); } <--> GW.phIn[2];
    	GW.phOut[2] <--> { @display("ls=,0"); } <--> photonicMemoryOut;
    
    	electronicControlInMemory --> { @display("ls=,0"); } --> GW.elIn[2];
    
    	electronicControlOutMemory <-- { @display("ls=,0"); } <-- GW.elOut[2];
      
    
    
    
    	GWSwitch.electronicOutGateway --> ElectronicChannel --> GW.elIn[1];
    	GWSwitch.electronicInGateway <-- ElectronicChannel <-- GW.elOut[1];


      

        electronicInN0 --> { @display("ls=red,0"); } --> GWSwitch.electronicInInjectionN;
        GWSwitch.electronicOutInjectionS --> ElectronicChannel { @display("ls=red,0"); } --> Sw4x4_SW.electronicInN;
        Sw4x4_SW.electronicOutS --> { @display("ls=red,0"); } --> electronicOutS0;
        electronicInN1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutS1;
        electronicInS0 --> { @display("ls=red,0"); } --> Sw4x4_SW.electronicInS;
        Sw4x4_SW.electronicOutN --> ElectronicChannel { @display("ls=red,0"); } --> GWSwitch.electronicInInjectionS;
        GWSwitch.electronicOutInjectionN --> electronicOutN0;
        electronicInS1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutN1;
        electronicInE0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutW0;
        electronicInE1 --> { @display("ls=red,0"); } --> Sw4x4_SW.electronicInE;
        Sw4x4_SW.electronicOutW --> { @display("ls=red,0"); } --> electronicOutW1;
        electronicInW0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutE0;
        electronicInW1 --> { @display("ls=red,0"); } --> Sw4x4_SW.electronicInW;
        Sw4x4_SW.electronicOutE --> { @display("ls=red,0"); } --> electronicOutE1;

      
}



//
//
//
//   Filled Nodes, South Side
//
//

module NonblockingTorusTileNE_PopulatedSouth
{
    // Minimum Tile Size = 500um x 500um
    parameters:
        int nodeId;
        int numIF;
        int numX;
        
        bool useIOplane;

        double PathSeparation;
        double coreSizeX;
        double coreSizeY;
        int numOfWavelengthChannels;


        int dispValueX;
        double dispValueY;
        int dispValueZ;
    gates:

        inout electronicGatewayIn;
        inout electronicGatewayOut;

        inout photonicMemoryIn;
        inout photonicMemoryOut;

        inout photonicInN0;
        inout photonicInN1;
        inout photonicInS0;
        inout photonicInS1;
        inout photonicInE0;
        inout photonicInE1;
        inout photonicInW0;
        inout photonicInW1;

        inout photonicOutN0;
        inout photonicOutN1;
        inout photonicOutS0;
        inout photonicOutS1;
        inout photonicOutE0;
        inout photonicOutE1;
        inout photonicOutW0;
        inout photonicOutW1;



        input electronicInN0;
        input electronicInN1;
        input electronicInS0;
        input electronicInS1;
        input electronicInE0;
        input electronicInE1;
        input electronicInW0;
        input electronicInW1;

        output electronicOutN0;
        output electronicOutN1;
        output electronicOutS0;
        output electronicOutS1;
        output electronicOutE0;
        output electronicOutE1;
        output electronicOutW0;
        output electronicOutW1;

      

        input electronicControlInMemory;
        input electronicControlInProc;

        output electronicControlOutMemory;
        output electronicControlOutProc;

      
    submodules:

      GW: HybridRouter {
     
                 parameters:
                id = string(floor(nodeId/numX)*2+((nodeId%numX)<(numX>>1)?0:1)) + ".";
			
					optSwitch = (useIOplane ? "Gateway" : "Gateway_NoMem");
				
				
				elRouter = 1;
				
				level = "NET";
				
				numX = numX;
				numY = 2;

        }
        GWSwitch: NonblockingTorusGatewayRouter {
            parameters:
                nodeId = nodeId;
                numX=numX;
                 type = (numIF > 1) ? 53 : 51;
                @display("p=238,525;b=58,26,,,,1");
        }
        Sw4x4_NE: NonblockingTorusNodeRouter {
            parameters:
                nodeId = nodeId;
                numX=numX;
                 type = (numIF > 1) ? 52 : 50;
                @display("p=197,187;b=174,174,,,,1");
        }
        Cross_NW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=136,200;b=64,64,,,,1");
        }
        Cross_SE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=384,416;b=64,64,,,,1");
        }
        Cross_SW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=152,416;b=64,64,,,,1");
        }
        Shift_N: Shift {
            parameters:
                Shift = 80;
                @display("p=401,85;b=54,4,,#408080,,1");
        }
        Shift_S: Shift {
            parameters:
                Shift = 80;
                @display("p=353,301;b=54,4,,#408080,,1");
        }
        Shift_E: Shift {
            parameters:
                Shift = 80;
                @display("p=481,165;b=4,54,,#408080,,1");
        }
        Shift_W: Shift {
            parameters:
                Shift = 80;
                @display("p=273,221;b=4,54,,#408080,,1");
        }
        Line_VertTopLeft: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,132;b=10,44,,#408080,,1");
        }
        Line_VertTopRight: LineBidirectional {
            parameters:
                Length_Line = 138;
                @display("p=136,132;b=10,44,,#408080,,1");
        }
        Line_VertMidLeft: LineBidirectional {
            parameters:
                Length_Line = 258+(coreSizeY-1034);
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_VertMidRight: LineBidirectional {
            parameters:
                Length_Line = 138+(coreSizeY-1034);
                @display("p=377,340;b=12,44,,#408080,,1");
        }
        Line_VertBottomLeft: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_VertBottomRight: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_HorizRightTop: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=448,424;b=28,4,,#408080,,1");
        }
        Line_HorizRightBottom: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=448,424;b=28,4,,#408080,,1");
        }
        Line_HorizMidTop: LineBidirectional {
            parameters:
                Length_Line = coreSizeX-620;
                @display("p=224,208;b=74,4,,#408080,,1");
        }
        Line_HorizMidBottom: LineBidirectional {
            parameters:
                Length_Line = 120+(coreSizeX-620);
                @display("p=224,208;b=74,4,,#408080,,1");
        }
        Line_HorizLeftTop: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=273,421;b=90,10,,#408080,,1");
        }
        Line_HorizLeftBottom: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=273,421;b=90,10,,#408080,,1");
        }
    connections allowunconnected:
        Cross_NW.photonicOutN <--> Line_VertTopLeft.photonicInA;
        Cross_NW.photonicOutS <--> Line_VertMidLeft.photonicInB;
        Cross_NW.photonicOutE <--> Line_HorizMidTop.photonicInB;
        Cross_NW.photonicOutW <--> Line_HorizLeftTop.photonicInA;

        Line_VertTopLeft.photonicOutB <--> Cross_NW.photonicInN;
        Line_VertMidLeft.photonicOutA <--> Cross_NW.photonicInS;
        Line_HorizMidTop.photonicOutA <--> Cross_NW.photonicInE;
        Line_HorizLeftTop.photonicOutB <--> Cross_NW.photonicInW;

        Cross_SE.photonicOutN <--> Line_VertMidRight.photonicInA;
        Cross_SE.photonicOutS <--> Line_VertBottomRight.photonicInB;
        Cross_SE.photonicOutE <--> Line_HorizRightBottom.photonicInB;
        Cross_SE.photonicOutW <--> Line_HorizMidBottom.photonicInA;

        Line_VertMidRight.photonicOutB <--> Cross_SE.photonicInN;
        Line_VertBottomRight.photonicOutA <--> Cross_SE.photonicInS;
        Line_HorizRightBottom.photonicOutA <--> Cross_SE.photonicInE;
        Line_HorizMidBottom.photonicOutB <--> Cross_SE.photonicInW;

        Cross_SW.photonicOutN <--> Line_VertMidLeft.photonicInA;
        Cross_SW.photonicOutS <--> Line_VertBottomLeft.photonicInB;
        Cross_SW.photonicOutE <--> Line_HorizMidBottom.photonicInB;
        Cross_SW.photonicOutW <--> Line_HorizLeftBottom.photonicInA;

        Line_VertMidLeft.photonicOutB <--> Cross_SW.photonicInN;
        Line_VertBottomLeft.photonicOutA <--> Cross_SW.photonicInS;
        Line_HorizMidBottom.photonicOutA <--> Cross_SW.photonicInE;
        Line_HorizLeftBottom.photonicOutB <--> Cross_SW.photonicInW;


        Sw4x4_NE.photonicOutN <--> Shift_N.photonicInA;
        Sw4x4_NE.photonicOutS <--> Shift_S.photonicInB;
        Sw4x4_NE.photonicOutE <--> Shift_E.photonicInB;
        Sw4x4_NE.photonicOutW <--> Shift_W.photonicInA;

        Shift_N.photonicOutB <--> Sw4x4_NE.photonicInN;
        Shift_S.photonicOutA <--> Sw4x4_NE.photonicInS;
        Shift_E.photonicOutA <--> Sw4x4_NE.photonicInE;
        Shift_W.photonicOutB <--> Sw4x4_NE.photonicInW;


        Shift_N.photonicOutA <--> Line_VertTopRight.photonicInA;
        Line_VertTopRight.photonicOutB <--> Shift_N.photonicInB;
        Shift_E.photonicOutB <--> Line_HorizRightTop.photonicInB;
        Line_HorizRightTop.photonicOutA <--> Shift_E.photonicInA;
        Shift_W.photonicOutA <--> Line_HorizMidTop.photonicInA;
        Line_HorizMidTop.photonicOutB <--> Shift_W.photonicInB;


        Line_VertTopLeft.photonicOutA <--> photonicOutN0;
        photonicInN0 <--> Line_VertTopLeft.photonicInB;
        Line_VertTopRight.photonicOutA <--> photonicOutN1;
        photonicInN1 <--> Line_VertTopRight.photonicInB;

        Line_VertBottomLeft.photonicOutB <--> photonicOutS0;
        photonicInS0 <--> Line_VertBottomLeft.photonicInA;
        Line_VertBottomRight.photonicOutB <--> photonicOutS1;
        photonicInS1 <--> Line_VertBottomRight.photonicInA;

        Line_HorizRightTop.photonicOutB <--> photonicOutE0;
        photonicInE0 <--> Line_HorizRightTop.photonicInA;
        Line_HorizRightBottom.photonicOutB <--> photonicOutE1;
        photonicInE1 <--> Line_HorizRightBottom.photonicInA;

        Line_HorizLeftTop.photonicOutA <--> photonicOutW0;
        photonicInW0 <--> Line_HorizLeftTop.photonicInB;
        Line_HorizLeftBottom.photonicOutA <--> photonicOutW1;
        photonicInW1 <--> Line_HorizLeftBottom.photonicInB;


        GWSwitch.photonicOutInjectionN <--> Shift_S.photonicInA;
        GWSwitch.photonicOutInjectionS <--> Line_VertMidRight.photonicInB;
        Shift_S.photonicOutB <--> GWSwitch.photonicInEjection;

        GWSwitch.photonicOutGateway <--> GW.phIn[1];
    	GW.phOut[1] <--> GWSwitch.photonicInGateway;
    
    	
    	electronicGatewayIn <--> { @display("ls=,0"); } <--> GW.phIn[0];
    	GW.phOut[0] <--> { @display("ls=,0"); } <--> electronicGatewayOut;
    	
    	electronicControlInProc --> { @display("ls=,0"); } --> GW.elIn[0];
    	electronicControlOutProc <-- { @display("ls=,0"); } <-- GW.elOut[0];
    	
    	
    	photonicMemoryIn <--> { @display("ls=,0"); } <--> GW.phIn[2];
    	GW.phOut[2] <--> { @display("ls=,0"); } <--> photonicMemoryOut;
    
    	electronicControlInMemory --> { @display("ls=,0"); } --> GW.elIn[2];
    
    	electronicControlOutMemory <-- { @display("ls=,0"); } <-- GW.elOut[2];
      
    
    	GWSwitch.electronicOutGateway --> ElectronicChannel --> GW.elIn[1];
    	GWSwitch.electronicInGateway <-- ElectronicChannel <-- GW.elOut[1];


       

        electronicInN0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutS0;
        electronicInN1 --> { @display("ls=red,0"); } --> Sw4x4_NE.electronicInN;
        Sw4x4_NE.electronicOutS --> ElectronicChannel { @display("ls=red,0"); } --> GWSwitch.electronicInInjectionN;
        GWSwitch.electronicOutInjectionS --> { @display("ls=red,0"); } --> electronicOutS1;
        electronicInS0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutN0;
        electronicInS1 --> { @display("ls=red,0"); } --> GWSwitch.electronicInInjectionS;
        GWSwitch.electronicOutInjectionN --> ElectronicChannel { @display("ls=red,0"); } --> Sw4x4_NE.electronicInS;
        Sw4x4_NE.electronicOutN --> electronicOutN1;
        electronicInE0 --> { @display("ls=red,0"); } --> Sw4x4_NE.electronicInE;
        Sw4x4_NE.electronicOutW --> { @display("ls=red,0"); } --> electronicOutW0;
        electronicInE1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutW1;
        electronicInW0 --> { @display("ls=red,0"); } --> Sw4x4_NE.electronicInW;
        Sw4x4_NE.electronicOutE --> { @display("ls=red,0"); } --> electronicOutE0;
        electronicInW1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutE1;

       
}





module NonblockingTorusTileNW_PopulatedSouth
{
    // Minimum Tile Size = 500um x 500um
    parameters:
        int nodeId;
        int numIF;
        int numX;
        
        bool useIOplane;

        double PathSeparation;
        double coreSizeX;
        double coreSizeY;
        int numOfWavelengthChannels;


        int dispValueX;
        double dispValueY;
        int dispValueZ;
    gates:

        inout electronicGatewayIn;
        inout electronicGatewayOut;

        inout photonicMemoryIn;
        inout photonicMemoryOut;

        inout photonicInN0;
        inout photonicInN1;
        inout photonicInS0;
        inout photonicInS1;
        inout photonicInE0;
        inout photonicInE1;
        inout photonicInW0;
        inout photonicInW1;

        inout photonicOutN0;
        inout photonicOutN1;
        inout photonicOutS0;
        inout photonicOutS1;
        inout photonicOutE0;
        inout photonicOutE1;
        inout photonicOutW0;
        inout photonicOutW1;



        input electronicInN0;
        input electronicInN1;
        input electronicInS0;
        input electronicInS1;
        input electronicInE0;
        input electronicInE1;
        input electronicInW0;
        input electronicInW1;

        output electronicOutN0;
        output electronicOutN1;
        output electronicOutS0;
        output electronicOutS1;
        output electronicOutE0;
        output electronicOutE1;
        output electronicOutW0;
        output electronicOutW1;

     

        input electronicControlInMemory;
        input electronicControlInProc;

        output electronicControlOutMemory;
        output electronicControlOutProc;

     
    submodules:

       GW: HybridRouter {
     
                 parameters:
                id = string(floor(nodeId/numX)*2+((nodeId%numX)<(numX>>1)?0:1)) + ".";
			
					optSwitch = (useIOplane ? "Gateway" : "Gateway_NoMem");
			
				
				elRouter = 1;
				
				level = "NET";
				
				numX = numX;
				numY = 2;

        }
        GWSwitch: NonblockingTorusGatewayRouter {
            parameters:
                nodeId = nodeId;
                numX=numX;
                 type = (numIF > 1) ? 53 : 51;
                @display("p=238,525;b=58,26,,,,1");
        }
        Sw4x4_NW: NonblockingTorusNodeRouter {
            parameters:
                nodeId = nodeId;
                numX=numX;
                 type = (numIF > 1) ? 52 : 50;
                @display("p=197,187;b=174,174,,,,1");
        }
        Cross_NE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=136,200;b=64,64,,,,1");
        }
        Cross_SE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=384,416;b=64,64,,,,1");
        }
        Cross_SW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=152,416;b=64,64,,,,1");
        }
        Shift_N: Shift {
            parameters:
                Shift = 80;
                @display("p=401,85;b=54,4,,#408080,,1");
        }
        Shift_S: Shift {
            parameters:
                Shift = 80;
                @display("p=353,301;b=54,4,,#408080,,1");
        }
        Shift_E: Shift {
            parameters:
                Shift = 80;
                @display("p=481,165;b=4,54,,#408080,,1");
        }
        Shift_W: Shift {
            parameters:
                Shift = 80;
                @display("p=273,221;b=4,54,,#408080,,1");
        }
        Line_VertTopLeft: LineBidirectional {
            parameters:
                Length_Line = 138;
                @display("p=136,132;b=10,44,,#408080,,1");
        }
        Line_VertTopRight: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,132;b=10,44,,#408080,,1");
        }
        Line_VertMidLeft: LineBidirectional {
            parameters:
                Length_Line = 138+(coreSizeY-1034);
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_VertMidRight: LineBidirectional {
            parameters:
                Length_Line = 258+(coreSizeY-1034);
                @display("p=377,340;b=12,44,,#408080,,1");
        }
        Line_VertBottomLeft: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_VertBottomRight: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_HorizRightTop: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=448,424;b=28,4,,#408080,,1");
        }
        Line_HorizRightBottom: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=448,424;b=28,4,,#408080,,1");
        }
        Line_HorizMidTop: LineBidirectional {
            parameters:
                Length_Line = coreSizeX-620;
                @display("p=224,208;b=74,4,,#408080,,1");
        }
        Line_HorizMidBottom: LineBidirectional {
            parameters:
                Length_Line = 120+(coreSizeX-620);
                @display("p=224,208;b=74,4,,#408080,,1");
        }
        Line_HorizLeftTop: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=273,421;b=90,10,,#408080,,1");
        }
        Line_HorizLeftBottom: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=273,421;b=90,10,,#408080,,1");
        }
    connections allowunconnected:
        Cross_NE.photonicOutN <--> Line_VertTopRight.photonicInA;
        Cross_NE.photonicOutS <--> Line_VertMidRight.photonicInB;
        Cross_NE.photonicOutE <--> Line_HorizRightTop.photonicInB;
        Cross_NE.photonicOutW <--> Line_HorizMidTop.photonicInA;

        Line_VertTopRight.photonicOutB <--> Cross_NE.photonicInN;
        Line_VertMidRight.photonicOutA <--> Cross_NE.photonicInS;
        Line_HorizRightTop.photonicOutA <--> Cross_NE.photonicInE;
        Line_HorizMidTop.photonicOutB <--> Cross_NE.photonicInW;

        Cross_SE.photonicOutN <--> Line_VertMidRight.photonicInA;
        Cross_SE.photonicOutS <--> Line_VertBottomRight.photonicInB;
        Cross_SE.photonicOutE <--> Line_HorizRightBottom.photonicInB;
        Cross_SE.photonicOutW <--> Line_HorizMidBottom.photonicInA;

        Line_VertMidRight.photonicOutB <--> Cross_SE.photonicInN;
        Line_VertBottomRight.photonicOutA <--> Cross_SE.photonicInS;
        Line_HorizRightBottom.photonicOutA <--> Cross_SE.photonicInE;
        Line_HorizMidBottom.photonicOutB <--> Cross_SE.photonicInW;

        Cross_SW.photonicOutN <--> Line_VertMidLeft.photonicInA;
        Cross_SW.photonicOutS <--> Line_VertBottomLeft.photonicInB;
        Cross_SW.photonicOutE <--> Line_HorizMidBottom.photonicInB;
        Cross_SW.photonicOutW <--> Line_HorizLeftBottom.photonicInA;

        Line_VertMidLeft.photonicOutB <--> Cross_SW.photonicInN;
        Line_VertBottomLeft.photonicOutA <--> Cross_SW.photonicInS;
        Line_HorizMidBottom.photonicOutA <--> Cross_SW.photonicInE;
        Line_HorizLeftBottom.photonicOutB <--> Cross_SW.photonicInW;


        Sw4x4_NW.photonicOutN <--> Shift_N.photonicInA;
        Sw4x4_NW.photonicOutS <--> Shift_S.photonicInB;
        Sw4x4_NW.photonicOutE <--> Shift_E.photonicInB;
        Sw4x4_NW.photonicOutW <--> Shift_W.photonicInA;

        Shift_N.photonicOutB <--> Sw4x4_NW.photonicInN;
        Shift_S.photonicOutA <--> Sw4x4_NW.photonicInS;
        Shift_E.photonicOutA <--> Sw4x4_NW.photonicInE;
        Shift_W.photonicOutB <--> Sw4x4_NW.photonicInW;


        Shift_N.photonicOutA <--> Line_VertTopLeft.photonicInA;
        Line_VertTopLeft.photonicOutB <--> Shift_N.photonicInB;
        Shift_E.photonicOutB <--> Line_HorizMidTop.photonicInB;
        Line_HorizMidTop.photonicOutA <--> Shift_E.photonicInA;
        Shift_W.photonicOutA <--> Line_HorizLeftTop.photonicInA;
        Line_HorizLeftTop.photonicOutB <--> Shift_W.photonicInB;


        Line_VertTopLeft.photonicOutA <--> photonicOutN0;
        photonicInN0 <--> Line_VertTopLeft.photonicInB;
        Line_VertTopRight.photonicOutA <--> photonicOutN1;
        photonicInN1 <--> Line_VertTopRight.photonicInB;

        Line_VertBottomLeft.photonicOutB <--> photonicOutS0;
        photonicInS0 <--> Line_VertBottomLeft.photonicInA;
        Line_VertBottomRight.photonicOutB <--> photonicOutS1;
        photonicInS1 <--> Line_VertBottomRight.photonicInA;

        Line_HorizRightTop.photonicOutB <--> photonicOutE0;
        photonicInE0 <--> Line_HorizRightTop.photonicInA;
        Line_HorizRightBottom.photonicOutB <--> photonicOutE1;
        photonicInE1 <--> Line_HorizRightBottom.photonicInA;

        Line_HorizLeftTop.photonicOutA <--> photonicOutW0;
        photonicInW0 <--> Line_HorizLeftTop.photonicInB;
        Line_HorizLeftBottom.photonicOutA <--> photonicOutW1;
        photonicInW1 <--> Line_HorizLeftBottom.photonicInB;


        GWSwitch.photonicOutInjectionN <--> Shift_S.photonicInA;
        GWSwitch.photonicOutInjectionS <--> Line_VertMidLeft.photonicInB;
        Shift_S.photonicOutB <--> GWSwitch.photonicInEjection;

        GWSwitch.photonicOutGateway <--> GW.phIn[1];
    	GW.phOut[1] <--> GWSwitch.photonicInGateway;
    
    	
    	electronicGatewayIn <--> { @display("ls=,0"); } <--> GW.phIn[0];
    	GW.phOut[0] <--> { @display("ls=,0"); } <--> electronicGatewayOut;
    	
    	electronicControlInProc --> { @display("ls=,0"); } --> GW.elIn[0];
    	electronicControlOutProc <-- { @display("ls=,0"); } <-- GW.elOut[0];
    	
    	
    	photonicMemoryIn <--> { @display("ls=,0"); } <--> GW.phIn[2];
    	GW.phOut[2] <--> { @display("ls=,0"); } <--> photonicMemoryOut;
    
    	electronicControlInMemory --> { @display("ls=,0"); } --> GW.elIn[2];
    
    	electronicControlOutMemory <-- { @display("ls=,0"); } <-- GW.elOut[2];
      
    
    
    
    	GWSwitch.electronicOutGateway --> ElectronicChannel --> GW.elIn[1];
    	GWSwitch.electronicInGateway <-- ElectronicChannel <-- GW.elOut[1];

    

        electronicInN0 --> { @display("ls=red,0"); } --> Sw4x4_NW.electronicInN;
        Sw4x4_NW.electronicOutS --> ElectronicChannel { @display("ls=red,0"); } --> GWSwitch.electronicInInjectionN;
        GWSwitch.electronicOutInjectionS --> { @display("ls=red,0"); } --> electronicOutS0;
        electronicInN1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutS1;
        electronicInS0 --> { @display("ls=red,0"); } --> GWSwitch.electronicInInjectionS;
        GWSwitch.electronicOutInjectionN --> ElectronicChannel { @display("ls=red,0"); } --> Sw4x4_NW.electronicInS;
        Sw4x4_NW.electronicOutN --> electronicOutN0;
        electronicInS1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutN1;
        electronicInE0 --> { @display("ls=red,0"); } --> Sw4x4_NW.electronicInE;
        Sw4x4_NW.electronicOutW --> { @display("ls=red,0"); } --> electronicOutW0;
        electronicInE1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutW1;
        electronicInW0 --> { @display("ls=red,0"); } --> Sw4x4_NW.electronicInW;
        Sw4x4_NW.electronicOutE --> { @display("ls=red,0"); } --> electronicOutE0;
        electronicInW1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutE1;

       
}







module NonblockingTorusTileSE_PopulatedSouth
{
    // Minimum Tile Size = 500um x 500um
    parameters:
        int nodeId;
        int numIF;
        int numX;
        
        bool useIOplane;

        double PathSeparation;
        double coreSizeX;
        double coreSizeY;
        int numOfWavelengthChannels;

        int dispValueX;
        double dispValueY;
        int dispValueZ;
    gates:

        inout electronicGatewayIn;
        inout electronicGatewayOut;

        inout photonicMemoryIn;
        inout photonicMemoryOut;

        inout photonicInN0;
        inout photonicInN1;
        inout photonicInS0;
        inout photonicInS1;
        inout photonicInE0;
        inout photonicInE1;
        inout photonicInW0;
        inout photonicInW1;

        inout photonicOutN0;
        inout photonicOutN1;
        inout photonicOutS0;
        inout photonicOutS1;
        inout photonicOutE0;
        inout photonicOutE1;
        inout photonicOutW0;
        inout photonicOutW1;



        input electronicInN0;
        input electronicInN1;
        input electronicInS0;
        input electronicInS1;
        input electronicInE0;
        input electronicInE1;
        input electronicInW0;
        input electronicInW1;

        output electronicOutN0;
        output electronicOutN1;
        output electronicOutS0;
        output electronicOutS1;
        output electronicOutE0;
        output electronicOutE1;
        output electronicOutW0;
        output electronicOutW1;


        input electronicControlInMemory;
        input electronicControlInProc;

        output electronicControlOutMemory;
        output electronicControlOutProc;

    submodules:

      
		GW: HybridRouter {
     
                 parameters:
                id = string(floor(nodeId/numX)*2+((nodeId%numX)<(numX>>1)?0:1)) + ".";
			
					optSwitch = (useIOplane ? "Gateway" : "Gateway_NoMem");
			
				
				elRouter = 1;
				
				level = "NET";
				
				numX = numX;
				numY = 2;

        }
        GWSwitch: NonblockingTorusGatewayRouter {
            parameters:
                nodeId = nodeId;
                numX=numX;
                 type = (numIF > 1) ? 53 : 51;
                @display("p=238,525;b=58,26,,,,1");
        }
        Sw4x4_SE: NonblockingTorusNodeRouter {
            parameters:
                nodeId = nodeId;
                numX=numX;
                 type = (numIF > 1) ? 52 : 50;
                @display("p=197,187;b=174,174,,,,1");
        }
        Cross_NW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=136,200;b=64,64,,,,1");
        }
        Cross_NE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=384,416;b=64,64,,,,1");
        }
        Cross_SW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=152,416;b=64,64,,,,1");
        }
        Shift_N: Shift {
            parameters:
                Shift = 80;
                @display("p=401,85;b=54,4,,#408080,,1");
        }
        Shift_S: Shift {
            parameters:
                Shift = 80;
                @display("p=353,301;b=54,4,,#408080,,1");
        }
        Shift_E: Shift {
            parameters:
                Shift = 80;
                @display("p=481,165;b=4,54,,#408080,,1");
        }
        Shift_W: Shift {
            parameters:
                Shift = 80;
                @display("p=273,221;b=4,54,,#408080,,1");
        }
        Line_VertTopLeft: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,132;b=10,44,,#408080,,1");
        }
        Line_VertTopRight: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,132;b=10,44,,#408080,,1");
        }
        Line_VertMidLeft: LineBidirectional {
            parameters:
                Length_Line = 258+(coreSizeY-1034);
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_VertMidRight: LineBidirectional {
            parameters:
                Length_Line = 138+(coreSizeY-1034);
                @display("p=377,340;b=12,44,,#408080,,1");
        }
        Line_VertBottomLeft: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_VertBottomRight: LineBidirectional {
            parameters:
                Length_Line = 138;
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_HorizRightTop: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=448,424;b=28,4,,#408080,,1");
        }
        Line_HorizRightBottom: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=448,424;b=28,4,,#408080,,1");
        }
        Line_HorizMidTop: LineBidirectional {
            parameters:
                Length_Line = 120+(coreSizeX-620);
                @display("p=224,208;b=74,4,,#408080,,1");
        }
        Line_HorizMidBottom: LineBidirectional {
            parameters:
                Length_Line = coreSizeX-620;
                @display("p=224,208;b=74,4,,#408080,,1");
        }
        Line_HorizLeftTop: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=273,421;b=90,10,,#408080,,1");
        }
        Line_HorizLeftBottom: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=273,421;b=90,10,,#408080,,1");
        }
    connections allowunconnected:
        Cross_NW.photonicOutN <--> Line_VertTopLeft.photonicInA;
        Cross_NW.photonicOutS <--> Line_VertMidLeft.photonicInB;
        Cross_NW.photonicOutE <--> Line_HorizMidTop.photonicInB;
        Cross_NW.photonicOutW <--> Line_HorizLeftTop.photonicInA;

        Line_VertTopLeft.photonicOutB <--> Cross_NW.photonicInN;
        Line_VertMidLeft.photonicOutA <--> Cross_NW.photonicInS;
        Line_HorizMidTop.photonicOutA <--> Cross_NW.photonicInE;
        Line_HorizLeftTop.photonicOutB <--> Cross_NW.photonicInW;

        Cross_NE.photonicOutN <--> Line_VertTopRight.photonicInA;
        Cross_NE.photonicOutS <--> Line_VertMidRight.photonicInB;
        Cross_NE.photonicOutE <--> Line_HorizRightTop.photonicInB;
        Cross_NE.photonicOutW <--> Line_HorizMidTop.photonicInA;

        Line_VertTopRight.photonicOutB <--> Cross_NE.photonicInN;
        Line_VertMidRight.photonicOutA <--> Cross_NE.photonicInS;
        Line_HorizRightTop.photonicOutA <--> Cross_NE.photonicInE;
        Line_HorizMidTop.photonicOutB <--> Cross_NE.photonicInW;

        Cross_SW.photonicOutN <--> Line_VertMidLeft.photonicInA;
        Cross_SW.photonicOutS <--> Line_VertBottomLeft.photonicInB;
        Cross_SW.photonicOutE <--> Line_HorizMidBottom.photonicInB;
        Cross_SW.photonicOutW <--> Line_HorizLeftBottom.photonicInA;

        Line_VertMidLeft.photonicOutB <--> Cross_SW.photonicInN;
        Line_VertBottomLeft.photonicOutA <--> Cross_SW.photonicInS;
        Line_HorizMidBottom.photonicOutA <--> Cross_SW.photonicInE;
        Line_HorizLeftBottom.photonicOutB <--> Cross_SW.photonicInW;


        Sw4x4_SE.photonicOutN <--> Shift_N.photonicInA;
        Sw4x4_SE.photonicOutS <--> Shift_S.photonicInB;
        Sw4x4_SE.photonicOutE <--> Shift_E.photonicInB;
        Sw4x4_SE.photonicOutW <--> Shift_W.photonicInA;

        Shift_N.photonicOutB <--> Sw4x4_SE.photonicInN;
        Shift_S.photonicOutA <--> Sw4x4_SE.photonicInS;
        Shift_E.photonicOutA <--> Sw4x4_SE.photonicInE;
        Shift_W.photonicOutB <--> Sw4x4_SE.photonicInW;


        Shift_N.photonicOutA <--> Line_VertMidRight.photonicInA;
        Line_VertMidRight.photonicOutB <--> Shift_N.photonicInB;
        Shift_E.photonicOutB <--> Line_HorizRightBottom.photonicInB;
        Line_HorizRightBottom.photonicOutA <--> Shift_E.photonicInA;
        Shift_W.photonicOutA <--> Line_HorizMidBottom.photonicInA;
        Line_HorizMidBottom.photonicOutB <--> Shift_W.photonicInB;


        Line_VertTopLeft.photonicOutA <--> photonicOutN0;
        photonicInN0 <--> Line_VertTopLeft.photonicInB;
        Line_VertTopRight.photonicOutA <--> photonicOutN1;
        photonicInN1 <--> Line_VertTopRight.photonicInB;

        Line_VertBottomLeft.photonicOutB <--> photonicOutS0;
        photonicInS0 <--> Line_VertBottomLeft.photonicInA;
        Line_VertBottomRight.photonicOutB <--> photonicOutS1;
        photonicInS1 <--> Line_VertBottomRight.photonicInA;

        Line_HorizRightTop.photonicOutB <--> photonicOutE0;
        photonicInE0 <--> Line_HorizRightTop.photonicInA;
        Line_HorizRightBottom.photonicOutB <--> photonicOutE1;
        photonicInE1 <--> Line_HorizRightBottom.photonicInA;

        Line_HorizLeftTop.photonicOutA <--> photonicOutW0;
        photonicInW0 <--> Line_HorizLeftTop.photonicInB;
        Line_HorizLeftBottom.photonicOutA <--> photonicOutW1;
        photonicInW1 <--> Line_HorizLeftBottom.photonicInB;


        GWSwitch.photonicOutInjectionN <--> Shift_S.photonicInA;
        GWSwitch.photonicOutInjectionS <--> Line_VertBottomRight.photonicInB;
        Shift_S.photonicOutB <--> GWSwitch.photonicInEjection;

       GWSwitch.photonicOutGateway <--> GW.phIn[1];
	GW.phOut[1] <--> GWSwitch.photonicInGateway;

	
	electronicGatewayIn <--> { @display("ls=,0"); } <--> GW.phIn[0];
	GW.phOut[0] <--> { @display("ls=,0"); } <--> electronicGatewayOut;
	
	electronicControlInProc --> { @display("ls=,0"); } --> GW.elIn[0];
	electronicControlOutProc <-- { @display("ls=,0"); } <-- GW.elOut[0];
	
	
	photonicMemoryIn <--> { @display("ls=,0"); } <--> GW.phIn[2];
	GW.phOut[2] <--> { @display("ls=,0"); } <--> photonicMemoryOut;

	electronicControlInMemory --> { @display("ls=,0"); } --> GW.elIn[2];

	electronicControlOutMemory <-- { @display("ls=,0"); } <-- GW.elOut[2];
  



	GWSwitch.electronicOutGateway --> ElectronicChannel --> GW.elIn[1];
	GWSwitch.electronicInGateway <-- ElectronicChannel <-- GW.elOut[1];




        electronicInN0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutS0;
        electronicInN1 --> { @display("ls=red,0"); } --> Sw4x4_SE.electronicInN;
        Sw4x4_SE.electronicOutS --> ElectronicChannel { @display("ls=red,0"); } --> GWSwitch.electronicInInjectionN;
        GWSwitch.electronicOutInjectionS --> { @display("ls=red,0"); } --> electronicOutS1;
        electronicInS0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutN0;
        electronicInS1 --> { @display("ls=red,0"); } --> GWSwitch.electronicInInjectionS;
        GWSwitch.electronicOutInjectionN --> ElectronicChannel { @display("ls=red,0"); } --> Sw4x4_SE.electronicInS;
        Sw4x4_SE.electronicOutN --> electronicOutN1;
        electronicInE0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutW0;
        electronicInE1 --> { @display("ls=red,0"); } --> Sw4x4_SE.electronicInE;
        Sw4x4_SE.electronicOutW --> { @display("ls=red,0"); } --> electronicOutW1;
        electronicInW0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutE0;
        electronicInW1 --> { @display("ls=red,0"); } --> Sw4x4_SE.electronicInW;
        Sw4x4_SE.electronicOutE --> { @display("ls=red,0"); } --> electronicOutE1;


      
}





module NonblockingTorusTileSW_PopulatedSouth
{
    parameters:
        int nodeId;
        int numIF;
int numX;

bool useIOplane;

        double PathSeparation;
        double coreSizeX;
        double coreSizeY;
        int numOfWavelengthChannels;

        int dispValueX;
        double dispValueY;
        int dispValueZ;
    gates:

        inout electronicGatewayIn;
        inout electronicGatewayOut;

        inout photonicMemoryIn;
        inout photonicMemoryOut;

        inout photonicInN0;
        inout photonicInN1;
        inout photonicInS0;
        inout photonicInS1;
        inout photonicInE0;
        inout photonicInE1;
        inout photonicInW0;
        inout photonicInW1;

        inout photonicOutN0;
        inout photonicOutN1;
        inout photonicOutS0;
        inout photonicOutS1;
        inout photonicOutE0;
        inout photonicOutE1;
        inout photonicOutW0;
        inout photonicOutW1;


        input electronicInN0;
        input electronicInN1;
        input electronicInS0;
        input electronicInS1;
        input electronicInE0;
        input electronicInE1;
        input electronicInW0;
        input electronicInW1;

        output electronicOutN0;
        output electronicOutN1;
        output electronicOutS0;
        output electronicOutS1;
        output electronicOutE0;
        output electronicOutE1;
        output electronicOutW0;
        output electronicOutW1;

      

        input electronicControlInMemory;
        input electronicControlInProc;

        output electronicControlOutMemory;
        output electronicControlOutProc;

      
    submodules:

        GW: HybridRouter {
     
                 parameters:
                id = string(floor(nodeId/numX)*2+((nodeId%numX)<(numX>>1)?0:1)) + ".";
			
					optSwitch = (useIOplane ? "Gateway" : "Gateway_NoMem");
			
				elRouter = 1;
				
				level = "NET";
				
				numX = numX;
				numY = 2;

        }
        GWSwitch: NonblockingTorusGatewayRouter {
            parameters:
                nodeId = nodeId;
                numX=numX;
                 type = (numIF > 1) ? 53 : 51;
                @display("p=238,525;b=58,26,,,,1");
        }
        Sw4x4_SW: NonblockingTorusNodeRouter {
            parameters:
                nodeId = nodeId;
                numX=numX;
                 type = (numIF > 1) ? 52 : 50;
                @display("p=197,187;b=174,174,,,,1");
        }
        Cross_NE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=136,200;b=64,64,,,,1");
        }
        Cross_SE: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=384,416;b=64,64,,,,1");
        }
        Cross_NW: CrossBidirectional {
            parameters:
                PathSeparationX = PathSeparation;
                PathSeparationY = PathSeparation;
                @display("p=152,416;b=64,64,,,,1");
        }
        Shift_N: Shift {
            parameters:
                Shift = 80;
                @display("p=401,85;b=54,4,,#408080,,1");
        }
        Shift_S: Shift {
            parameters:
                Shift = 80;
                @display("p=353,301;b=54,4,,#408080,,1");
        }
        Shift_E: Shift {
            parameters:
                Shift = 80;
                @display("p=481,165;b=4,54,,#408080,,1");
        }
        Shift_W: Shift {
            parameters:
                Shift = 80;
                @display("p=273,221;b=4,54,,#408080,,1");
        }
        Line_VertTopLeft: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,132;b=10,44,,#408080,,1");
        }
        Line_VertTopRight: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,132;b=10,44,,#408080,,1");
        }
        Line_VertMidLeft: LineBidirectional {
            parameters:
                Length_Line = 138+(coreSizeY-1034);
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_VertMidRight: LineBidirectional {
            parameters:
                Length_Line = 258+(coreSizeY-1034);
                @display("p=377,340;b=12,44,,#408080,,1");
        }
        Line_VertBottomLeft: LineBidirectional {
            parameters:
                Length_Line = 138;
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_VertBottomRight: LineBidirectional {
            parameters:
                Length_Line = 258;
                @display("p=136,305;b=12,66,,#408080,,1");
        }
        Line_HorizRightTop: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=448,424;b=28,4,,#408080,,1");
        }
        Line_HorizRightBottom: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=448,424;b=28,4,,#408080,,1");
        }
        Line_HorizMidTop: LineBidirectional {
            parameters:
                Length_Line = 120+(coreSizeX-620);
                @display("p=224,208;b=74,4,,#408080,,1");
        }
        Line_HorizMidBottom: LineBidirectional {
            parameters:
                Length_Line = coreSizeX-620;
                @display("p=224,208;b=74,4,,#408080,,1");
        }
        Line_HorizLeftTop: LineBidirectional {
            parameters:
                Length_Line = 120;
                @display("p=273,421;b=90,10,,#408080,,1");
        }
        Line_HorizLeftBottom: LineBidirectional {
            parameters:
                Length_Line = 0;
                @display("p=273,421;b=90,10,,#408080,,1");
        }
    connections allowunconnected:
        Cross_NE.photonicOutN <--> Line_VertTopRight.photonicInA;
        Cross_NE.photonicOutS <--> Line_VertMidRight.photonicInB;
        Cross_NE.photonicOutE <--> Line_HorizRightTop.photonicInB;
        Cross_NE.photonicOutW <--> Line_HorizMidTop.photonicInA;

        Line_VertTopRight.photonicOutB <--> Cross_NE.photonicInN;
        Line_VertMidRight.photonicOutA <--> Cross_NE.photonicInS;
        Line_HorizRightTop.photonicOutA <--> Cross_NE.photonicInE;
        Line_HorizMidTop.photonicOutB <--> Cross_NE.photonicInW;

        Cross_SE.photonicOutN <--> Line_VertMidRight.photonicInA;
        Cross_SE.photonicOutS <--> Line_VertBottomRight.photonicInB;
        Cross_SE.photonicOutE <--> Line_HorizRightBottom.photonicInB;
        Cross_SE.photonicOutW <--> Line_HorizMidBottom.photonicInA;

        Line_VertMidRight.photonicOutB <--> Cross_SE.photonicInN;
        Line_VertBottomRight.photonicOutA <--> Cross_SE.photonicInS;
        Line_HorizRightBottom.photonicOutA <--> Cross_SE.photonicInE;
        Line_HorizMidBottom.photonicOutB <--> Cross_SE.photonicInW;

        Cross_NW.photonicOutN <--> Line_VertTopLeft.photonicInA;
        Cross_NW.photonicOutS <--> Line_VertMidLeft.photonicInB;
        Cross_NW.photonicOutE <--> Line_HorizMidTop.photonicInB;
        Cross_NW.photonicOutW <--> Line_HorizLeftTop.photonicInA;

        Line_VertTopLeft.photonicOutB <--> Cross_NW.photonicInN;
        Line_VertMidLeft.photonicOutA <--> Cross_NW.photonicInS;
        Line_HorizMidTop.photonicOutA <--> Cross_NW.photonicInE;
        Line_HorizLeftTop.photonicOutB <--> Cross_NW.photonicInW;


        Sw4x4_SW.photonicOutN <--> Shift_N.photonicInA;
        Sw4x4_SW.photonicOutS <--> Shift_S.photonicInB;
        Sw4x4_SW.photonicOutE <--> Shift_E.photonicInB;
        Sw4x4_SW.photonicOutW <--> Shift_W.photonicInA;

        Shift_N.photonicOutB <--> Sw4x4_SW.photonicInN;
        Shift_S.photonicOutA <--> Sw4x4_SW.photonicInS;
        Shift_E.photonicOutA <--> Sw4x4_SW.photonicInE;
        Shift_W.photonicOutB <--> Sw4x4_SW.photonicInW;


        Shift_N.photonicOutA <--> Line_VertMidLeft.photonicInA;
        Line_VertMidLeft.photonicOutB <--> Shift_N.photonicInB;
        Shift_E.photonicOutB <--> Line_HorizMidBottom.photonicInB;
        Line_HorizMidBottom.photonicOutA <--> Shift_E.photonicInA;
        Shift_W.photonicOutA <--> Line_HorizLeftBottom.photonicInA;
        Line_HorizLeftBottom.photonicOutB <--> Shift_W.photonicInB;


        Line_VertTopLeft.photonicOutA <--> photonicOutN0;
        photonicInN0 <--> Line_VertTopLeft.photonicInB;
        Line_VertTopRight.photonicOutA <--> photonicOutN1;
        photonicInN1 <--> Line_VertTopRight.photonicInB;

        Line_VertBottomLeft.photonicOutB <--> photonicOutS0;
        photonicInS0 <--> Line_VertBottomLeft.photonicInA;
        Line_VertBottomRight.photonicOutB <--> photonicOutS1;
        photonicInS1 <--> Line_VertBottomRight.photonicInA;

        Line_HorizRightTop.photonicOutB <--> photonicOutE0;
        photonicInE0 <--> Line_HorizRightTop.photonicInA;
        Line_HorizRightBottom.photonicOutB <--> photonicOutE1;
        photonicInE1 <--> Line_HorizRightBottom.photonicInA;

        Line_HorizLeftTop.photonicOutA <--> photonicOutW0;
        photonicInW0 <--> Line_HorizLeftTop.photonicInB;
        Line_HorizLeftBottom.photonicOutA <--> photonicOutW1;
        photonicInW1 <--> Line_HorizLeftBottom.photonicInB;

        GWSwitch.photonicOutInjectionN <--> Shift_S.photonicInA;
        GWSwitch.photonicOutInjectionS <--> Line_VertBottomLeft.photonicInB;
        Shift_S.photonicOutB <--> GWSwitch.photonicInEjection;

        GWSwitch.photonicOutGateway <--> GW.phIn[1];
    	GW.phOut[1] <--> GWSwitch.photonicInGateway;
    
    	
    	electronicGatewayIn <--> { @display("ls=,0"); } <--> GW.phIn[0];
    	GW.phOut[0] <--> { @display("ls=,0"); } <--> electronicGatewayOut;
    	
    	electronicControlInProc --> { @display("ls=,0"); } --> GW.elIn[0];
    	electronicControlOutProc <-- { @display("ls=,0"); } <-- GW.elOut[0];
    	
    	
    	photonicMemoryIn <--> { @display("ls=,0"); } <--> GW.phIn[2];
    	GW.phOut[2] <--> { @display("ls=,0"); } <--> photonicMemoryOut;
    
    	electronicControlInMemory --> { @display("ls=,0"); } --> GW.elIn[2];
    
    	electronicControlOutMemory <-- { @display("ls=,0"); } <-- GW.elOut[2];
      
    
    
    
    	GWSwitch.electronicOutGateway --> ElectronicChannel --> GW.elIn[1];
    	GWSwitch.electronicInGateway <-- ElectronicChannel <-- GW.elOut[1];

      

        electronicInN0 --> { @display("ls=red,0"); } --> Sw4x4_SW.electronicInN;
        Sw4x4_SW.electronicOutS --> ElectronicChannel { @display("ls=red,0"); } --> GWSwitch.electronicInInjectionN;
        GWSwitch.electronicOutInjectionS --> { @display("ls=red,0"); } --> electronicOutS0;
        electronicInN1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutS1;
        electronicInS0 --> { @display("ls=red,0"); } --> GWSwitch.electronicInInjectionS;
        GWSwitch.electronicOutInjectionN --> ElectronicChannel { @display("ls=red,0"); } --> Sw4x4_SW.electronicInS;
        Sw4x4_SW.electronicOutN --> electronicOutN0;
        electronicInS1 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutN1;
        electronicInE0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutW0;
        electronicInE1 --> { @display("ls=red,0"); } --> Sw4x4_SW.electronicInE;
        Sw4x4_SW.electronicOutW --> { @display("ls=red,0"); } --> electronicOutW1;
        electronicInW0 --> ElectronicChannel { @display("ls=red,0"); } --> electronicOutE0;
        electronicInW1 --> { @display("ls=red,0"); } --> Sw4x4_SW.electronicInW;
        Sw4x4_SW.electronicOutE --> { @display("ls=red,0"); } --> electronicOutE1;

       

}

