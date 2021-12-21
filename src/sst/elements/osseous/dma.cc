#include <iostream>
#include "DMATop_O1.h"
#include "mm.h"
#include "dma.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <typeinfo>

using namespace std;

static uint64_t trace_count = 0;
static uint64_t main_time = 0;
dma_state state;
axi_writer_data_state axi_w_data_state; 
axi_writer_addr_state axi_w_addr_state;
mm_magic_t* sig_mem; // shared memory interface

/*
Interfaces used:
   AXI4LiteCSR      top->csrFrontend csrFrontend;    //control signal interface
   AXIStreamSlave                    readerFrontend; //source read interface
   AXI4Writer      writerFrontend; //destination write interface
*/

int dma_write_req(uint64_t * source_address, uint64_t *destination_address)
{
    int result;
    switch (state)
    {
    case sIdle :
                //bit that checks if the DMA controller can process write
                if(top->io_control_aw_awvalid)
                {

                    if(!cmd_queue.empty())
                    {
                        if (cmd_queue.front() == 'r')
                        {
                            cmd_queue.pop();
                            cout<<"exiting idle state, executing read request"<<endl;
                            state = sReadAddr;
                        }
                        else if (cmd_queue.front() == 'w')
                        {
                            cmd_queue.pop();
                            cout<<"exiting idle state, executing write request"<<endl;
                            state = sWriteAddr;
                        }          
                    }
                    else
                    {
                        state = sIdle;
                    }
                    //proceed to write state, from the Idle state since resource is available
                    cout<<"Exiting Idle State"<<endl;
                }
        break;
    case sWriteAddr:
                cout<<"state :   sWriteAddr:    DMA Write Request"<<endl;
                top->csrFrontend.addr = *source_address;
                top->csrFrontend.rvalid = 1;
                top->csrFrontend.awready = 0;
                top->csrFrontend.wready = 0;
                top->csrFrontend.arready = 0;

                if (!top->csrFrontend.arready)
                {
                    state=sWriteData;
                }
        break;
    case sWriteData:
                cout<<"state :   sWriteData:    DMA Write Request"<<endl;
                top->csrFrontend.arready = 1;
                top->writerFrontend.bready      = 1;
                top->writerFrontend.done        = 1;
                state = sWriteAddr;
                result = 1;
        break;
    case sReadAddr:
                cout<<"state :   sReadAddr:    DMA Read Request"<<endl;
                top->csrFrontend.addr = *source_address;
                top->csrFrontend.rvalid = 1;
                top->csrFrontend.awready = 0;
                top->csrFrontend.wready = 0;
                top->csrFrontend.arready = 0;

                if (!top->csrFrontend.arready)
                {
                    state=sReadData;
                }
        break;
    case sReadData:
                cout<<"state :   sReadData:    DMA Read Request"<<endl;
                top->csrFrontend.arready = 1;
                top->writerFrontend.bready      = 1;
                top->writerFrontend.done        = 1;
                state = sReadAddr;
                result = 1;

        break;
    case sWriteResp:
                cout<<"state :   sWriteResp"<<endl;

        break;
    default:
        break;
    }
    //cout<<"state :   "<<state<<endl;
    return result;
}

void mem_print(uint64_t * address, int num_bytes)
{
    for(int i=0; i<10;i++)
    {
        cout<<hex<<(address)<<"   ";
        cout<<hex<<setfill('0')<<(*address);
        address++;
        cout<<endl;
    }
    //char *p = (char*)address;
    //int offset;
    //for (offset = 0; offset < num_bytes; offset++) {
    //    printf("%08x", p[offset]);
    //    //printf("\n");
    //    //if (offset % 4 == 1) 
    //    {
    //        printf(" "); 
    //    }
    //}
    printf("\n");
}

