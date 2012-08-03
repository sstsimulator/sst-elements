package ioComponents;

import ioComponents.cDramModule;

import photonic.devices.Facet;
import photonic.devices.Detector;
import photonic.devices.Modulator;

import electronicComponents.Serializer;
import electronicComponents.mux;
import electronicComponents.demux;

import processingPlane.interfaces.NIF;

module MemNode_Electronic
{
    parameters:
        int dramID;
        
        string NIF_type;
       
        double dramFrequency;
        //double offChipClockRate;
        
        double IL_Facet_Lateral;
        
        int numOfWavelengthChannels;
        double firstWavelength;
        double Switch_FreeSpectralRange;
        double dispValue;
        bool useWDM;
        
        @display("i=old/pccard");
    gates:
        input elIn;
       
        output elOut;
        
        inout phIn;
        inout phOut;
        
    submodules:
        mem: cDramModule {
            parameters:
                dramID = dramID;

                @display("p=56,170");
        }
        
        txrx: <NIF_type> like NIF {
           id = dramID;
            @display("p=158,114");
        }
        
     
    connections allowunconnected:
     
        elIn --> txrx.portIn[0];
        elOut <-- txrx.portOut[0];
    

        mem.nicReq <--> txrx.procReq;
        mem.fromNic <-- txrx.toProc;
        mem.toNic --> txrx.fromProc;
       
        
}
