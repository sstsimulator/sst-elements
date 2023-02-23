#include <pando/backend_node_context.hpp>
#include <pando/backend_task.hpp>
#include <dlfcn.h>
#include "sst_config.h"
#include "PandosNode.h"
#include "PandosEvent.h"
#include "PandosMemoryRequestEvent.h"
#include "PandosPacketEvent.h"

#define DO_MEMCPY

using namespace SST;
using namespace SST::PandosProgramming;

#define DEBUG_SCHEDULE        0x00000001
#define DEBUG_MEMORY_REQUESTS 0x00000002

/**
 * schedule work onto a core
 */
void PandosNodeT::schedule(int thief_core_id) {
        using namespace pando;
        using namespace backend;
        core_context_t *thief_core_ctx = core_contexts[thief_core_id];
        out->verbose(CALL_INFO, 3, DEBUG_SCHEDULE, "%s: scheduling for thief core %d\n", __func__, thief_core_id);
        // search for a core that's not idle
        for (int victim_core_id = 0;
             victim_core_id < core_contexts.size();
             victim_core_id++) {
                core_context_t *victim_core_ctx = core_contexts[victim_core_id];
                out->verbose(CALL_INFO, 4, DEBUG_SCHEDULE, "%s: scanning victim %d\n", __func__, victim_core_id);
                // skip over self                
                if (victim_core_id == thief_core_id) {
                        out->verbose(CALL_INFO, 4, DEBUG_SCHEDULE, "%s: victim %d: = thief\n", __func__, victim_core_id);
                        continue;
                }
                // skip over idle cores
                if (victim_core_ctx->core_state.type == eCoreIdle) {
                        out->verbose(CALL_INFO, 4, DEBUG_SCHEDULE, "%s: victim %d: is idle\n", __func__, victim_core_id);
                        continue;
                }
                // skip over if task queeu is empty
                if (victim_core_ctx->task_deque.empty()) {
                        out->verbose(CALL_INFO, 4, DEBUG_SCHEDULE, "%s: victim %d: task queue empty\n", __func__, victim_core_id);
                        continue;
                }
                
                out->verbose(CALL_INFO, 1, DEBUG_SCHEDULE, "%s: thief %d stealing from victim %d\n",
                             __func__, thief_core_id, victim_core_id);
                                
                // steal a task from the back
                task_t *stolen = victim_core_ctx->task_deque.back();
                victim_core_ctx->task_deque.pop_back();
                thief_core_ctx->task_deque.push_front(stolen);
                thief_core_ctx->setStateType(eCoreReady);
                break;
        }
}


void PandosNodeT::checkCoreID(int line, const char *file, const char *function, int core_id) {
        if (core_id < 0 || core_id >= core_contexts.size()) {
                out->fatal(line, file, function, -1, "%s: bad core id = %d, num_cores = %d\n"
                           ,getName().c_str()
                           ,core_id
                           ,core_contexts.size());
                abort();
        }
}

