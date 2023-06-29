#ifndef AGILE_IO_CONSUMER_H_
#define AGILE_IO_CONSUMER_H_

#include "mpi/embermpigen.h"

namespace SST {

namespace Ember {

class agileIOconsumer : public EmberMessagePassingGenerator
{
    public:
  SST_ELI_REGISTER_SUBCOMPONENT(
      agileIOconsumer,
      "ember",
      "IOConsumerMotif",
      SST_ELI_ELEMENT_VERSION(1,0,0),
      "Performs a IOConsumer Motif",
      SST::Ember::EmberGenerator
  )

  agileIOconsumer(SST::ComponentId_t id, Params& prms);
  ~agileIOconsumer() {}

  bool generate(std::queue<EmberEvent*>& evQ);
  void read_request();
  int  write_data();
  int  num_io_nodes();
};

}

}

#endif /* AGILE_IO_CONSUMER_H_ */
