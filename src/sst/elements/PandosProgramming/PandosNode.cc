#include <pando/backend_node_context.hpp>
#include <pando/backend_task.hpp>
#include <dlfcn.h>
#include "sst_config.h"
#include "PandosNode.h"
#include "PandosEvent.h"
#include "PandosMemoryRequestEvent.h"


using namespace SST;
using namespace SST::PandosProgramming;

void PandosNodeT::checkCoreID(int line, const char *file, const char *function, int core_id) {
        if (core_id < 0 || core_id >= core_contexts.size()) {
                out->fatal(line, file, function, -1, "%s: bad core id = %d, num_cores = %d\n"
                           ,getName().c_str()
                           ,core_id
                           ,core_contexts.size());
                abort();
        }
}

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
        out->verbose(CALL_INFO, 1, 0, "opened shared object '%s @ %p\n",
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

        pando_context = new pando::backend::node_context_t(static_cast<int64_t>(getId()));
        out->verbose(CALL_INFO, 1, 0, "made pando context @ %p\n", pando_context);
}

void PandosNodeT::closeProgramBinary()
{
        /*
          close the shared object
        */
        if (program_binary_handle != nullptr) {
                out->verbose(CALL_INFO, 1, 0, "closing shared object '%s' @ %p\n",
                            program_binary_fname.c_str(),
                            program_binary_handle
                        );
                dlclose(program_binary_handle);
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
                core->id = c;
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
                        out->verbose(CALL_INFO, 1, 0, "my_main() has returned\n");
                });
        cc0->task_deque.push_front(main_task);
}

/**
 * Constructors/Destructors
 */
PandosNodeT::PandosNodeT(ComponentId_t id, Params &params) : Component(id), program_binary_handle(nullptr) {
        // Read parameters
        bool found;
        int64_t verbose_level = params.find<int32_t>("verbose_level", 0, found);
        num_cores = params.find<int32_t>("num_cores", 1, found);
        instr_per_task = params.find<int32_t>("instr_per_task", 100, found);
        program_binary_fname = params.find<std::string>("program_binary_fname", "", found);

        out = new Output("[PandosNode] ", verbose_level, 0, Output::STDOUT);
        out->verbose(CALL_INFO, 2, 0, "Hello, world!\n");    
        
        out->verbose(CALL_INFO, 1, 0, "num_cores = %d, instr_per_task = %d, program_binary_fname = %s\n"
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

        // configure the coreLocalSPM link
        coreLocalSPM = configureLink("coreLocalSPM", new Event::Handler<PandosNodeT>(this, &PandosNodeT::recvMemoryResponse));
        podSharedDRAM = configureLink("podSharedDRAM", new Event::Handler<PandosNodeT>(this, &PandosNodeT::recvMemoryResponse));
        
        // Register clock
        registerClock("1GHz", new Clock::Handler<PandosNodeT>(this, &PandosNodeT::clockTic));
}

PandosNodeT::~PandosNodeT() {
        out->verbose(CALL_INFO, 2, 0, "Goodbye, cruel world!\n");
        for (int64_t cc = 0; cc < core_contexts.size(); cc++) {
                delete core_contexts[cc];
                core_contexts[cc] = nullptr;
        }
        core_contexts.clear();
        closeProgramBinary();
        delete out;
}

/**
 * send memory request on behalf of a core
 */
void PandosNodeT::sendMemoryRequest(int src_core) {
        using namespace pando;
        using namespace backend;
        checkCoreID(CALL_INFO, src_core);
        core_context_t *core_ctx = core_contexts[src_core];

        // create a new event
        PandosMemoryRequestEventT *req = new PandosMemoryRequestEventT;
        req->src_core = src_core;
        req->size = 0;

        // send event on link
        if (core_ctx->core_state.mem_request_addr.dram_not_spm) {
                podSharedDRAM->send(req);                
        } else {
                coreLocalSPM->send(req);
        }
}

/**
 * handle a response from memory to a request
 */
void PandosNodeT::recvMemoryResponse(SST::Event *memrsp) {
        using namespace pando;
        using namespace backend;
        out->verbose(CALL_INFO, 1, 0, "Memory response recieved.\n");

        // check that this is actually the right type of event        
        PandosMemoryRequestEventT *mem_request_event = dynamic_cast<PandosMemoryRequestEventT*>(memrsp);
        if (!mem_request_event) {
                out->fatal(CALL_INFO, -1
                           ,"%s: %s: Received an event that is not a memory request\n"
                           ,__func__
                           ,getName().c_str()
                        );
                return;
        }
        
        // map memory request back to the core from which it originated
        int core_id = mem_request_event->src_core;
        checkCoreID(CALL_INFO, core_id);

        core_context_t *core_ctx = core_contexts[core_id];

        // make core as ready
        core_ctx->core_state.type = eCoreReady;

        // cleanup the event
        delete mem_request_event;
}

        
/**
 * Handle a clockTic
 *
 * return true if the clock handler should be disabeld, false o/w
 */
bool PandosNodeT::clockTic(SST::Cycle_t cycle) {
        using namespace pando;
        using namespace backend;
        
        // execute some pando program        
        set_current_pando_ctx(pando_context);
        
        // have each core execute if not busy
        for (int core_id = 0; core_id < core_contexts.size(); core_id++) {
                core_context_t *core = core_contexts[core_id];
                // TODO: this should maybe be a while () loop instead...
                if (core->core_state.type == eCoreReady || !core->task_deque.empty()) {
                        // execute some code on this core
                        core->execute();
                        // check the core's state
                        switch (core->core_state.type) {
                        case eCoreStallMemory:
                                // generate an event an depending on the memory type
                                sendMemoryRequest(core_id);
                                break;
                        case eCoreReady:
                        case eCoreIdle:
                        default:                                
                                break;
                                
                        }
                }                
        }

        return false;
}
