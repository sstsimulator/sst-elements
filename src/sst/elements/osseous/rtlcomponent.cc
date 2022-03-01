// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst/core/sst_config.h>
#include <sst/core/event.h>
#include "rtlcomponent.h"
#include "rtlevent.h"
#include "rtl_header.h"
#include "rtlmemmgr.h"

using namespace SST;
using namespace std;
using namespace SST::RtlComponent;
using namespace SST::Interfaces;

Rtlmodel::Rtlmodel(SST::ComponentId_t id, SST::Params& params) :
	SST::Component(id)/*, verbosity(static_cast<uint32_t>(out->getVerboseLevel()))*/ {

    bool found;
    dut = new Rtlheader;
    axiport = new AXITop;
    RtlAckEv = new ArielComponent::ArielRtlEvent();
	output.init("Rtlmodel-" + getName() + "-> ", 1, 0, SST::Output::STDOUT);

	RTLClk  = params.find<std::string>("ExecFreq", "1GHz" , found);
    if (!found) {
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1,"couldn't find work per cycle\n");
    }

	maxCycles = params.find<Cycle_t>("maxCycles", 0, found);
    if (!found) {
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1,"couldn't find work per cycle\n");
    }

	/*if(RTLClk == NULL || RTLClk == "0") 
		output.fatal(CALL_INFO, -1, "Error: printFrequency must be greater than zero.\n");*/

 	output.verbose(CALL_INFO, 1, 0, "Config: maxCycles=%" PRIu64 ", RTL Clock Freq=%s\n",
		static_cast<uint64_t>(maxCycles), RTLClk.c_str());

	// Just register a plain clock for this simple example
    output.verbose(CALL_INFO, 1, 0, "Registering RTL clock at %s\n", RTLClk.c_str());
   
   //set our clock 
   clock_handler = new Clock::Handler<Rtlmodel>(this, &Rtlmodel::clockTick); 
   timeConverter = registerClock(RTLClk, clock_handler);
   unregisterClock(timeConverter, clock_handler);
   writePayloads = params.find<int>("writepayloadtrace") == 0 ? false : true;

    //Configure and register Event Handler for ArielRtllink
   ArielRtlLink = configureLink("ArielRtllink", new Event::Handler<Rtlmodel>(this, &Rtlmodel::handleArielEvent)); 

    // Find all the components loaded into the "memory" slot
    // Make sure all cores have a loaded subcomponent in their slot
    cacheLink = loadUserSubComponent<Interfaces::StandardMem>("memory", ComponentInfo::SHARE_NONE, timeConverter, new StandardMem::Handler<Rtlmodel>(this, &Rtlmodel::handleMemEvent));
    if(!cacheLink) {
       std::string interfaceName = params.find<std::string>("memoryinterface", "memHierarchy.standardInterface");
       output.verbose(CALL_INFO, 1, 0, "Memory interface to be loaded is: %s\n", interfaceName.c_str());
       
       Params interfaceParams = params.find_prefix_params("memoryinterfaceparams.");
       interfaceParams.insert("port", "RtlCacheLink");
       cacheLink = loadAnonymousSubComponent<Interfaces::StandardMem>(interfaceName, "memory", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS,
               interfaceParams, timeConverter, new StandardMem::Handler<Rtlmodel>(this, &Rtlmodel::handleMemEvent));
       
       if (!cacheLink)
           output.fatal(CALL_INFO, -1, "%s, Error loading memory interface\n", getName().c_str());
   }

    std::string memorymanager = params.find<std::string>("memmgr", "rtl.MemoryManagerSimple");
    if (NULL != (memmgr = loadUserSubComponent<RtlMemoryManager>("memmgr"))) {
        output.verbose(CALL_INFO, 1, 0, "Loaded memory manager: %s\n", memmgr->getName().c_str());
    } else {
        // Warn about memory levels and the selected memory manager if needed
        if (memorymanager == "rtl.MemoryManagerSimple" /*&& memLevels > 1*/) {
            output.verbose(CALL_INFO, 1, 0, "Warning - the default 'rtl.MemoryManagerSimple' does not support multiple memory levels. Configuring anyways but memorylevels will be 1.\n");
            params.insert("memmgr.memorylevels", "1", true);
        }

        output.verbose(CALL_INFO, 1, 0, "Loading memory manager: %s\n", memorymanager.c_str());
        Params mmParams = params.find_prefix_params("memmgr");
        memmgr = loadAnonymousSubComponent<RtlMemoryManager>(memorymanager, "memmgr", 0, ComponentInfo::SHARE_NONE | ComponentInfo::INSERT_STATS, mmParams);
        if (NULL == memmgr) output.fatal(CALL_INFO, -1, "Failed to load memory manager: %s\n", memorymanager.c_str());
    }

    output.verbose(CALL_INFO, 1, 0, "RTL Memory manager construction is completed.\n");

   pendingTransactions = new std::unordered_map<StandardMem::Request::id_t, StandardMem::Request*>();
   pending_transaction_count = 0;
   unregisterClock(timeConverter, clock_handler);
   isStalled = true;

    statReadRequests  = registerStatistic<uint64_t>( "read_requests");
    statWriteRequests = registerStatistic<uint64_t>( "write_requests");
    statReadRequestSizes = registerStatistic<uint64_t>( "read_request_sizes");
    statWriteRequestSizes = registerStatistic<uint64_t>( "write_request_sizes");
    statSplitReadRequests = registerStatistic<uint64_t>( "split_read_requests");
    statSplitWriteRequests = registerStatistic<uint64_t>( "split_write_requests");
    statFlushRequests = registerStatistic<uint64_t>( "flush_requests");
    statFenceRequests = registerStatistic<uint64_t>( "fence_requests");


   // Tell SST to wait until we authorize it to exit
   registerAsPrimaryComponent();
   primaryComponentDoNotEndSim();

   assert(ArielRtlLink);
}