void PandosNodeT::checkPXNID(int line, const char *file, const char *function, int pxn_id) {
        if (pxn_id != pando_context->id) {
                out->fatal(line, file, function, -1, "%s: bad pxn id = %d, this pxn's id = %d\n"
                           ,getName().c_str()
                           ,pxn_id
                           ,pando_context->id);
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
        using namespace pando;
        using namespace backend;
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
        cc0->setStateType(eCoreReady);
}

/**
 * configure a port
 */

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
        bool debug_scheduler = params.find<bool>("debug_scheduler", false, found);
        bool debug_memory_requests = params.find<bool>("debug_memory_requests", false, found);
        uint32_t verbose_mask = 0;
        if (debug_scheduler) verbose_mask |= DEBUG_SCHEDULE;
        if (debug_memory_requests) verbose_mask |= DEBUG_MEMORY_REQUESTS;
        
        out = new Output("[PandosNode] ", verbose_level, verbose_mask, Output::STDOUT);
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
        toCoreLocalSPM = configureLink(
                "toCoreLocalSPM",
                new Event::Handler<PandosNodeT, Link**>(this, &PandosNodeT::receiveRequest, &this->fromCoreLocalSPM));
        fromCoreLocalSPM = configureLink(
                "fromCoreLocalSPM",
                new Event::Handler<PandosNodeT, Link**>(this, &PandosNodeT::receiveResponse, &this->toCoreLocalSPM));
        // configure the nodeSharedDRAM link
        toNodeSharedDRAM = configureLink(
                "toNodeSharedDRAM",
                new Event::Handler<PandosNodeT, Link**>(this, &PandosNodeT::receiveRequest, &this->fromNodeSharedDRAM));
        fromNodeSharedDRAM = configureLink(
                "fromNodeSharedDRAM",
                new Event::Handler<PandosNodeT, Link**>(this, &PandosNodeT::receiveResponse, &this->toNodeSharedDRAM)
                );
        // configure the remoteNode link
        toRemoteNode = configureLink(
                "toRemoteNode",
                new Event::Handler<PandosNodeT, Link**>(this, &PandosNodeT::receiveResponse, &this->toRemoteNode)
                );
        fromRemoteNode = configureLink(
                "fromRemoteNode",
                new Event::Handler<PandosNodeT, Link**>(this, &PandosNodeT::receiveRequest, &this->fromRemoteNode)
                );

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

        // what type of request are we sending
        
        
        // create a new event
        // TODO: constructors here
        PandosRequestEventT *req = nullptr;
        if (core_ctx->core_state.type == eCoreIssueMemoryRead) {
                /* read request */
                PandosReadRequestEventT *read_req = new PandosReadRequestEventT;
                read_req->size = core_ctx->core_state.mem_req.size;
                out->verbose(CALL_INFO, 1, DEBUG_MEMORY_REQUESTS, "core %d: Sending memory request with size = %ld\n",
                             src_core, core_ctx->core_state.mem_req.size);
                core_ctx->setStateType(eCoreStallMemoryRead);
                req = read_req;
        } else if (core_ctx->core_state.type == eCoreIssueMemoryWrite) {
                /* write request */
                PandosWriteRequestEventT *write_req = new PandosWriteRequestEventT;
                write_req->size = core_ctx->core_state.mem_req.size;
                write_req->payload.reserve(write_req->size);
#ifdef DO_MEMCPY
                memcpy(write_req->payload.data(), core_ctx->core_state.mem_req.data, write_req->size);
#endif
                core_ctx->setStateType(eCoreStallMemoryWrite);
                req = write_req;
        } else {
                // should never happen
                out->fatal(CALL_INFO, -1, "%s: core %d: bad core state: %s\n"
                           ,__func__
                           ,src_core
                           ,core_state_type_string(core_ctx->core_state.type)
                        );
        }
        req->src_core = src_core;
        req->src_pxn = core_ctx->node_ctx->id;;
        req->dst = core_ctx->core_state.mem_req.addr;

        // destination?
        if (core_ctx->core_state.mem_req.addr.pxn != core_ctx->node_ctx->id) {
                /* remote request */
                toRemoteNode->send(req);
        } else if (core_ctx->core_state.mem_req.addr.dram_not_spm) {
                /* dram */
                out->verbose(CALL_INFO, 1, DEBUG_MEMORY_REQUESTS, "%s: Sending memory request to DRAM\n", __func__);
                toNodeSharedDRAM->send(req);
        } else {
                /* spm */
                out->verbose(CALL_INFO, 1, DEBUG_MEMORY_REQUESTS, "%s: Sending memory request to SPM\n", __func__);                
                toCoreLocalSPM->send(req);
        }
}

/**
 * handle a response from memory to a request
 */
void PandosNodeT::receiveResponse(SST::Event *evt, Link** requestLink) {
        using namespace pando;
        using namespace backend;
        out->verbose(CALL_INFO, 1, DEBUG_MEMORY_REQUESTS, "Received packet on link\n");
        PandosResponseEventT *rsp = dynamic_cast<PandosResponseEventT*>(evt);
        if (!rsp) {
                out->fatal(CALL_INFO, -1, "Bad event type\n");
                abort();
        }
        /**
         * map response back to core/pxn
         */
        int64_t src_core = rsp->src_core;
        int64_t src_pxn = rsp->src_pxn;
        checkCoreID(CALL_INFO, src_core);
        checkPXNID(CALL_INFO, src_pxn);

        core_context_t *core_ctx = core_contexts[src_core];

        // change core's state to ready
        core_ctx->setStateType(eCoreReady);

        // was this a read response?
        PandosReadResponseEventT *read_rsp = dynamic_cast<PandosReadResponseEventT*>(rsp);
        if (read_rsp) {
                void *read_val_p = malloc(read_rsp->size);
#ifdef DO_MEMCPY
                memcpy(read_val_p, read_rsp->payload.data(), read_rsp->size);
#endif
                core_ctx->core_state.mem_req.data = read_val_p;
        }

        // delete response
        delete evt;
}


