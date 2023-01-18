#include "sst_config.h"
#include "PandosNode.h"

using namespace SST;
using namespace SST::PandosProgramming;

/**
 * Constructors/Destructors
 */
PandosNodeT::PandosNodeT(ComponentId_t id, Params &params) : Component(id) {
    out = new Output("[PandosNode]", 1, 0, Output::STDOUT);
    out->output(CALL_INFO, "Hello, world!\n");
    // Read parameters
    // Configure link
    // Register clock
}

PandosNodeT::~PandosNodeT() {
    out->output(CALL_INFO, "Goodbye, cruel world!\n");
    delete out;
}

/**
 * Handles event when it arrives on the link
 */
void PandosNodeT::handleEvent(SST::Event *ev) {
}


/**
 * Handle a clockTic
 *
 * return true if the clock handler should be disabeld, false o/w
 */
bool PandosNodeT::clockTic(SST::Cycle_t cycle) {
    return true;
}