Rtlmodel::~Rtlmodel() {
    delete dut;
    delete axiport;
    delete RtlAckEv;
}

void Rtlmodel::setup() {
    dut->reset = UInt<1>(1);
    axiport->reset = UInt<1>(1);
	output.verbose(CALL_INFO, 1, 0, "Component is being setup.\n");
    for(int i = 0; i < 512; i++)
        axiport->queue.ram[i] = 0;
    axiport->eval(true,true,true);
    axiport->reset = UInt<1>(0);
}

void Rtlmodel::init(unsigned int phase) {
	output.verbose(CALL_INFO, 1, 0, "Component Init Phase Called %d\n", phase);
    cacheLink->init(phase);
}

//Nothing to add in finish as of now. Need to see what could be added.
void Rtlmodel::finish() {
	output.verbose(CALL_INFO, 1, 0, "Component is being finished.\n");
    free(getBaseDataAddress());
}

//clockTick will actually execute the RTL design at every cycle based on the input and control signals updated by CPU CPU or Event Handler.
bool Rtlmodel::clockTick( SST::Cycle_t currentCycle ) {

    /*uint64_t DUTclk; 
    char* endPtr;
    DUTclk = strtoull(RTLClk.c_str(), &endPtr, 10);
    DUTcycles = 10^9/DUTclk;
	unitAlgebra clock_ua(RTLClk);
    if( currentCycle %  == 0 ) {
		output.verbose(CALL_INFO, 1, 0, "Hello World!\n");
	}*/

    if(!isStalled) {
        dut->eval(ev.update_registers, ev.verbose, ev.done_reset);
        if(tickCount == 4) {
            output.verbose(CALL_INFO, 1, 0, "AXI signals changed"); 
            axi_tvalid_$next = 1;
            axi_tdata_$next = 34;
            cout<<"\n Sending data at tickCount 4";
        }
     tickCount++;
    }

    if((axi_tvalid_$old ^ axi_tvalid_$next) || (axi_tdata_$old ^ axi_tdata_$next))  {
        uint8_t ready = 1;
        output.verbose(CALL_INFO, 1, 0, "handleAXISignals called"); 
        if(axiport->queue.maybe_full) 
            ready = 0;
        handleAXISignals(ready); 
        axiport->eval(true, true, true);

        //Initial value of AXI control signals
        fifo_enq_$old = axiport->queue.value_1.as_single_word();
        fifo_enq_$next = axiport->queue.value.as_single_word();
        uint64_t prev_data = axiport->queue.ram[fifo_enq_$old].as_single_word();

        while(!(prev_data ^ axiport->queue.ram[fifo_enq_$next].as_single_word())) {
            prev_data = axiport->queue.ram[fifo_enq_$next].as_single_word();
            axiport->eval(true, true, true);
            fifo_enq_$next = axiport->queue.value.as_single_word();
            if(fifo_enq_$old ^ fifo_enq_$next) {
                cout<<"\nQueue_value is: "<<axiport->queue.value<<fifo_enq_$next;
                cout<<"\nData enqueued in the queue: "<< axiport->queue.ram[fifo_enq_$next];
            }
            fifo_enq_$old = fifo_enq_$next;
        }
    }

    axi_tdata_$old = axi_tdata_$next;
    axi_tvalid_$old = axi_tvalid_$next;
    axi_tready_$old = axi_tready_$next;

    uint64_t read_addr = (axiport->queue.ram[fifo_enq_$next].as_single_word());// << 32) | (axiport->queue.ram[fifo_enq_$next+1].as_single_word());
    uint64_t size = (axiport->queue.ram[fifo_enq_$next+2].as_single_word());// << 32) | (axiport->queue.ram[fifo_enq_$next+3].as_single_word());
    cout << "\nread_addr is: "<<read_addr;
    cout << "\nsize is: "<<size;

    //RtlReadEvent* axi_readev = new RtlReadEvent(read_addr, size); 
    //generateReadRequest(axi_readev);

    cout<<"Sim Done is: "<<ev.sim_done;

	if( tickCount >= sim_cycle /*|| tickCount >= maxCycles*/ ) {
        //output.verbose(CALL_INFO, 1, 0, "sim_cycle ending is: %" PRIu64, sim_cycle);
        if(ev.sim_done) {
            output.verbose(CALL_INFO, 1, 0, "OKToEndSim, TickCount %" PRIu64, tickCount);
            RtlAckEv->setEndSim(true);
            ArielRtlLink->send(RtlAckEv);
            primaryComponentOKToEndSim();  //Tell the SST that it can finish the simulation.
            return true;
        }
	} 
    
    else 
        return false;
}


