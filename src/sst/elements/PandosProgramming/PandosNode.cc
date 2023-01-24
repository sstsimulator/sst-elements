#include "sst_config.h"
#include "PandosNode.h"
#include "PandosEvent.h"

using namespace SST;
using namespace SST::PandosProgramming;

/**
 * Constructors/Destructors
 */
PandosNodeT::PandosNodeT(ComponentId_t id, Params &params) : Component(id) {
        out = new Output("[PandosNode] ", 1, 0, Output::STDOUT);
        out->output(CALL_INFO, "Hello, world!\n");    

        // Read parameters
        bool found;
        num_cores = params.find<int32_t>("num_cores", 1, found);
        instr_per_task = params.find<int32_t>("instr_per_task", 100, found);
        program_binary_fname = params.find<std::string>("program_binary_fname", "", found);
        
        out->output(CALL_INFO, "num_cores = %d, instr_per_task = %d, program_binary_fname = %s\n"
                    ,num_cores
                    ,instr_per_task
                    ,program_binary_fname.c_str());
        
        // Tell the simulation not to end until we're ready
        registerAsPrimaryComponent();
        primaryComponentDoNotEndSim();    

        // Configure link
        port = configureLink("port", new Event::Handler<PandosNodeT>(this, &PandosNodeT::handleEvent));
        
        // Register clock
        registerClock("1GHz", new Clock::Handler<PandosNodeT>(this, &PandosNodeT::clockTic));
}

PandosNodeT::~PandosNodeT() {
        out->output(CALL_INFO, "Goodbye, cruel world!\n");
        delete out;
}

/**
 * Handles event when it arrives on the link
 */
void PandosNodeT::handleEvent(SST::Event *ev) {    
        out->output(CALL_INFO, "Event received.\n");
        PandosEventT *pev = dynamic_cast<PandosEventT*>(ev);
        if (!pev) {
                out->fatal(CALL_INFO, -1, "Bad event type received by %s\n", getName().c_str());
        } else {
                delete pev;
        }
        primaryComponentOKToEndSim();
}

/**
 * Handle a clockTic
 *
 * return true if the clock handler should be disabeld, false o/w
 */
bool PandosNodeT::clockTic(SST::Cycle_t cycle) {
        auto *ev = new PandosEventT();
        ev->payload.push_back('h');
        ev->payload.push_back('i');
        port->send(ev);
        return false;
}


