#include "sst_config.h"
#include "PandosNode.h"

using namespace SST;
using namespace SST::PandosProgramming;

/**
 * Constructors/Destructors
 */
PandosNodeT::PandosNodeT(ComponentId_t id, Params &params) : Component(id) {
    // Read parameters
    // Configure link
    // Register clock
}

PandosNodeT::~PandosNodeT() {
    // delete out
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
bool clockTic(SST::Cycle_t cycle) {
    return false;
}