/*Event Handle will be called by Ariel CPU once it(Ariel CPU) puts the input and control signals in the shared memory. Now, we need to modify Ariel CPU code for that.
Event handler will update the input and control signal based on the workload/C program to be executed.
Don't know what should be the argument of Event handle as of now. As, I think we don't need any argument. It's just a requst/call by Ariel CPU to update input and control signals.*/
void Rtlmodel::handleArielEvent(SST::Event *event) {
    /*
    * Event will pick information from shared memory. (What will be the use of Event queue.)
    * Need to insert code for it. 
    * Probably void pointers will be used to get the data from the shared memory which will get casted based on the width set by the user at runtime.
    * void pointers will be defined by Ariel CPU and passed as parameters through SST::Event to the C++ model. 
    * As of now, shared memory is like a scratch-pad or heap which is passive without any intelligent performance improving stuff like TLB, Cache hierarchy, accessing mechanisms(VIPT/PIPT) etc.  
    */

    unregisterClock(timeConverter, clock_handler);
    ArielComponent::ArielRtlEvent* ariel_ev = dynamic_cast<ArielComponent::ArielRtlEvent*>(event);
    RtlAckEv->setEventRecvAck(true);
    ArielRtlLink->send(RtlAckEv);

    output.verbose(CALL_INFO, 1, 0, "\nVecshiftReg RTL Event handle called \n");

    memmgr->AssignRtlMemoryManagerSimple(*ariel_ev->RtlData.pageTable, ariel_ev->RtlData.freePages, ariel_ev->RtlData.pageSize);
    memmgr->AssignRtlMemoryManagerCache(*ariel_ev->RtlData.translationCache, ariel_ev->RtlData.translationCacheEntries, ariel_ev->RtlData.translationEnabled);

    //Update all the virtual address pointers in RTLEvent class
    updated_rtl_params = ariel_ev->get_updated_rtl_params();
    inp_ptr = ariel_ev->get_rtl_inp_ptr(); 
    inp_size = ariel_ev->RtlData.rtl_inp_size;
    cacheLineSize = ariel_ev->RtlData.cacheLineSize;

    //Creating Read Event from memHierarchy for the above virtual address pointers
    RtlReadEvent* rtlrev_params = new RtlReadEvent((uint64_t)ariel_ev->get_updated_rtl_params(),(uint32_t)ariel_ev->get_updated_rtl_params_size()); 
    RtlReadEvent* rtlrev_inp_ptr = new RtlReadEvent((uint64_t)ariel_ev->get_rtl_inp_ptr(),(uint32_t)ariel_ev->get_rtl_inp_size()); 
    RtlReadEvent* rtlrev_ctrl_ptr = new RtlReadEvent((uint64_t)ariel_ev->get_rtl_ctrl_ptr(),(uint32_t)ariel_ev->get_rtl_ctrl_size()); 
    output.verbose(CALL_INFO, 1, 0, "\nVirtual address in handleArielEvent is: %" PRIu64, (uint64_t)ariel_ev->get_updated_rtl_params());

    if(!mem_allocated) {
        size_t size = ariel_ev->get_updated_rtl_params_size() + ariel_ev->get_rtl_inp_size() + ariel_ev->get_rtl_ctrl_size();
        uint8_t* data = (uint8_t*)malloc(size);
        VA_VA_map.insert({(uint64_t)ariel_ev->get_updated_rtl_params(), (uint64_t)data});
        uint64_t index = ariel_ev->get_updated_rtl_params_size()/sizeof(uint8_t);
        VA_VA_map.insert({(uint64_t)ariel_ev->get_rtl_inp_ptr(), (uint64_t)(data+index)});
        index += ariel_ev->get_rtl_inp_size()/sizeof(uint8_t);
        VA_VA_map.insert({(uint64_t)ariel_ev->get_rtl_ctrl_ptr(), (uint64_t)(data+index)});
        setBaseDataAddress(data);
        setDataAddress(getBaseDataAddress());
        mem_allocated = true;
    }



    //Initiating the read request from memHierarchy
    generateReadRequest(rtlrev_params);
    generateReadRequest(rtlrev_inp_ptr);
    generateReadRequest(rtlrev_ctrl_ptr);
    isStalled = true;
    sendArielEvent();
}

