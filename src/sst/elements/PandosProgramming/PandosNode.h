#pragma once
#include <pando/backend_node_context.hpp>
#include <pando/backend_core_context.hpp>
#include <pando/arch_coroutine.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <PandosPacketEvent.h>
#include <vector>
#include <cstdint>
#include <string>

namespace SST {
namespace PandosProgramming {

typedef pando::backend::node_context_t* (*getContextFunc_t)();
typedef void  (*setContextFunc_t)(pando::backend::node_context_t*);
typedef int (*mainFunc_t)(int, char **);

/**
 * PANDOS Node component
 */
class PandosNodeT : public SST::Component {
public:
        // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
        SST_ELI_REGISTER_COMPONENT(
                PandosNodeT,                    // class name of the Component
                "PandosProgramming",            // component library (for Python/library lookup)
                "PandosNodeT",                  // Component name (for Python/library lookup)
                SST_ELI_ELEMENT_VERSION(0,0,0), // Version of the component
                "Pandos Node",                  // Description
                COMPONENT_CATEGORY_PROCESSOR    // Category
        )
        // Document the parameters that this component accepts
        SST_ELI_DOCUMENT_PARAMS(
                {"num_cores", "Number of cores on PandoNode", NULL},
                {"instructions_per_task", "Instructions per task", NULL},
                {"program_binary_fname", "Program binary file name", NULL},
                {"verbose_level", "Verbosity of logging", NULL},
                {"debug_scheduler", "Debug scheduler", NULL},
                {"debug_memory_requests", "Debug memory requests", NULL},
                {"debug_initialization", "Debug initializtion code", NULL},                
        )
        // Document the ports that this component accepts
        SST_ELI_DOCUMENT_PORTS(
                // self-links to represent latency from node to spm
                {"toCoreLocalSPM"         ,"Link from node to SPM"             ,{"PandosProgramming.PandosMemoryRequestEventT",""}},
                {"fromCoreLocalSPM"       ,"Link from SPM to node"             ,{"PandosProgramming.PandosMemoryResponseEventT",""}},
                // self-links to represent latency from node to dram
                {"toNodeSharedDRAM"       ,"Link from node to DRAM"            ,{"PandosProgramming.PandosMemoryRequestEventT",""}},
                {"fromNodeSharedDRAM"     ,"Link from DRAM to Node"            ,{"PandosProgramming.PandosMemoryResponseEventT",""}},
                // links to a remote node component
                {"requestsToRemoteNode"   ,"Link from this Node to other Node" ,{"PandosProgramming.PandosMemoryResponseEventT",""}},
                {"requestsFromRemoteNode" ,"Link from other Node to this Node" ,{"PandosProgramming.PandosMemoryRequestEventT",""}},
         )

        /**
         * Constructors/Destructors
         */
        PandosNodeT(ComponentId_t id, Params &params);
        ~PandosNodeT();

        /**
         * Clock tick handler
         */
        virtual bool clockTic(SST::Cycle_t);

        /**
         * Open a user program
         */
        void openProgramBinary();
        /**
         * Close a user program
         */
        void closeProgramBinary();

        /**
         * initalize cores
         */
        void initCores();

        /**
         * send memory request on behalf of a core
         */
        void sendMemoryRequest(int src_core);

        /**
         * handle a response from memory to a request
         */
        void receiveResponse(SST::Event *rsp, Link **requestLink);

        /**
         * handle a write request
         */
        void receiveWriteRequest(PandosWriteRequestEventT *write_req, Link **responseLink);

        /**
         * handle a read request
         */
        void receiveReadRequest(PandosReadRequestEventT *read_req, Link **responseLink);
                
        /**
         * handle a request for memory operation
         */
        void receiveRequest(SST::Event *req, Link **responseLink);

        /**
         * check if core id is valid, abort() if not
         */
        void checkCoreID(int line, const char *file, const char *function, int core_id);

        /**
         * check if pxn id is valid, abort() if not
         */
        void checkPXNID(int line, const char *file, const char *function, int pxn_id);

        /**
         * schedule work onto a core
         */
        void schedule(int core_id);
        
        // SST Output object, for printing error messages, etc.
        SST::Output *out;

        // Links memories

        /*
         * Core <=> SPM
         */
        Link *toCoreLocalSPM;
        Link *fromCoreLocalSPM;
        
        /*
         * Core <=> DRAM
         */
        Link *toNodeSharedDRAM;
        Link *fromNodeSharedDRAM;
        
        /*
         * Core <=> REMOTE
         */
        Link *toRemoteNode;
        Link *fromRemoteNode;

        int32_t num_cores; //!< The number of cores in this node
        int32_t instr_per_task; //!< The number of instructions per task
        std::string program_binary_fname; //!< The name of the program binary to load
        void *program_binary_handle;
        getContextFunc_t get_current_pando_ctx;
        setContextFunc_t set_current_pando_ctx;
        pando::backend::node_context_t *pando_context; //!< PANDO context
        std::vector<pando::backend::core_context_t*> core_contexts; //!< PANDO cores
};

}
}
