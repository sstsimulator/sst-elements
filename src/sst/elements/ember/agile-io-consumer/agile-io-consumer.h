#ifndef AGILE_IO_CONSUMER_H_
#define AGILE_IO_CONSUMER_H_

#include "mpi/embermpigen.h"
#include "sst/elements/hermes/msgapi.h"
#include <codecvt>

namespace SST::Ember {

const long combined_read_size = 10*1024*1024;

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


  SST_ELI_DOCUMENT_PARAMS(
      {"arg.IONodes", "Array of IO nodes", "0"}
  )

  agileIOconsumer(SST::ComponentId_t id, Params& prms);
  ~agileIOconsumer() override = default;

  void setup() override;

  bool generate(std::queue<EmberEvent*>& evQ) override;
  void read_request();
  int  write_data();
  int  num_io_nodes();

  // Sent to all the IO nodes
  void validate(const long total_request_size);
  void broadcast_and_receive(const long &total_request_size, std::queue<EmberEvent *> &evQ);
  void blue_request(long total_request_size);

  // Each IO node responds with amount of data read
  long green_read();

  private:

  Hermes::MP::Addr sendBuf;
  std::vector<Hermes::MP::Addr> recvBuf;
  std::queue<EmberEvent*>* evQ_;
  uint64_t rank_;
  std::vector<int> ionodes;
  // Whether the node is IO (green) or consumer (blue)
  enum Kind { Green, Blue };
  Kind kind;
};

}

#endif /* AGILE_IO_CONSUMER_H_ */