/*void Rtlmodel::handleAXIEvent(uint64_t* data) {
    //axi4 Writer configuration, writes data to the destination address
    axiport->io_write_aw_awid = 0;
    axiport->writerFrontend.enable      = 0;    
    axiport->writerFrontend.length      = 64;
    axiport->writerFrontend.awaddr      = (uint64_t)destination_address;
    axiport->writerFrontend.awlen       = 64;
    axiport->writerFrontend.awvalid     = 1;
    axiport->writerFrontend.bready      = 0;
    axiport->writerFrontend.done        = 0;
    axiport->writerFrontend.addrState   = axi_w_addr_state; //use globally visible state in the FSM
    axiport->writerFrontend.dataState   = axi_w_data_state; //use globally visible state in the FSM
    cout<<"axiport->io_read_tdata -->:"<<axiport->io_read_tdata<<endl;
    
    cmd_queue.push('r');

    for(int j = 0; j<2 ;j++)
    {
        //mem_print(source_address, 8);
        //mem_print(destination_address, 8);
        //source_address++;
        //destination_address++;
        //logic for sending 2 32-bit addresses here
        axiport->io_write_aw_awid = 0;        



        //this is a 32 bit dma request
        for(int i=0; i<2; i++)
        {
            req_result = dma_write_req(data[0], data[i]);
            axiport->eval(true, true, true);
            //current logic considers 
            destination_address = (uint64_t*)&axiport->io_write_w_wdata;
            trace_count++;
        }
            
            *source_address = *source_address>>16;
            axiport->io_write_aw_awid = 1 ;//tells the ID of the packet
            cout<<"Request result!     "<< ((req_result == 1)? ("passed"): ("failed"))<<endl;
            cout<<"data from the chisel design : "<<sizeof(axiport->io_write_w_wdata)<<endl;
            
    }
    
}*/