/**
 * receive a write request
 */
void PandosNodeT::receiveWriteRequest(PandosWriteRequestEventT *write_req, Link **responseLink) {
        /* create a response packet */
        out->verbose(CALL_INFO, 1, DEBUG_MEMORY_REQUESTS, "%s: formatting a response packet\n", __func__);
        PandosWriteResponseEventT *write_rsp = new PandosWriteResponseEventT;
        write_rsp->src_pxn = write_req->src_pxn;
        write_rsp->src_core = write_req->src_core;
        void *p = reinterpret_cast<void*>(write_req->dst.uptr);
        out->verbose(CALL_INFO, 1, DEBUG_MEMORY_REQUESTS, "%s: memcpying %zu bytes to %p\n", __func__, write_req->size, p);
#ifdef DO_MEMCPY
        memcpy(p, write_req->payload.data(), write_req->size);
#endif
        /* delete the request */
        delete write_req;
        /* send the response */
        out->verbose(CALL_INFO, 1, DEBUG_MEMORY_REQUESTS, "Sending write response\n");
        (*responseLink)->send(write_rsp);
        out->verbose(CALL_INFO, 1, DEBUG_MEMORY_REQUESTS, "Write response sent\n");
}

/**
 * receive a read request
 */
void PandosNodeT::receiveReadRequest(PandosReadRequestEventT *read_req, Link **responseLink) {
        /* create a response packet */
        out->verbose(CALL_INFO, 1, DEBUG_MEMORY_REQUESTS, "%s: formatting response packet\n", __func__);
        PandosReadResponseEventT *read_rsp = new PandosReadResponseEventT;
        read_rsp->src_pxn = read_req->src_pxn;
        read_rsp->src_core = read_req->src_core;
        read_rsp->size = read_req->size;
        read_rsp->payload.reserve(read_req->size);
        void *p = reinterpret_cast<void*>(read_req->dst.uptr);
#ifdef DO_MEMCPY
        memcpy(read_rsp->payload.data(), p, read_req->size);
#endif
        /* delete the request event */
        delete read_req;
        /* send the response */
        out->verbose(CALL_INFO, 1, DEBUG_MEMORY_REQUESTS, "Sending read response\n");
        (*responseLink)->send(read_rsp);
}

/**
 * handle a request for memory operation
 */
void PandosNodeT::receiveRequest(SST::Event *evt, Link **responseLink) {
        out->verbose(CALL_INFO, 1, DEBUG_MEMORY_REQUESTS, "Received packet on link\n");
        PandosReadRequestEventT *read_req;
        PandosWriteRequestEventT *write_req;
        read_req = dynamic_cast<PandosReadRequestEventT*>(evt);
        if (!read_req) {
                write_req = dynamic_cast<PandosWriteRequestEventT*>(evt);
                if (!write_req) {
                        out->fatal(CALL_INFO, -1, "Bad event type\n");
                        abort();
                }
                // write request
                out->verbose(CALL_INFO, 1, DEBUG_MEMORY_REQUESTS, "Received write packet\n");
                receiveWriteRequest(write_req, responseLink);
        } else {
                // read request
                out->verbose(CALL_INFO, 1, DEBUG_MEMORY_REQUESTS, "Received read packet\n");
                receiveReadRequest(read_req, responseLink);                
        }
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
                switch (core->core_state.type) {
                case eCoreIssueMemoryRead:
                case eCoreIssueMemoryWrite:
                        // generate an event an depending on the memory type
                        sendMemoryRequest(core_id);
                        break;
                case eCoreIdle:
                        out->verbose(CALL_INFO, 2, DEBUG_SCHEDULE, "core %d is idle\n", core_id);                        
                        schedule(core_id);
                case eCoreReady:
                        core->execute();
                        break;
                case eCoreStallMemoryRead:
                case eCoreStallMemoryWrite:
                default:
                        break;
                }                        
        }
        return false;
}

