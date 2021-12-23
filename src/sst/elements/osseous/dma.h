#include "DMATop_O1.h"
#include <queue>
#include <stdint.h>
#include <cstring>
#define FATAL                                                \
  do                                                         \
  {                                                          \
    fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
            __LINE__, __FILE__, errno, strerror(errno));     \
    exit(1);                                                 \
  } while (0)

struct data_packet
{
  //every data packet has an ID and a Last bit!
  uint64_t id;
  //data to be transmitted through DMA
  std::vector<char> data;
  //last word specifier
  bool last;
  //packet consisting of the ID, Data and the last word in the burst mode
  data_packet(uint64_t id, std::vector<char> data, bool last)
  {
    this->id = id;
    this->data = data;
    this->last = last;
  }
  data_packet()
  {
    this->id = 0;
    this->last = false;
  }
};

class dma_interface
{
private:
  //data to be transferred in the burst mode
  char    *data;
  //size of the data to be transferred
  size_t   size;
  size_t   word_size;
  bool     store_inflight;
  uint64_t store_addr;
  uint64_t store_id;
  uint64_t store_size;
  uint64_t store_count;

  //Write response. This signal indicates the status of the write transaction
  std::queue<uint64_t> bresp;
  //Read response. This signal indicates the status of the read transfer.
  std::queue<data_packet> rresp;
  //internal buffers to the handler!
  std::vector<char> dummy_data;
  
  //bus response queue

  //read response queue
  //std::queue<dma_interface> rresp;
  //identifies the cycle time
  uint64_t cycle;

public:
  //magic memory instance
  dma_interface(size_t size, size_t word_size);
  ~dma_interface();

  //initialize the memory used for the signals from the testbench
  //interface functions
  void init(size_t sz, int word_size);
  char *get_data()
  { 
    return data; 
  }
  size_t get_size() 
  { 
    return size; 
  }
  bool ar_ready() 
  { 
    return true; 
  }
  bool aw_ready() 
  { 
    return !store_inflight;
  }
  bool w_ready() 
  { 
    return store_inflight; 
  }
  bool b_valid() 
  { 
    return !bresp.empty(); 
  }
  uint64_t b_resp() 
  { 
    return 0; 
  }
  uint64_t b_id() 
  { 
    return b_valid() ? bresp.front() : 0; 
  }
  bool r_valid() 
  { 
    return !rresp.empty(); 
  }
  uint64_t r_resp() 
  { 
    return 0; 
  }
  uint64_t r_id() 
  { 
    return r_valid() ? rresp.front().id : 0; 
  }
  void *r_data() 
  { 
    return r_valid() ? &rresp.front().data[0] : &dummy_data[0]; 
  }
  bool r_last() 
  { 
    return r_valid() ? rresp.front().last : false; 
  }

};

DMATop *top;                //DMA controller instance
AXI4LiteCSR *axi4lite;      //AXI interface instance, CSR
AXI4Writer *axi4writer;     //writer front end
AXIStreamSlave *axi4reader; //reader front end

enum dma_state
{
  sIdle,
  sReadAddr,
  sReadData,
  sWriteAddr,
  sWriteData,
  sWriteResp
};
enum axi_writer_data_state
{
  sDataIdle,
  sDataTransfer,
  sDataResp,
  sDataDone
};
enum axi_writer_addr_state
{
  sAddrTransfer,
  sAddrDone,
  sAddrIdle
};

std::queue<char> cmd_queue;

int dma_write_req(unsigned int *, unsigned int *);
void mem_print(void *, int);

/*
  UInt<32> io_control_aw_awaddr;
  UInt<32> io_control_w_wdata;
  UInt<32> io_control_ar_araddr;
  UInt<32> io_control_r_rdata;
  UInt<32> io_read_tdata;
  UInt<32> io_write_aw_awaddr;
  UInt<32> io_write_w_wdata;
  UInt<32> io_write_ar_araddr;
  UInt<32> io_write_r_rdata;
*/