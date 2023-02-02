#include <pando/backend_node_context.hpp>
#include <pando/backend_task.hpp>
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
 * initalize cores
 */
void PandosNodeT::initCores()
{
        /**
         * start all cores
         */
        for (int64_t c = 0; c < num_cores; c++) {
                auto *core = new pando::backend::core_context_t(pando_context);
                core_contexts.push_back(core);
                core->start();
        }

        /**
         * enque an initial task on core 0
         */
        pando::backend::core_context_t *cc0 = core_contexts[0];
        mainFunc_t my_main = (mainFunc_t)dlsym(
                program_binary_handle,
                "my_main"
                );

        if (!my_main) {
                out->fatal(CALL_INFO, -1, "failed to find symbol '%s'\n", "my_main");
                exit(1);
        }

        auto main_task = pando::backend::NewTask([=]()mutable{
                        my_main(0, nullptr);
                        out->output(CALL_INFO, "my_main() has returned\n");
                });
        cc0->task_deque.push_front(main_task);
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

        // initialize cores
        initCores();

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
        for (int64_t cc = 0; cc < core_contexts.size(); cc++) {
                delete core_contexts[cc];
                core_contexts[cc] = nullptr;
        }
        core_contexts.clear();
        closeProgramBinary();
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

        // have each core execute if not busy
        for (pando::backend::core_context_t *core : core_contexts) {
                if (core->busy || !core->task_deque.empty()) {
                        core->execute();
                }
        }

        return false;
}
