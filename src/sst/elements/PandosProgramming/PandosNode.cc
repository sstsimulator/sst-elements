#include <pando/backend_node_context.hpp>
#include <dlfcn.h>
#include "sst_config.h"
#include "PandosNode.h"
#include "PandosEvent.h"

using namespace SST;
using namespace SST::PandosProgramming;

void PandosNodeT::openProgramBinary()
{
        /*
          open program binary in its own namespace 
          this lets us open the same binary multiple times but have
          the symbols retain independent copies
        */    
        program_binary_handle = dlopen(
                program_binary_fname.c_str(),
                RTLD_LAZY | RTLD_LOCAL
                );
        if (!program_binary_handle) {
                out->fatal(CALL_INFO, -1, "Failed to open shared object '%s': %s\n",
                           program_binary_fname.c_str(), dlerror());
                exit(1);
        }
        out->output(CALL_INFO, "opened shared object '%s @ %p\n",
                    program_binary_fname.c_str(),
                    program_binary_handle
                );

        get_current_pando_ctx = (getContextFunc_t)dlsym(
                program_binary_handle,
                "PANDORuntimeBackendGetCurrentContext"
                );
        set_current_pando_ctx = (setContextFunc_t)dlsym(
                program_binary_handle,
                "PANDORuntimeBackendSetCurrentContext"
                );

        pando_context = new pando::backend::node_context_t;
        out->output(CALL_INFO, "made pando context @ %p\n", pando_context);
}

void PandosNodeT::closeProgramBinary()
{
        /*
          close the shared object
        */
        if (program_binary_handle != nullptr) {
                out->output(CALL_INFO, "closing shared object '%s' @ %p\n",
                            program_binary_fname.c_str(),
                            program_binary_handle
                        );
                dlclose(program_binary_handle);
                out->output(CALL_INFO, "closed\n");
                program_binary_handle = nullptr;
        }
}

/**
 * Constructors/Destructors
 */
PandosNodeT::PandosNodeT(ComponentId_t id, Params &params) : Component(id), program_binary_handle(nullptr) {
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

        // open binary
        openProgramBinary();
        
        // Tell the simulation not to end until we're ready
        registerAsPrimaryComponent();
        primaryComponentDoNotEndSim();    

        // Configure link
        port = configureLink("port", new Event::Handler<PandosNodeT>(this, &PandosNodeT::handleEvent));
        
        // Register clock
        registerClock("1GHz", new Clock::Handler<PandosNodeT>(this, &PandosNodeT::clockTic));

        // Spawn PANDO program coroutine
        pando_coroutine_opts_t opts = PANDO_COROUTINE_OPTS_INIT;
        opts.coroutine_entry = PandosNodeT::PANDOProgramStart;
        pando_coroutine_spawn(&pando_program_state, &opts, (void*)this);
}

PandosNodeT::~PandosNodeT() {
        out->output(CALL_INFO, "Goodbye, cruel world!\n");
        closeProgramBinary();
        pando_coroutine_delete(&pando_program_state);
        delete pando_context;
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
        //primaryComponentOKToEndSim();
}

/**
 * Handle a clockTic
 *
 * return true if the clock handler should be disabeld, false o/w
 */
bool PandosNodeT::clockTic(SST::Cycle_t cycle) {
        // auto *ev = new PandosEventT();
        // ev->payload.push_back('h');
        // ev->payload.push_back('i');
        // port->send(ev);

        // execute some pando program
        set_current_pando_ctx(pando_context);
        pando_coroutine_resume(&pando_program_state);
        return false;
}


void PandosNodeT::PANDOProgramStart(
        pando_coroutine_t *cr,
        void *pando_node_v){
        // get the pando node
        PandosNodeT *pando_node = reinterpret_cast<PandosNodeT*>(pando_node_v);

        // yield on first call
        pando_coroutine_yield(cr);

        // find "my_main"                
        mainFunc_t my_main = (mainFunc_t)dlsym(
                pando_node->program_binary_handle,
                "my_main"
                );
        if (!my_main) {
                pando_node->fatal(CALL_INFO, -1, "Could mot symbol '%s'\n", "my_main");
                exit(1);
        }        

        pando_node->set_current_pando_ctx(pando_node->pando_context);
        my_main(0, nullptr);

        // yield indefinitely
        while (1) pando_coroutine_yield(cr);
}