void Rtlmodel::sendArielEvent() {
     
    RtlAckEv = new ArielComponent::ArielRtlEvent();
    RtlAckEv->RtlData.rtl_inp_ptr = inp_ptr;
    RtlAckEv->RtlData.rtl_inp_size = inp_size;
    ArielRtlLink->send(RtlAckEv);
    return;
}


void Rtlmodel::handleAXISignals(uint8_t tready) {
    axiport->readerFrontend.done = 0;
    axiport->readerFrontend.enable = 1;
    axiport->readerFrontend.length = 64;
    axiport->io_read_tdata = axi_tdata_$next; 
    axiport->io_read_tvalid = axi_tvalid_$next; 
    axiport->io_read_tready = tready; 
    //axiport->cmd_queue.push('r');
}

void Rtlmodel::handleMemEvent(StandardMem::Request* event) {
    StandardMem::ReadResp* read = (StandardMem::ReadResp*)event;
    output.verbose(CALL_INFO, 4, 0, " handling a memory event in RtlModel.\n");
    StandardMem::Request::id_t mev_id = read->getID();
    auto find_entry = pendingTransactions->find(mev_id);
    if(find_entry != pendingTransactions->end()) {
        output.verbose(CALL_INFO, 4, 0, "Correctly identified event in pending transactions, removing from list, before there are: %" PRIu32 " transactions pending.\n", (uint32_t) pendingTransactions->size());
       
        int i;
        auto DataAddress = VA_VA_map.find(read->vAddr);
        if(DataAddress != VA_VA_map.end())
            setDataAddress((uint8_t*)DataAddress->second);
        else
            output.fatal(CALL_INFO, -1, "Error: DataAddress corresponding to VA: %" PRIu64, read->vAddr);

        //Actual reading of data from memEvent and storing it to getDataAddress
        output.verbose(CALL_INFO, 1, 0, "\nAddress is: %" PRIu64, (uint64_t)getDataAddress());
        for(i = 0; i < read->data.size(); i++)
            getDataAddress()[i] = read->data[i]; 

        if(read->vAddr == (uint64_t)updated_rtl_params) {
            bool* ptr = (bool*)getBaseDataAddress();
            output.verbose(CALL_INFO, 1, 0, "Updated Rtl Params is: %d\n",*ptr);
        }

        pendingTransactions->erase(find_entry);
        pending_transaction_count--;

        if(isStalled && pending_transaction_count == 0) {
            ev.UpdateRtlSignals((void*)getBaseDataAddress(), dut, sim_cycle);
            tickCount = 0;
            reregisterClock(timeConverter, clock_handler);
            setDataAddress(getBaseDataAddress());
            isStalled = false;
        }
    } 
    
    else 
        output.fatal(CALL_INFO, -4, "Memory event response to VecShiftReg was not found in pending list.\n");
        
    delete event;
}

void Rtlmodel::commitReadEvent(const uint64_t address,
            const uint64_t virtAddress, const uint32_t length) {
    if(length > 0) {
        StandardMem::Read *req = new StandardMem::Read(address, length, 0, virtAddress);
        
        pending_transaction_count++;
        pendingTransactions->insert(std::pair<StandardMem::Request::id_t, StandardMem::Request*>(req->getID(), req));

        // Actually send the event to the cache
        cacheLink->send(req);
    }
}