int main(int argc, char* argv[]){
    uint64_t timeout = 1000L;
    top = new DMATop;
    axi4reader = new AXIStreamSlave;

    //define the communication interface here
    sig_mem = new mm_magic_t(1L << 32, 8);

    //load the memroy with the data to be transferred
    load_mem(sig_mem->get_data(), (const char*)(argv[1])); 

    /*using self defined physical memory and using it as the MMIO*/
    int dh; //= open("/dev/mem", O_RDWR | O_SYNC); // Open /dev/mem which represents the whole physical memory
    cout<<"hello"<<std::endl;

    // Using external pendrive as a MMIO interface, for preventing memory
    // faults in using /dev/mem
    // change the path accordingly
    if((dh = open("/dev/sdc", O_RDWR | O_SYNC)) == -1) FATAL;

    printf("/dev/sdc opened.\n"); 
    //cout << "Testing MMIO allocated memory!" << endl;
    // Memory map AXI Lite register block
    uint64_t* addresses = (uint64_t*)mmap(NULL, 2*4096UL, PROT_READ | PROT_WRITE, MAP_SHARED, dh, 0x404000000);
    // Memory map source address
    uint64_t* source_address  = (uint64_t*)mmap(NULL, 2*4096UL, PROT_READ | PROT_WRITE, MAP_SHARED, dh, 0x0e0000000); 
    // Memory map destination address
    uint64_t* destination_address = (uint64_t*)mmap(NULL, 2*4096UL, PROT_READ | PROT_WRITE, MAP_SHARED, dh, 0x0f0000000); 

    memset(source_address, 0, 128);
    //4 byte data allocated
    source_address[0]= 0xbbbbbbbbaaaaaaaa;
    // resetting the destination memory block
    // after the write request the value above should get copied to the 
    // destination

    memset(destination_address, 0, 128);

    //test mmio allocated using the print
    cout << "Testing MMIO allocated memory!" << endl<<endl;
    //print 4 byte data on source
    //cout << "Source Address      : Source Data !       :";
    //mem_print(source_address, 4);
    //print 4 byte data on source
    //cout<< "Destination Address : Destination Data !  :";
    //mem_print(destination_address, 4);
    cout<< endl;
    //load data into the memory here
    /*
        Define all input and control signals here
    */
    //resetting and flushing the design
    top->reset = UInt<1>(1);
    /*  
        axi4 interface used for sending the control signals --> AXI4LiteCSR
    */

    cout << "Resetting All the DMA Internal Registers!" << endl;

    /*
        bool update_registers, 
        bool verbose, 
        bool done_reset
    */
   
    top->eval(false, false, false);

    for (int i = 0; i < 3; i++) 
        {
            top->eval(true, false, false);
        }

    top->reset = UInt<1>(0);
    top->eval(false, true, false);

    cout << "Configuring the DMA controller!" << endl;
    cout << "Initializing AXI4Lite Control registers for write request!" << endl;

    //state initialization for the interfaces
    state = sIdle; //axi4lite
    axi_w_data_state = sDataIdle; //axi4writer
    axi_w_addr_state = sAddrIdle; //axi4writer

    //axi4 lite control configuration CSR registers
    
    top->csrFrontend.state = state; //use globally visible state in the FSM
    top->io_control_b_bresp = 0x00;
    top->csrFrontend.bvalid = 0;
    top->clock = 1;
        
    //axi4 Writer configuration, writes data to the destination address
    top->io_write_aw_awid = 0;
    top->writerFrontend.enable      = 0;    
    top->writerFrontend.length      = 64;
    top->writerFrontend.awaddr      = (uint64_t)destination_address;
    top->writerFrontend.awlen       = 64;
    top->writerFrontend.awvalid     = 1;
    top->writerFrontend.bready      = 0;
    top->writerFrontend.done        = 0;
    top->writerFrontend.addrState   = axi_w_addr_state; //use globally visible state in the FSM
    top->writerFrontend.dataState   = axi_w_data_state; //use globally visible state in the FSM

    //axi4stream reader configuration
    top->readerFrontend.done        = 0;
    top->readerFrontend.enable      = 1;
    top->readerFrontend.length      = 64;
    top->io_read_tvalid     = 1; //reader interface from the DUT
    top->io_read_tready     = 1; //data transfer ready bit from the DUT
    top->io_read_tdata      = source_address[0];  //data to be sent from the source address defined in the MMIO interface
    cout<<"top->io_read_tdata -->:"<<top->io_read_tdata<<endl;
    
    cmd_queue.push('r');
    
    int req_result;
    //call request function here inside a loop for multiple requests

    mem_print(source_address, 8);
    mem_print(destination_address, 8);



    cout << "Starting simulation"<<endl;


    for(int j = 0; j<2 ;j++)
    {
        //mem_print(source_address, 8);
        //mem_print(destination_address, 8);
        //source_address++;
        //destination_address++;
        //logic for sending 2 32-bit addresses here
        top->io_write_aw_awid = 0;        



        //this is a 32 bit dma request
        for(int i=0; i<2; i++)
        {
            req_result = dma_write_req(source_address, destination_address);
            top->eval(true, true, true);
            //current logic considers 
            destination_address = (uint64_t*)&top->io_write_w_wdata;
            trace_count++;
        }
            
            
            
            *source_address = *source_address>>16;
            top->io_write_aw_awid = 1 ;//tells the ID of the packet
            cout<<"Request result!     "<< ((req_result == 1)? ("passed"): ("failed"))<<endl;
            cout<<"data from the chisel design : "<<sizeof(top->io_write_w_wdata)<<endl;
            
    }
    

    

   
    //source_address--;
    //source_address--;

    
    mem_print(source_address, 8);
    mem_print(destination_address, 8);

    //memcpy((void*)destination_address, (void*)result, 32);
    //destination_address[0] = (result);
    //cout << "Debug" <<source_address<< endl;


    cout<<hex<< "destination_data      :  "<<*destination_address<<endl;
    cout<< "Clock Ticks Used      :  "<<trace_count<<endl;

    return 0;
}
