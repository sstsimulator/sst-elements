#pragma once
#include <pando/backend_node_context.hpp>
#include <pando/backend_core_context.hpp>
#include <pando/arch_coroutine.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <vector>
#include <cstdint>

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
        )
        // Document the ports that this component accepts
        SST_ELI_DOCUMENT_PORTS(
                {"port",  "Link to another component", { "PandosProgramming.PandosEvent", ""} }
        )

        /**
         * Constructors/Destructors
         */
        PandosNodeT(ComponentId_t id, Params &params);
        ~PandosNodeT();

        /**
         * Event handler which we register to a link
         */
        void handleEvent(SST::Event *ev);

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
        
        // SST Output object, for printing error messages, etc.
        SST::Output *out;

        // Links to other nodes
        SST::Link* port;

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