void Rtlmodel::commitWriteEvent(const uint64_t address,
        const uint64_t virtAddress, const uint32_t length, const uint8_t* payload) {

    if(length > 0) {

        std::vector<uint8_t> data;

        if( writePayloads ) {
                data.insert(data.end(), &payload[0], &payload[length]);
                char* buffer = new char[64];
                std::string payloadString = "";
                for(int i = 0; i < length; ++i) {
                    sprintf(buffer, "0x%X ", payload[i]);
                    payloadString.append(buffer);
                }

                delete[] buffer;

                output.verbose(CALL_INFO, 16, 0, "Write-Payload: Len=%" PRIu32 ", Data={ %s } %p\n",
                        length, payloadString.c_str(), (void*)virtAddress);
        } else {
            data.resize(length, 0);
        }
        
        StandardMem::Write *req = new StandardMem::Write(address, length, data, false, 0, virtAddress);
        pending_transaction_count++;
        pendingTransactions->insert( std::pair<StandardMem::Request::id_t, StandardMem::Request*>(req->getID(), req) );

        // Actually send the event to the cache
        cacheLink->send(req);
    }
}

void Rtlmodel::generateReadRequest(RtlReadEvent* rEv) {

    const uint64_t readAddress = rEv->getAddress();
    const uint64_t readLength  = std::min((uint64_t) rEv->getLength(), cacheLineSize); // Trim to cacheline size (occurs rarely for instructions such as xsave and fxsave)

    // NOTE: Physical and virtual addresses may not be aligned the same w.r.t. line size if map-on-malloc is being used (arielinterceptcalls != 0), so use physical offsets to determine line splits
    // There is a chance that the non-alignment causes an undetected bug if an access spans multiple malloc regions that are contiguous in VA space but non-contiguous in PA space.
    // However, a single access spanning multiple malloc'd regions shouldn't happen...
    // Addresses mapped via first touch are always line/page aligned
    const uint64_t physAddr = memmgr->translateAddress(readAddress);
    const uint64_t addr_offset  = physAddr % ((uint64_t) cacheLineSize);

    if((addr_offset + readLength) <= cacheLineSize) {
        output.verbose(CALL_INFO, 4, 0, " generating a non-split read request: Addr=%" PRIu64 " Length=%" PRIu64 "\n",
                            readAddress, readLength);

        // We do not need to perform a split operation

        output.verbose(CALL_INFO, 4, 0, " issuing read, VAddr=%" PRIu64 ", Size=%" PRIu64 ", PhysAddr=%" PRIu64 "\n",
                            readAddress, readLength, physAddr);

        commitReadEvent(physAddr, readAddress, (uint32_t) readLength);
    } else {
        output.verbose(CALL_INFO, 4, 0, " generating a split read request: Addr=%" PRIu64 " Length=%" PRIu64 "\n",
                            readAddress, readLength);

        // We need to perform a split operation
        const uint64_t leftAddr = readAddress;
        const uint64_t leftSize = cacheLineSize - addr_offset;

        const uint64_t rightAddr = (readAddress + ((uint64_t) cacheLineSize)) - addr_offset;
        const uint64_t rightSize = readLength - leftSize;

        const uint64_t physLeftAddr = physAddr;
        const uint64_t physRightAddr = memmgr->translateAddress(rightAddr);

        output.verbose(CALL_INFO, 4, 0, " issuing split-address read, LeftVAddr=%" PRIu64 ", RightVAddr=%" PRIu64 ", LeftSize=%" PRIu64 ", RightSize=%" PRIu64 ", LeftPhysAddr=%" PRIu64 ", RightPhysAddr=%" PRIu64 "\n",
                            leftAddr, rightAddr, leftSize, rightSize, physLeftAddr, physRightAddr);

        if(((physLeftAddr + leftSize) % cacheLineSize) != 0) {
            output.fatal(CALL_INFO, -4, "Error leftAddr=%" PRIu64 " + size=%" PRIu64 " is not a multiple of cache line size: %" PRIu64 "\n",
                    leftAddr, leftSize, cacheLineSize);
        }

        commitReadEvent(physLeftAddr, leftAddr, (uint32_t) leftSize);
        commitReadEvent(physRightAddr, rightAddr, (uint32_t) rightSize);

        statSplitReadRequests->addData(1);
    }

    statReadRequests->addData(1);
    statReadRequestSizes->addData(readLength);
    delete rEv;
}

