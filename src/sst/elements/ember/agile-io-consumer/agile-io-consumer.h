#ifndef AGILE_IO_CONSUMER_H_
#define AGILE_IO_CONSUMER_H_

#include "mpi/embermpigen.h"
#include "sst/elements/hermes/hermes.h"
#include "sst/elements/hermes/msgapi.h"
#include <codecvt>
#include <cstddef>
#include <cstdint>

namespace SST::Ember {

const long combined_read_size = 10*1024*1024;

struct PacketHeader {
    uint64_t src;
    uint64_t dst;
    uint64_t len;
};

const int magicNumber = (1 << 0) + (1 << 4) + (1 << 8) + (1 << 12) + (1 << 1);

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
  bool broadcast_and_receive(const long &total_request_size, std::queue<EmberEvent *> &evQ);
  bool blue_request(long total_request_size);

  // Each IO node responds with amount of data read
  bool green_read();

  private:
  unsigned count;
  long iteration;
  static int memory_bitmask;

  Hermes::MemAddr blue_sendBuf = nullptr;
  Hermes::MemAddr *blue_recvBuf = nullptr;
  Hermes::MemAddr green_sendBuf = nullptr;
  Hermes::MemAddr green_recvBuf = nullptr;

  std::queue<EmberEvent*>* evQ_;
  uint64_t rank_;
  std::vector<int> ionodes;
  // Whether the node is IO (green) or consumer (blue)
  enum Kind { Green, Blue };
  Kind kind;
};

}

#endif /* AGILE_IO_CONSUMER_H_ */