void Rtlmodel::generateWriteRequest(RtlWriteEvent* wEv) {

    const uint64_t writeAddress = wEv->getAddress();
    const uint64_t writeLength  = std::min((uint64_t) wEv->getLength(), cacheLineSize); // Trim to cacheline size (occurs rarely for instructions such as xsave and fxsave)

    // See note in handleReadRequest() on alignment issues
    const uint64_t physAddr = memmgr->translateAddress(writeAddress);
    const uint64_t addr_offset  = physAddr % ((uint64_t) cacheLineSize);

    // We do not need to perform a split operation
    if((addr_offset + writeLength) <= cacheLineSize) {
        
        output.verbose(CALL_INFO, 4, 0, " generating a non-split write request: Addr=%" PRIu64 " Length=%" PRIu64 "\n",
                            writeAddress, writeLength);


        output.verbose(CALL_INFO, 4, 0, " issuing write, VAddr=%" PRIu64 ", Size=%" PRIu64 ", PhysAddr=%" PRIu64 "\n",
                            writeAddress, writeLength, physAddr);

        if(writePayloads) {
            uint8_t* payloadPtr = wEv->getPayload();
            commitWriteEvent(physAddr, writeAddress, (uint32_t) writeLength, payloadPtr);
        } else {
            commitWriteEvent(physAddr, writeAddress, (uint32_t) writeLength, NULL);
        }
    } else {
        output.verbose(CALL_INFO, 4, 0, " generating a split write request: Addr=%" PRIu64 " Length=%" PRIu64 "\n",
                            writeAddress, writeLength);

        // We need to perform a split operation
        const uint64_t leftAddr = writeAddress;
        const uint64_t leftSize = cacheLineSize - addr_offset;

        const uint64_t rightAddr = (writeAddress + ((uint64_t) cacheLineSize)) - addr_offset;
        const uint64_t rightSize = writeLength - leftSize;

        const uint64_t physLeftAddr = physAddr;
        const uint64_t physRightAddr = memmgr->translateAddress(rightAddr);

        output.verbose(CALL_INFO, 4, 0, " issuing split-address write, LeftVAddr=%" PRIu64 ", RightVAddr=%" PRIu64 ", LeftSize=%" PRIu64 ", RightSize=%" PRIu64 ", LeftPhysAddr=%" PRIu64 ", RightPhysAddr=%" PRIu64 "\n",
                            leftAddr, rightAddr, leftSize, rightSize, physLeftAddr, physRightAddr);

        if(((physLeftAddr + leftSize) % cacheLineSize) != 0) {
            output.fatal(CALL_INFO, -4, "Error leftAddr=%" PRIu64 " + size=%" PRIu64 " is not a multiple of cache line size: %" PRIu64 "\n",
                            leftAddr, leftSize, cacheLineSize);
        }

        if(writePayloads) {
            uint8_t* payloadPtr = wEv->getPayload();
            commitWriteEvent(physLeftAddr, leftAddr, (uint32_t) leftSize, payloadPtr);
            commitWriteEvent(physRightAddr, rightAddr, (uint32_t) rightSize, &payloadPtr[leftSize]);
        } else {
            commitWriteEvent(physLeftAddr, leftAddr, (uint32_t) leftSize, NULL);
            commitWriteEvent(physRightAddr, rightAddr, (uint32_t) rightSize, NULL);
        }
        statSplitWriteRequests->addData(1);
    }

    statWriteRequests->addData(1);
    statWriteRequestSizes->addData(writeLength);
    delete wEv;
}

uint8_t* Rtlmodel::getDataAddress() {
    return dataAddress;
}

uint64_t* Rtlmodel::getAXIDataAddress() {
    return AXIdataAddress;
}


void Rtlmodel::setDataAddress(uint8_t* virtAddress){
    dataAddress = virtAddress;
}

uint8_t* Rtlmodel::getBaseDataAddress(){
    return baseDataAddress;
}

void Rtlmodel::setBaseDataAddress(uint8_t* virtAddress){
    baseDataAddress = virtAddress;
}
