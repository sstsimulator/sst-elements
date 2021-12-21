#ifndef DMATOP_H_
#define DMATOP_H_

//not originally included in the headerfile 
enum DEBUG_FLAGS{d_control, d_data, d_addr, d_reg};  
DEBUG_FLAGS debug_flag;
//void print_registers(DEBUG_FLAGS);


#include <array>
#include <cstdint>
#include <cstdlib>
#include "uint.h"
#include "sint.h"
#define UNLIKELY(condition) __builtin_expect(static_cast<bool>(condition), 0)

typedef struct AXI4LiteCSR {
  UInt<3> state;
  UInt<1> awready;
  UInt<1> wready;
  UInt<1> bvalid;
  UInt<1> arready;
  UInt<1> rvalid;
  UInt<32> addr;

  AXI4LiteCSR() {
    state.rand_init();
    awready.rand_init();
    wready.rand_init();
    bvalid.rand_init();
    arready.rand_init();
    rvalid.rand_init();
    addr.rand_init();
  }
} AXI4LiteCSR;

typedef struct AXIStreamSlave {
  UInt<2> state;
  UInt<1> done;
  UInt<1> enable;
  UInt<32> length;

  AXIStreamSlave() {
    state.rand_init();
    done.rand_init();
    enable.rand_init();
    length.rand_init();
  }
} AXIStreamSlave;

typedef struct AXI4Writer {
  UInt<2> dataState;
  UInt<2> addrState;
  UInt<1> done;
  UInt<1> enable;
  UInt<32> length;
  UInt<32> awlen;
  UInt<32> awaddr;
  UInt<1> awvalid;
  UInt<1> bready;

  AXI4Writer() {
    dataState.rand_init();
    addrState.rand_init();
    done.rand_init();
    enable.rand_init();
    length.rand_init();
    awlen.rand_init();
    awaddr.rand_init();
    awvalid.rand_init();
    bready.rand_init();
  }
} AXI4Writer;

typedef struct CSR {

  CSR() {
  }
} CSR;

typedef struct AddressGenerator {
  UInt<2> state;
  UInt<32> lineCount;
  UInt<32> lineGap;
  UInt<32> address_o;
  UInt<32> address_i;
  UInt<32> length_o;
  UInt<32> length_i;
  UInt<1> valid;
  UInt<1> busy;

  AddressGenerator() {
    state.rand_init();
    lineCount.rand_init();
    lineGap.rand_init();
    address_o.rand_init();
    address_i.rand_init();
    length_o.rand_init();
    length_i.rand_init();
    valid.rand_init();
    busy.rand_init();
  }
} AddressGenerator;

typedef struct TransferSplitter {

  TransferSplitter() {
  }
} TransferSplitter;

typedef struct TransferSplitter_1 {
  UInt<32> _T_42;
  UInt<32> _T_45;
  UInt<32> _T_48;
  UInt<32> _T_51;
  UInt<1> _T_60;
  UInt<1> _T_63;
  UInt<2> _T_65;

  TransferSplitter_1() {
    _T_42.rand_init();
    _T_45.rand_init();
    _T_48.rand_init();
    _T_51.rand_init();
    _T_60.rand_init();
    _T_63.rand_init();
    _T_65.rand_init();
  }
} TransferSplitter_1;

typedef struct ClearCSR {
  UInt<32> reg;

  ClearCSR() {
    reg.rand_init();
  }
} ClearCSR;

typedef struct StatusCSR {
  UInt<32> reg;

  StatusCSR() {
    reg.rand_init();
  }
} StatusCSR;

typedef struct SimpleCSR {
  UInt<32> reg;

  SimpleCSR() {
    reg.rand_init();
  }
} SimpleCSR;

typedef struct SetCSR {
  UInt<32> reg;

  SetCSR() {
    reg.rand_init();
  }
} SetCSR;

typedef struct InterruptController {
  UInt<1> readBusy;
  UInt<1> readBusyOld;
  UInt<1> writeBusy;
  UInt<1> writeBusyOld;
  UInt<1> writeBusyIrq;
  UInt<1> readBusyIrq;
  SimpleCSR SimpleCSR$$inst;
  SetCSR SetCSR$$inst;

  InterruptController() {
    readBusy.rand_init();
    readBusyOld.rand_init();
    writeBusy.rand_init();
    writeBusyOld.rand_init();
    writeBusyIrq.rand_init();
    readBusyIrq.rand_init();
  }
} InterruptController;

typedef struct WorkerCSRWrapper {
  UInt<2> status;
  UInt<1> readerSync;
  UInt<1> readerSyncOld;
  UInt<1> writerSync;
  UInt<1> writerSyncOld;
  UInt<1> readerStart;
  UInt<1> writerStart;
  AddressGenerator addressGeneratorRead;
  TransferSplitter transferSplitterRead;
  AddressGenerator addressGeneratorWrite;
  TransferSplitter_1 transferSplitterWrite;
  ClearCSR ClearCSR$$inst;
  StatusCSR StatusCSR$$inst;
  InterruptController InterruptController$$inst;
  SimpleCSR SimpleCSR$$inst;
  SimpleCSR SimpleCSR_1;
  SimpleCSR SimpleCSR_2;
  SimpleCSR SimpleCSR_3;
  SimpleCSR SimpleCSR_4;
  SimpleCSR SimpleCSR_5;
  SimpleCSR SimpleCSR_6;
  SimpleCSR SimpleCSR_7;
  SimpleCSR SimpleCSR_8;
  SimpleCSR SimpleCSR_9;
  SimpleCSR SimpleCSR_10;
  SimpleCSR SimpleCSR_11;

  WorkerCSRWrapper() {
    status.rand_init();
    readerSync.rand_init();
    readerSyncOld.rand_init();
    writerSync.rand_init();
    writerSyncOld.rand_init();
    readerStart.rand_init();
    writerStart.rand_init();
  }
} WorkerCSRWrapper;

typedef struct Queue {
  UInt<9> value;
  UInt<9> value_1;
  UInt<1> maybe_full;
  UInt<32> ram[512];

  Queue() {
    value.rand_init();
    value_1.rand_init();
    maybe_full.rand_init();
    for (size_t a=0; a < 512; a++) ram[a].rand_init();
  }
} Queue;

typedef struct AXITop {
  UInt<1> clock;
  UInt<1> reset;
  UInt<32> io_control_aw_awaddr;
  UInt<3> io_control_aw_awprot;
  UInt<1> io_control_aw_awvalid;
  UInt<1> io_control_aw_awready;
  UInt<32> io_control_w_wdata;
  UInt<4> io_control_w_wstrb;
  UInt<1> io_control_w_wvalid;
  UInt<1> io_control_w_wready;
  UInt<2> io_control_b_bresp;
  UInt<1> io_control_b_bvalid;
  UInt<1> io_control_b_bready;
  UInt<32> io_control_ar_araddr;
  UInt<3> io_control_ar_arprot;
  UInt<1> io_control_ar_arvalid;
  UInt<1> io_control_ar_arready;
  UInt<32> io_control_r_rdata;
  UInt<2> io_control_r_rresp;
  UInt<1> io_control_r_rvalid;
  UInt<1> io_control_r_rready;
  UInt<32> io_read_tdata;
  //================================= i would have to use these pins ============
  UInt<1> io_read_tvalid;
  UInt<1> io_read_tready;
  UInt<1> io_read_tuser;
  UInt<1> io_read_tlast;
  //================================= i would have to use these pins ============
  UInt<4>  io_write_aw_awid;
  UInt<32> io_write_aw_awaddr;
  UInt<8>  io_write_aw_awlen;
  UInt<3>  io_write_aw_awsize;
  UInt<2>  io_write_aw_awburst;
  UInt<1>  io_write_aw_awlock;
  UInt<4>  io_write_aw_awcache;
  UInt<3>  io_write_aw_awprot;
  UInt<4>  io_write_aw_awqos;
  UInt<1>  io_write_aw_awvalid;
  UInt<1>  io_write_aw_awready;
  UInt<32> io_write_w_wdata;
  UInt<4>  io_write_w_wstrb;
  UInt<1>  io_write_w_wlast;
  UInt<1>  io_write_w_wvalid;
  UInt<1>  io_write_w_wready;
  UInt<4>  io_write_b_bid;
  UInt<2>  io_write_b_bresp;
  UInt<1>  io_write_b_bvalid;
  UInt<1>  io_write_b_bready;
  UInt<4>  io_write_ar_arid;
  UInt<32> io_write_ar_araddr;
  UInt<8>  io_write_ar_arlen;
  UInt<3>  io_write_ar_arsize;
  UInt<2>  io_write_ar_arburst;
  UInt<1>  io_write_ar_arlock;
  UInt<4>  io_write_ar_arcache;
  UInt<3>  io_write_ar_arprot;
  UInt<4>  io_write_ar_arqos;
  UInt<1>  io_write_ar_arvalid;
  UInt<1>  io_write_ar_arready;
  UInt<4>  io_write_r_rid;
  UInt<32> io_write_r_rdata;
  UInt<2> io_write_r_rresp;
  UInt<1> io_write_r_rlast;
  UInt<1> io_write_r_rvalid;
  UInt<1> io_write_r_rready;
  UInt<1> io_irq_readerDone;
  UInt<1> io_irq_writerDone;
  UInt<1> io_sync_readerSync;
  UInt<1> io_sync_writerSync;
  AXI4LiteCSR csrFrontend;
  AXIStreamSlave readerFrontend;
  AXI4Writer writerFrontend;
  CSR csr;
  WorkerCSRWrapper ctl;
  Queue queue;

  AXITop() {
    reset.rand_init();
    io_control_aw_awaddr.rand_init();
    io_control_aw_awprot.rand_init();
    io_control_aw_awvalid.rand_init();
    io_control_aw_awready.rand_init();
    io_control_w_wdata.rand_init();
    io_control_w_wstrb.rand_init();
    io_control_w_wvalid.rand_init();
    io_control_w_wready.rand_init();
    io_control_b_bresp.rand_init();
    io_control_b_bvalid.rand_init();
    io_control_b_bready.rand_init();
    io_control_ar_araddr.rand_init();
    io_control_ar_arprot.rand_init();
    io_control_ar_arvalid.rand_init();
    io_control_ar_arready.rand_init();
    io_control_r_rdata.rand_init();
    io_control_r_rresp.rand_init();
    io_control_r_rvalid.rand_init();
    io_control_r_rready.rand_init();
    io_read_tdata.rand_init();
    io_read_tvalid.rand_init();
    io_read_tready.rand_init();
    io_read_tuser.rand_init();
    io_read_tlast.rand_init();
    io_write_aw_awid.rand_init();
    io_write_aw_awaddr.rand_init();
    io_write_aw_awlen.rand_init();
    io_write_aw_awsize.rand_init();
    io_write_aw_awburst.rand_init();
    io_write_aw_awlock.rand_init();
    io_write_aw_awcache.rand_init();
    io_write_aw_awprot.rand_init();
    io_write_aw_awqos.rand_init();
    io_write_aw_awvalid.rand_init();
    io_write_aw_awready.rand_init();
    io_write_w_wdata.rand_init();
    io_write_w_wstrb.rand_init();
    io_write_w_wlast.rand_init();
    io_write_w_wvalid.rand_init();
    io_write_w_wready.rand_init();
    io_write_b_bid.rand_init();
    io_write_b_bresp.rand_init();
    io_write_b_bvalid.rand_init();
    io_write_b_bready.rand_init();
    io_write_ar_arid.rand_init();
    io_write_ar_araddr.rand_init();
    io_write_ar_arlen.rand_init();
    io_write_ar_arsize.rand_init();
    io_write_ar_arburst.rand_init();
    io_write_ar_arlock.rand_init();
    io_write_ar_arcache.rand_init();
    io_write_ar_arprot.rand_init();
    io_write_ar_arqos.rand_init();
    io_write_ar_arvalid.rand_init();
    io_write_ar_arready.rand_init();
    io_write_r_rid.rand_init();
    io_write_r_rdata.rand_init();
    io_write_r_rresp.rand_init();
    io_write_r_rlast.rand_init();
    io_write_r_rvalid.rand_init();
    io_write_r_rready.rand_init();
    io_irq_readerDone.rand_init();
    io_irq_writerDone.rand_init();
    io_sync_readerSync.rand_init();
    io_sync_writerSync.rand_init();
  }

  void eval(bool update_registers, bool verbose, bool done_reset) {

    //std::cout<<reset<<std::endl;
    //std::cout<<"io_control_aw_awready "<<io_control_aw_awready <<std::endl;
    //std::cout<<"io_control_w_wready   "<<io_control_w_wready   <<std::endl;
    //std::cout<<"io_control_b_bresp    "<<io_control_b_bresp    <<std::endl;
    //std::cout<<"io_control_b_bvalid   "<<io_control_b_bvalid   <<std::endl;
    //std::cout<<"io_control_ar_arready "<<io_control_ar_arready <<std::endl;    
    
    io_control_aw_awready = csrFrontend.awready;
    io_control_w_wready   = csrFrontend.wready;
    io_control_b_bresp    = UInt<2>(0x0);
    io_control_b_bvalid   = csrFrontend.bvalid;
    io_control_ar_arready = csrFrontend.arready;
    fprintf(stderr, "\nAXI_Port Eval Called");

    UInt<4> csrFrontend$io_bus_addr = csrFrontend.addr.bits<3,0>();
    UInt<1> csr$_T_343 = csrFrontend$io_bus_addr == UInt<4>(0xf);
    UInt<1> csrFrontend$io_bus_read = io_control_r_rready & csrFrontend.rvalid;
    UInt<1> csr$_T_344 = csr$_T_343 & csrFrontend$io_bus_read;
    UInt<1> csr$_T_332 = csrFrontend$io_bus_addr == UInt<4>(0xe);
    UInt<1> csr$_T_333 = csr$_T_332 & csrFrontend$io_bus_read;
    UInt<1> csr$_T_321 = csrFrontend$io_bus_addr == UInt<4>(0xd);
    UInt<1> csr$_T_322 = csr$_T_321 & csrFrontend$io_bus_read;
    UInt<1> csr$_T_310 = csrFrontend$io_bus_addr == UInt<4>(0xc);
    UInt<1> csr$_T_311 = csr$_T_310 & csrFrontend$io_bus_read;
    UInt<1> csr$_T_299 = csrFrontend$io_bus_addr == UInt<4>(0xb);
    UInt<1> csr$_T_300 = csr$_T_299 & csrFrontend$io_bus_read;
    UInt<1> csr$_T_288 = csrFrontend$io_bus_addr == UInt<4>(0xa);
    UInt<1> csr$_T_289 = csr$_T_288 & csrFrontend$io_bus_read;
    UInt<1> csr$_T_277 = csrFrontend$io_bus_addr == UInt<4>(0x9);
    UInt<1> csr$_T_278 = csr$_T_277 & csrFrontend$io_bus_read;
    UInt<1> csr$_T_266 = csrFrontend$io_bus_addr == UInt<4>(0x8);
    UInt<1> csr$_T_267 = csr$_T_266 & csrFrontend$io_bus_read;
    UInt<1> csr$_T_255 = csrFrontend$io_bus_addr == UInt<4>(0x7);
    UInt<1> csr$_T_256 = csr$_T_255 & csrFrontend$io_bus_read;
    UInt<1> csr$_T_244 = csrFrontend$io_bus_addr == UInt<4>(0x6);
    UInt<1> csr$_T_245 = csr$_T_244 & csrFrontend$io_bus_read;
    UInt<1> csr$_T_233 = csrFrontend$io_bus_addr == UInt<4>(0x5);
    UInt<1> csr$_T_234 = csr$_T_233 & csrFrontend$io_bus_read;
    UInt<1> csr$_T_222 = csrFrontend$io_bus_addr == UInt<4>(0x4);
    UInt<1> csr$_T_223 = csr$_T_222 & csrFrontend$io_bus_read;
    UInt<1> csr$_T_211 = csrFrontend$io_bus_addr == UInt<4>(0x3);
    UInt<1> csr$_T_212 = csr$_T_211 & csrFrontend$io_bus_read;
    UInt<1> csr$_T_200 = csrFrontend$io_bus_addr == UInt<4>(0x2);
    UInt<1> csr$_T_201 = csr$_T_200 & csrFrontend$io_bus_read;
    UInt<1> csr$_T_189 = csrFrontend$io_bus_addr == UInt<4>(0x1);
    UInt<1> csr$_T_190 = csr$_T_189 & csrFrontend$io_bus_read;
    UInt<1> csr$_T_178 = csrFrontend$io_bus_addr == UInt<4>(0x0);
    UInt<1> csr$_T_179 = csr$_T_178 & csrFrontend$io_bus_read;
    UInt<32> csr$_GEN_0 = csr$_T_179 ? ctl.ClearCSR$$inst.reg : UInt<32>(0x0);
    UInt<32> csr$_GEN_4 = csr$_T_190 ? ctl.StatusCSR$$inst.reg : csr$_GEN_0;
    UInt<32> csr$_GEN_8 = csr$_T_201 ? ctl.InterruptController$$inst.SimpleCSR$$inst.reg : csr$_GEN_4;
    UInt<32> csr$_GEN_12 = csr$_T_212 ? ctl.InterruptController$$inst.SetCSR$$inst.reg : csr$_GEN_8;
    UInt<32> csr$_GEN_16 = csr$_T_223 ? ctl.SimpleCSR$$inst.reg : csr$_GEN_12;
    UInt<32> csr$_GEN_20 = csr$_T_234 ? ctl.SimpleCSR_1.reg : csr$_GEN_16;
    UInt<32> csr$_GEN_24 = csr$_T_245 ? ctl.SimpleCSR_2.reg : csr$_GEN_20;
    UInt<32> csr$_GEN_28 = csr$_T_256 ? ctl.SimpleCSR_3.reg : csr$_GEN_24;
    UInt<32> csr$_GEN_32 = csr$_T_267 ? ctl.SimpleCSR_4.reg : csr$_GEN_28;
    UInt<32> csr$_GEN_36 = csr$_T_278 ? ctl.SimpleCSR_5.reg : csr$_GEN_32;
    UInt<32> csr$_GEN_40 = csr$_T_289 ? ctl.SimpleCSR_6.reg : csr$_GEN_36;
    UInt<32> csr$_GEN_44 = csr$_T_300 ? ctl.SimpleCSR_7.reg : csr$_GEN_40;
    UInt<32> csr$_GEN_48 = csr$_T_311 ? ctl.SimpleCSR_8.reg : csr$_GEN_44;
    UInt<32> csr$_GEN_52 = csr$_T_322 ? ctl.SimpleCSR_9.reg : csr$_GEN_48;
    UInt<32> csr$_GEN_56 = csr$_T_333 ? ctl.SimpleCSR_10.reg : csr$_GEN_52;
    UInt<32> csr$io_bus_dataIn = csr$_T_344 ? ctl.SimpleCSR_11.reg : csr$_GEN_56;
    io_control_r_rdata = csr$io_bus_dataIn;    
    io_control_r_rresp = UInt<2>(0x0);
    io_control_r_rvalid = csrFrontend.rvalid;
    UInt<1> queue$_T_41 = queue.value == queue.value_1;
    UInt<1> queue$_T_44 = queue$_T_41 & queue.maybe_full;
    UInt<1> queue$io_enq_ready = ~queue$_T_44;
    UInt<1> readerFrontend$io_bus_tready = queue$io_enq_ready & readerFrontend.enable;
    io_read_tready = readerFrontend$io_bus_tready;


    io_write_aw_awid = UInt<4>(0x0);    
    
    io_write_aw_awaddr = writerFrontend.awaddr;
    print_registers();
    std::cout<<"io_write_aw_awaddr "<<io_write_aw_awaddr <<std::endl;
    UInt<8> writerFrontend$io_bus_aw_awlen = writerFrontend.awlen.bits<7,0>();
  
  
  //================================problem is here==================================
  
    io_write_aw_awlen   = writerFrontend$io_bus_aw_awlen;
    io_write_aw_awsize  = UInt<3>(0x2);
    io_write_aw_awburst = UInt<2>(0x1);
    io_write_aw_awlock  = UInt<1>(0x0);
    io_write_aw_awcache = UInt<4>(0x2);
    io_write_aw_awprot  = UInt<3>(0x0);
    io_write_aw_awqos   = UInt<4>(0x0);
    
  //================================problem is here==================================  
    io_write_aw_awvalid = writerFrontend.awvalid;
    UInt<32> queue$ram$_T_63 = queue.ram[queue.value_1.as_single_word()];
    io_write_w_wdata = queue$ram$_T_63;
    io_write_w_wstrb = UInt<4>(0xf);
    UInt<1> writerFrontend$io_bus_w_wlast = writerFrontend.length == UInt<32>(0x1);
    io_write_w_wlast = writerFrontend$io_bus_w_wlast;
    UInt<1> queue$_T_43 = ~queue.maybe_full;
    UInt<1> queue$empty = queue$_T_41 & queue$_T_43;
    UInt<1> queue$io_deq_valid = ~queue$empty;
    UInt<1> writerFrontend$io_bus_w_wvalid = queue$io_deq_valid & writerFrontend.enable;
    io_write_w_wvalid = writerFrontend$io_bus_w_wvalid;
    io_write_b_bready = writerFrontend.bready;
    io_write_ar_arid = UInt<4>(0x0);
    io_write_ar_araddr = UInt<32>(0x0);
    io_write_ar_arlen = UInt<8>(0x0);
    io_write_ar_arsize = UInt<3>(0x0);
    io_write_ar_arburst = UInt<2>(0x0);
    io_write_ar_arlock = UInt<1>(0x0);
    io_write_ar_arcache = UInt<4>(0x0);
    io_write_ar_arprot = UInt<3>(0x0);
    io_write_ar_arqos = UInt<4>(0x0);
    io_write_ar_arvalid = UInt<1>(0x0);
    io_write_r_rready = UInt<1>(0x0);
    UInt<32> ctl$InterruptController$$inst$isr = ctl.InterruptController$$inst.SetCSR$$inst.reg;
    UInt<1> ctl$InterruptController$$inst$io_irq_readerDone = ctl$InterruptController$$inst$isr.bits<1,1>();
    io_irq_readerDone = ctl$InterruptController$$inst$io_irq_readerDone;
    UInt<1> ctl$InterruptController$$inst$io_irq_writerDone = ctl$InterruptController$$inst$isr.bits<0,0>();
    io_irq_writerDone = ctl$InterruptController$$inst$io_irq_writerDone;
    UInt<1> csrFrontend$_T_110 = io_control_r_rready & csrFrontend.rvalid;
    UInt<1> csrFrontend$_T_111 = io_control_w_wvalid & csrFrontend.wready;
    UInt<1> csrFrontend$_T_112 = UInt<3>(0x0) == csrFrontend.state;
    UInt<4> csrFrontend$_T_113 = io_control_aw_awaddr.bits<5,2>();
    UInt<4> csrFrontend$_T_115 = io_control_ar_araddr.bits<5,2>();
    UInt<3> csrFrontend$_GEN_0 = io_control_ar_arvalid ? UInt<3>(0x1) : csrFrontend.state;
    UInt<32> csrFrontend$_GEN_1 = io_control_ar_arvalid ? (csrFrontend$_T_115.pad<32>()) : csrFrontend.addr;
    UInt<1> csrFrontend$_GEN_2 = io_control_ar_arvalid | csrFrontend.arready;
    UInt<3> csrFrontend$_GEN_3 = io_control_aw_awvalid ? UInt<3>(0x3) : csrFrontend$_GEN_0;
    UInt<32> csrFrontend$_GEN_4 = io_control_aw_awvalid ? (csrFrontend$_T_113.pad<32>()) : csrFrontend$_GEN_1;
    UInt<1> csrFrontend$_GEN_5 = io_control_aw_awvalid | csrFrontend.awready;
    UInt<1> csrFrontend$_GEN_6 = io_control_aw_awvalid ? csrFrontend.arready : csrFrontend$_GEN_2;
    UInt<1> csrFrontend$_T_117 = UInt<3>(0x1) == csrFrontend.state;
    UInt<1> csrFrontend$_T_118 = io_control_ar_arvalid & csrFrontend.arready;
    UInt<3> csrFrontend$_GEN_7 = csrFrontend$_T_118 ? UInt<3>(0x2) : csrFrontend.state;
    UInt<1> csrFrontend$_GEN_8 = csrFrontend$_T_118 ? UInt<1>(0x0) : csrFrontend.arready;
    UInt<1> csrFrontend$_GEN_9 = csrFrontend$_T_118 | csrFrontend.rvalid;
    UInt<1> csrFrontend$_T_121 = UInt<3>(0x2) == csrFrontend.state;
    UInt<3> csrFrontend$_GEN_10 = csrFrontend$_T_110 ? UInt<3>(0x0) : csrFrontend.state;
    UInt<1> csrFrontend$_GEN_11 = csrFrontend$_T_110 ? UInt<1>(0x0) : csrFrontend.rvalid;
    UInt<1> csrFrontend$_T_124 = UInt<3>(0x3) == csrFrontend.state;
    UInt<1> csrFrontend$_T_125 = io_control_aw_awvalid & csrFrontend.awready;
    UInt<3> csrFrontend$_GEN_12 = csrFrontend$_T_125 ? UInt<3>(0x4) : csrFrontend.state;
    UInt<1> csrFrontend$_GEN_13 = csrFrontend$_T_125 ? UInt<1>(0x0) : csrFrontend.awready;
    UInt<1> csrFrontend$_GEN_14 = csrFrontend$_T_125 | csrFrontend.wready;
    UInt<1> csrFrontend$_T_128 = UInt<3>(0x4) == csrFrontend.state;
    UInt<3> csrFrontend$_GEN_15 = csrFrontend$_T_111 ? UInt<3>(0x5) : csrFrontend.state;
    UInt<1> csrFrontend$_GEN_16 = csrFrontend$_T_111 ? UInt<1>(0x0) : csrFrontend.wready;
    UInt<1> csrFrontend$_GEN_17 = csrFrontend$_T_111 | csrFrontend.bvalid;
    UInt<1> csrFrontend$_T_132 = UInt<3>(0x5) == csrFrontend.state;
    UInt<1> csrFrontend$_T_133 = io_control_b_bready & csrFrontend.bvalid;
    UInt<3> csrFrontend$_GEN_18 = csrFrontend$_T_133 ? UInt<3>(0x0) : csrFrontend.state;
    UInt<1> csrFrontend$_GEN_19 = csrFrontend$_T_133 ? UInt<1>(0x0) : csrFrontend.bvalid;
    UInt<3> csrFrontend$_GEN_20 = csrFrontend$_T_132 ? csrFrontend$_GEN_18 : csrFrontend.state;
    UInt<1> csrFrontend$_GEN_21 = csrFrontend$_T_132 ? csrFrontend$_GEN_19 : csrFrontend.bvalid;
    UInt<3> csrFrontend$_GEN_22 = csrFrontend$_T_128 ? csrFrontend$_GEN_15 : csrFrontend$_GEN_20;
    UInt<1> csrFrontend$_GEN_23 = csrFrontend$_T_128 ? csrFrontend$_GEN_16 : csrFrontend.wready;
    UInt<1> csrFrontend$_GEN_24 = csrFrontend$_T_128 ? csrFrontend$_GEN_17 : csrFrontend$_GEN_21;
    UInt<3> csrFrontend$_GEN_25 = csrFrontend$_T_124 ? csrFrontend$_GEN_12 : csrFrontend$_GEN_22;
    UInt<1> csrFrontend$_GEN_26 = csrFrontend$_T_124 ? csrFrontend$_GEN_13 : csrFrontend.awready;
    UInt<1> csrFrontend$_GEN_27 = csrFrontend$_T_124 ? csrFrontend$_GEN_14 : csrFrontend$_GEN_23;
    UInt<1> csrFrontend$_GEN_28 = csrFrontend$_T_124 ? csrFrontend.bvalid : csrFrontend$_GEN_24;
    UInt<3> csrFrontend$_GEN_29 = csrFrontend$_T_121 ? csrFrontend$_GEN_10 : csrFrontend$_GEN_25;
    UInt<1> csrFrontend$_GEN_30 = csrFrontend$_T_121 ? csrFrontend$_GEN_11 : csrFrontend.rvalid;
    UInt<1> csrFrontend$_GEN_31 = csrFrontend$_T_121 ? csrFrontend.awready : csrFrontend$_GEN_26;
    UInt<1> csrFrontend$_GEN_32 = csrFrontend$_T_121 ? csrFrontend.wready : csrFrontend$_GEN_27;
    UInt<1> csrFrontend$_GEN_33 = csrFrontend$_T_121 ? csrFrontend.bvalid : csrFrontend$_GEN_28;
    UInt<3> csrFrontend$_GEN_34 = csrFrontend$_T_117 ? csrFrontend$_GEN_7 : csrFrontend$_GEN_29;
    UInt<1> csrFrontend$_GEN_35 = csrFrontend$_T_117 ? csrFrontend$_GEN_8 : csrFrontend.arready;
    UInt<1> csrFrontend$_GEN_36 = csrFrontend$_T_117 ? csrFrontend$_GEN_9 : csrFrontend$_GEN_30;
    UInt<1> csrFrontend$_GEN_37 = csrFrontend$_T_117 ? csrFrontend.awready : csrFrontend$_GEN_31;
    UInt<1> csrFrontend$_GEN_38 = csrFrontend$_T_117 ? csrFrontend.wready : csrFrontend$_GEN_32;
    UInt<1> csrFrontend$_GEN_39 = csrFrontend$_T_117 ? csrFrontend.bvalid : csrFrontend$_GEN_33;
    UInt<3> csrFrontend$_GEN_40 = csrFrontend$_T_112 ? csrFrontend$_GEN_3 : csrFrontend$_GEN_34;
    UInt<32> csrFrontend$_GEN_41 = csrFrontend$_T_112 ? csrFrontend$_GEN_4 : csrFrontend.addr;
    UInt<1> csrFrontend$_GEN_42 = csrFrontend$_T_112 ? csrFrontend$_GEN_5 : csrFrontend$_GEN_37;
    UInt<1> csrFrontend$_GEN_43 = csrFrontend$_T_112 ? csrFrontend$_GEN_6 : csrFrontend$_GEN_35;
    UInt<1> csrFrontend$_GEN_44 = csrFrontend$_T_112 ? csrFrontend.rvalid : csrFrontend$_GEN_36;
    UInt<1> csrFrontend$_GEN_45 = csrFrontend$_T_112 ? csrFrontend.wready : csrFrontend$_GEN_38;
    UInt<1> csrFrontend$_GEN_46 = csrFrontend$_T_112 ? csrFrontend.bvalid : csrFrontend$_GEN_39;
    UInt<1> csrFrontend$io_bus_write = io_control_w_wvalid & csrFrontend.wready;
    if (update_registers) csrFrontend.state = reset ? UInt<3>(0x0) : csrFrontend$_GEN_40;
    if (update_registers) csrFrontend.awready = reset ? UInt<1>(0x0) : csrFrontend$_GEN_42;
    if (update_registers) csrFrontend.wready = reset ? UInt<1>(0x0) : csrFrontend$_GEN_45;
    if (update_registers) csrFrontend.bvalid = reset ? UInt<1>(0x0) : csrFrontend$_GEN_46;
    if (update_registers) csrFrontend.arready = reset ? UInt<1>(0x0) : csrFrontend$_GEN_43;
    if (update_registers) csrFrontend.rvalid = reset ? UInt<1>(0x0) : csrFrontend$_GEN_44;
    if (update_registers) csrFrontend.addr = reset ? UInt<32>(0x0) : csrFrontend$_GEN_41;
    UInt<1> readerFrontend$ready = queue$io_enq_ready & readerFrontend.enable;
    UInt<1> readerFrontend$valid = io_read_tvalid & readerFrontend.enable;
    UInt<1> readerFrontend$_T_66 = UInt<2>(0x0) == readerFrontend.state;
    UInt<2> readerFrontend$_GEN_0 = ctl.addressGeneratorRead.valid ? UInt<2>(0x1) : readerFrontend.state;
    UInt<32> readerFrontend$_GEN_1 = ctl.addressGeneratorRead.valid ? ctl.addressGeneratorRead.length_o : readerFrontend.length;
    UInt<1> readerFrontend$_T_70 = UInt<2>(0x1) == readerFrontend.state;
    UInt<1> readerFrontend$_T_71 = readerFrontend$ready & readerFrontend$valid;
    UInt<33> readerFrontend$_T_73 = readerFrontend.length - UInt<32>(0x1);
    UInt<32> readerFrontend$_T_75 = readerFrontend$_T_73.tail<1>();
    UInt<1> readerFrontend$_T_77 = readerFrontend.length == UInt<32>(0x1);
    UInt<2> readerFrontend$_GEN_3 = readerFrontend$_T_77 ? UInt<2>(0x2) : readerFrontend.state;
    UInt<1> readerFrontend$_GEN_4 = readerFrontend$_T_77 ? UInt<1>(0x0) : readerFrontend.enable;
    UInt<32> readerFrontend$_GEN_5 = readerFrontend$_T_71 ? readerFrontend$_T_75 : readerFrontend.length;
    UInt<2> readerFrontend$_GEN_6 = readerFrontend$_T_71 ? readerFrontend$_GEN_3 : readerFrontend.state;
    UInt<1> readerFrontend$_GEN_7 = readerFrontend$_T_71 ? readerFrontend$_GEN_4 : readerFrontend.enable;
    UInt<1> readerFrontend$_T_79 = UInt<2>(0x2) == readerFrontend.state;
    UInt<2> readerFrontend$_GEN_8 = readerFrontend$_T_79 ? UInt<2>(0x0) : readerFrontend.state;
    UInt<1> readerFrontend$_GEN_9 = readerFrontend$_T_79 | readerFrontend.done;
    UInt<32> readerFrontend$_GEN_10 = readerFrontend$_T_70 ? readerFrontend$_GEN_5 : readerFrontend.length;
    UInt<2> readerFrontend$_GEN_11 = readerFrontend$_T_70 ? readerFrontend$_GEN_6 : readerFrontend$_GEN_8;
    UInt<1> readerFrontend$_GEN_12 = readerFrontend$_T_70 ? readerFrontend$_GEN_7 : readerFrontend.enable;
    UInt<1> readerFrontend$_GEN_13 = readerFrontend$_T_70 ? readerFrontend.done : readerFrontend$_GEN_9;
    UInt<1> readerFrontend$_GEN_14 = readerFrontend$_T_66 ? UInt<1>(0x0) : readerFrontend$_GEN_13;
    UInt<1> readerFrontend$_GEN_15 = readerFrontend$_T_66 ? ctl.addressGeneratorRead.valid : readerFrontend$_GEN_12;
    UInt<2> readerFrontend$_GEN_16 = readerFrontend$_T_66 ? readerFrontend$_GEN_0 : readerFrontend$_GEN_11;
    UInt<32> readerFrontend$_GEN_17 = readerFrontend$_T_66 ? readerFrontend$_GEN_1 : readerFrontend$_GEN_10;
    UInt<1> readerFrontend$io_dataOut_valid = io_read_tvalid & readerFrontend.enable;
    if (update_registers) readerFrontend.state = reset ? UInt<2>(0x0) : readerFrontend$_GEN_16;
    UInt<1> ctl$addressGeneratorRead$_T_69 = ctl.addressGeneratorRead.lineCount > UInt<32>(0x0);
    UInt<2> ctl$addressGeneratorRead$_GEN_7 = ctl$addressGeneratorRead$_T_69 ? UInt<2>(0x1) : UInt<2>(0x0);
    UInt<2> ctl$addressGeneratorRead$_GEN_8 = readerFrontend.done ? ctl$addressGeneratorRead$_GEN_7 : ctl.addressGeneratorRead.state;
    if (update_registers) readerFrontend.done = reset ? UInt<1>(0x0) : readerFrontend$_GEN_14;
    if (update_registers) readerFrontend.enable = reset ? UInt<1>(0x0) : readerFrontend$_GEN_15;
    if (update_registers) readerFrontend.length = reset ? UInt<32>(0x0) : readerFrontend$_GEN_17;
    UInt<1> writerFrontend$ready = io_write_w_wready & writerFrontend.enable;
    UInt<1> writerFrontend$valid = queue$io_deq_valid & writerFrontend.enable;
    UInt<1> writerFrontend$_T_243 = UInt<2>(0x0) == writerFrontend.dataState;
    UInt<32> writerFrontend$_GEN_0 = ctl.transferSplitterWrite._T_63 ? ctl.transferSplitterWrite._T_51 : writerFrontend.length;
    UInt<2> writerFrontend$_GEN_1 = ctl.transferSplitterWrite._T_63 ? UInt<2>(0x1) : writerFrontend.dataState;
    UInt<1> writerFrontend$_GEN_2 = ctl.transferSplitterWrite._T_63 | writerFrontend.enable;
    UInt<1> writerFrontend$_T_246 = UInt<2>(0x1) == writerFrontend.dataState;
    UInt<1> writerFrontend$_T_247 = writerFrontend$ready & writerFrontend$valid;
    UInt<1> writerFrontend$_T_249 = writerFrontend.length > UInt<32>(0x1);
    UInt<33> writerFrontend$_T_251 = writerFrontend.length - UInt<32>(0x1);
    UInt<32> writerFrontend$_T_253 = writerFrontend$_T_251.tail<1>();
    UInt<32> writerFrontend$_GEN_3 = writerFrontend$_T_249 ? writerFrontend$_T_253 : writerFrontend.length;
    UInt<2> writerFrontend$_GEN_4 = writerFrontend$_T_249 ? writerFrontend.dataState : UInt<2>(0x2);
    UInt<1> writerFrontend$_GEN_5 = writerFrontend$_T_249 & writerFrontend.enable;
    UInt<1> writerFrontend$_GEN_6 = writerFrontend$_T_249 ? writerFrontend.bready : UInt<1>(0x1);
    UInt<32> writerFrontend$_GEN_7 = writerFrontend$_T_247 ? writerFrontend$_GEN_3 : writerFrontend.length;
    UInt<2> writerFrontend$_GEN_8 = writerFrontend$_T_247 ? writerFrontend$_GEN_4 : writerFrontend.dataState;
    UInt<1> writerFrontend$_GEN_9 = writerFrontend$_T_247 ? writerFrontend$_GEN_5 : writerFrontend.enable;
    UInt<1> writerFrontend$_GEN_10 = writerFrontend$_T_247 ? writerFrontend$_GEN_6 : writerFrontend.bready;
    UInt<1> writerFrontend$_T_256 = UInt<2>(0x2) == writerFrontend.dataState;
    UInt<1> writerFrontend$_T_257 = writerFrontend.bready & io_write_b_bvalid;
    UInt<1> writerFrontend$_GEN_11 = writerFrontend$_T_257 ? UInt<1>(0x0) : writerFrontend.bready;
    UInt<2> writerFrontend$_GEN_12 = writerFrontend$_T_257 ? UInt<2>(0x3) : writerFrontend.dataState;
    UInt<1> writerFrontend$_T_259 = UInt<2>(0x3) == writerFrontend.dataState;
    UInt<1> writerFrontend$_GEN_13 = writerFrontend$_T_259 | writerFrontend.done;
    UInt<2> writerFrontend$_GEN_14 = writerFrontend$_T_259 ? UInt<2>(0x0) : writerFrontend.dataState;
    UInt<1> writerFrontend$_GEN_15 = writerFrontend$_T_256 ? writerFrontend$_GEN_11 : writerFrontend.bready;
    UInt<2> writerFrontend$_GEN_16 = writerFrontend$_T_256 ? writerFrontend$_GEN_12 : writerFrontend$_GEN_14;
    UInt<1> writerFrontend$_GEN_17 = writerFrontend$_T_256 ? writerFrontend.done : writerFrontend$_GEN_13;
    UInt<32> writerFrontend$_GEN_18 = writerFrontend$_T_246 ? writerFrontend$_GEN_7 : writerFrontend.length;
    UInt<2> writerFrontend$_GEN_19 = writerFrontend$_T_246 ? writerFrontend$_GEN_8 : writerFrontend$_GEN_16;
    UInt<1> writerFrontend$_GEN_20 = writerFrontend$_T_246 ? writerFrontend$_GEN_9 : writerFrontend.enable;
    UInt<1> writerFrontend$_GEN_21 = writerFrontend$_T_246 ? writerFrontend$_GEN_10 : writerFrontend$_GEN_15;
    UInt<1> writerFrontend$_GEN_22 = writerFrontend$_T_246 ? writerFrontend.done : writerFrontend$_GEN_17;
    UInt<1> writerFrontend$_GEN_23 = writerFrontend$_T_243 ? UInt<1>(0x0) : writerFrontend$_GEN_22;
    UInt<32> writerFrontend$_GEN_24 = writerFrontend$_T_243 ? writerFrontend$_GEN_0 : writerFrontend$_GEN_18;
    UInt<2> writerFrontend$_GEN_25 = writerFrontend$_T_243 ? writerFrontend$_GEN_1 : writerFrontend$_GEN_19;
    UInt<1> writerFrontend$_GEN_26 = writerFrontend$_T_243 ? writerFrontend$_GEN_2 : writerFrontend$_GEN_20;
    UInt<1> writerFrontend$_GEN_27 = writerFrontend$_T_243 ? writerFrontend.bready : writerFrontend$_GEN_21;
    UInt<1> writerFrontend$_T_261 = UInt<2>(0x0) == writerFrontend.addrState;
    UInt<33> writerFrontend$_T_263 = ctl.transferSplitterWrite._T_51 - UInt<32>(0x1);
    UInt<32> writerFrontend$_T_265 = writerFrontend$_T_263.tail<1>();
    UInt<32> writerFrontend$_GEN_28 = ctl.transferSplitterWrite._T_63 ? ctl.transferSplitterWrite._T_48 : writerFrontend.awaddr;
    UInt<32> writerFrontend$_GEN_29 = ctl.transferSplitterWrite._T_63 ? writerFrontend$_T_265 : writerFrontend.awlen;
    UInt<1> writerFrontend$_GEN_30 = ctl.transferSplitterWrite._T_63 | writerFrontend.awvalid;
    UInt<2> writerFrontend$_GEN_31 = ctl.transferSplitterWrite._T_63 ? UInt<2>(0x1) : writerFrontend.addrState;
    UInt<1> writerFrontend$_T_267 = UInt<2>(0x1) == writerFrontend.addrState;
    UInt<1> writerFrontend$_T_268 = writerFrontend.awvalid & io_write_aw_awready;
    UInt<2> writerFrontend$_GEN_32 = writerFrontend$_T_268 ? UInt<2>(0x2) : writerFrontend.addrState;
    UInt<1> writerFrontend$_GEN_33 = writerFrontend$_T_268 ? UInt<1>(0x0) : writerFrontend.awvalid;
    UInt<1> writerFrontend$_T_270 = UInt<2>(0x2) == writerFrontend.addrState;
    UInt<2> writerFrontend$_GEN_34 = writerFrontend.done ? UInt<2>(0x0) : writerFrontend.addrState;
    UInt<2> writerFrontend$_GEN_35 = writerFrontend$_T_270 ? writerFrontend$_GEN_34 : writerFrontend.addrState;
    UInt<2> writerFrontend$_GEN_36 = writerFrontend$_T_267 ? writerFrontend$_GEN_32 : writerFrontend$_GEN_35;
    UInt<1> writerFrontend$_GEN_37 = writerFrontend$_T_267 ? writerFrontend$_GEN_33 : writerFrontend.awvalid;
    UInt<32> writerFrontend$_GEN_38 = writerFrontend$_T_261 ? writerFrontend$_GEN_28 : writerFrontend.awaddr;

    std::cout<<"writerFrontend$_GEN_38     "<<writerFrontend$_GEN_37<<std::endl;

    UInt<32> writerFrontend$_GEN_39 = writerFrontend$_T_261 ? writerFrontend$_GEN_29 : writerFrontend.awlen;
    UInt<1> writerFrontend$_GEN_40 = writerFrontend$_T_261 ? writerFrontend$_GEN_30 : writerFrontend$_GEN_37;
    UInt<2> writerFrontend$_GEN_41 = writerFrontend$_T_261 ? writerFrontend$_GEN_31 : writerFrontend$_GEN_36;
    UInt<1> writerFrontend$io_dataIn_ready = io_write_w_wready & writerFrontend.enable;
    if (update_registers) writerFrontend.dataState = reset ? UInt<2>(0x0) : writerFrontend$_GEN_25;
    if (update_registers) writerFrontend.addrState = reset ? UInt<2>(0x0) : writerFrontend$_GEN_41;
    UInt<1> ctl$transferSplitterWrite$_T_91 = ctl.transferSplitterWrite._T_45 > UInt<32>(0x0);
    UInt<2> ctl$transferSplitterWrite$_GEN_7 = ctl$transferSplitterWrite$_T_91 ? UInt<2>(0x1) : UInt<2>(0x0);
    UInt<2> ctl$transferSplitterWrite$_GEN_9 = writerFrontend.done ? ctl$transferSplitterWrite$_GEN_7 : ctl.transferSplitterWrite._T_65;
    UInt<1> ctl$transferSplitterWrite$_GEN_8 = ctl$transferSplitterWrite$_T_91 ? ctl.transferSplitterWrite._T_60 : UInt<1>(0x1);
    UInt<1> ctl$transferSplitterWrite$_GEN_10 = writerFrontend.done ? ctl$transferSplitterWrite$_GEN_8 : ctl.transferSplitterWrite._T_60;
    if (update_registers) writerFrontend.done = reset ? UInt<1>(0x0) : writerFrontend$_GEN_23;
    if (update_registers) writerFrontend.enable = reset ? UInt<1>(0x0) : writerFrontend$_GEN_26;
    if (update_registers) writerFrontend.length = reset ? UInt<32>(0x0) : writerFrontend$_GEN_24;
    if (update_registers) writerFrontend.awlen = reset ? UInt<32>(0x0) : writerFrontend$_GEN_39;
    if (update_registers) writerFrontend.awaddr = reset ? UInt<32>(0x0) : writerFrontend$_GEN_38;
    if (update_registers) writerFrontend.awvalid = reset ? UInt<1>(0x0) : writerFrontend$_GEN_40;
    if (update_registers) writerFrontend.bready = reset ? UInt<1>(0x0) : writerFrontend$_GEN_27;
    UInt<1> csr$_T_184 = csr$_T_178 & csrFrontend$io_bus_write;
    UInt<1> csr$_T_206 = csr$_T_200 & csrFrontend$io_bus_write;
    UInt<1> csr$_T_217 = csr$_T_211 & csrFrontend$io_bus_write;
    UInt<1> csr$_T_228 = csr$_T_222 & csrFrontend$io_bus_write;
    UInt<1> csr$_T_239 = csr$_T_233 & csrFrontend$io_bus_write;
    UInt<1> csr$_T_250 = csr$_T_244 & csrFrontend$io_bus_write;
    UInt<1> csr$_T_261 = csr$_T_255 & csrFrontend$io_bus_write;
    UInt<1> csr$_T_272 = csr$_T_266 & csrFrontend$io_bus_write;
    UInt<1> csr$_T_283 = csr$_T_277 & csrFrontend$io_bus_write;
    UInt<1> csr$_T_294 = csr$_T_288 & csrFrontend$io_bus_write;
    UInt<1> csr$_T_305 = csr$_T_299 & csrFrontend$io_bus_write;
    UInt<1> csr$_T_316 = csr$_T_310 & csrFrontend$io_bus_write;
    UInt<1> csr$_T_327 = csr$_T_321 & csrFrontend$io_bus_write;
    UInt<1> csr$_T_338 = csr$_T_332 & csrFrontend$io_bus_write;
    UInt<1> csr$_T_349 = csr$_T_343 & csrFrontend$io_bus_write;
    UInt<32> csr$io_csr_0_dataOut = csr$_T_184 ? io_control_w_wdata : UInt<32>(0x0);
    UInt<1> csr$io_csr_0_dataWrite = csr$_T_178 & csrFrontend$io_bus_write;
    UInt<32> csr$io_csr_2_dataOut = csr$_T_206 ? io_control_w_wdata : UInt<32>(0x0);
    UInt<1> csr$io_csr_2_dataWrite = csr$_T_200 & csrFrontend$io_bus_write;
    UInt<32> csr$io_csr_3_dataOut = csr$_T_217 ? io_control_w_wdata : UInt<32>(0x0);
    UInt<1> csr$io_csr_3_dataWrite = csr$_T_211 & csrFrontend$io_bus_write;
    UInt<32> csr$io_csr_4_dataOut = csr$_T_228 ? io_control_w_wdata : UInt<32>(0x0);
    UInt<1> csr$io_csr_4_dataWrite = csr$_T_222 & csrFrontend$io_bus_write;
    UInt<32> csr$io_csr_5_dataOut = csr$_T_239 ? io_control_w_wdata : UInt<32>(0x0);
    UInt<1> csr$io_csr_5_dataWrite = csr$_T_233 & csrFrontend$io_bus_write;
    UInt<32> csr$io_csr_6_dataOut = csr$_T_250 ? io_control_w_wdata : UInt<32>(0x0);
    UInt<1> csr$io_csr_6_dataWrite = csr$_T_244 & csrFrontend$io_bus_write;
    UInt<32> csr$io_csr_7_dataOut = csr$_T_261 ? io_control_w_wdata : UInt<32>(0x0);
    UInt<1> csr$io_csr_7_dataWrite = csr$_T_255 & csrFrontend$io_bus_write;
    UInt<32> csr$io_csr_8_dataOut = csr$_T_272 ? io_control_w_wdata : UInt<32>(0x0);
    UInt<1> csr$io_csr_8_dataWrite = csr$_T_266 & csrFrontend$io_bus_write;
    UInt<32> csr$io_csr_9_dataOut = csr$_T_283 ? io_control_w_wdata : UInt<32>(0x0);
    UInt<1> csr$io_csr_9_dataWrite = csr$_T_277 & csrFrontend$io_bus_write;
    UInt<32> csr$io_csr_10_dataOut = csr$_T_294 ? io_control_w_wdata : UInt<32>(0x0);
    UInt<1> csr$io_csr_10_dataWrite = csr$_T_288 & csrFrontend$io_bus_write;
    UInt<32> csr$io_csr_11_dataOut = csr$_T_305 ? io_control_w_wdata : UInt<32>(0x0);
    UInt<1> csr$io_csr_11_dataWrite = csr$_T_299 & csrFrontend$io_bus_write;
    UInt<32> csr$io_csr_12_dataOut = csr$_T_316 ? io_control_w_wdata : UInt<32>(0x0);
    UInt<1> csr$io_csr_12_dataWrite = csr$_T_310 & csrFrontend$io_bus_write;
    UInt<32> csr$io_csr_13_dataOut = csr$_T_327 ? io_control_w_wdata : UInt<32>(0x0);
    UInt<1> csr$io_csr_13_dataWrite = csr$_T_321 & csrFrontend$io_bus_write;
    UInt<32> csr$io_csr_14_dataOut = csr$_T_338 ? io_control_w_wdata : UInt<32>(0x0);
    UInt<1> csr$io_csr_14_dataWrite = csr$_T_332 & csrFrontend$io_bus_write;
    UInt<32> csr$io_csr_15_dataOut = csr$_T_349 ? io_control_w_wdata : UInt<32>(0x0);
    UInt<1> csr$io_csr_15_dataWrite = csr$_T_343 & csrFrontend$io_bus_write;
    UInt<2> ctl$_T_203 = ctl.readerStart.cat(ctl.writerStart);
    UInt<32> ctl$control = ctl.ClearCSR$$inst.reg;
    UInt<1> ctl$_T_204 = ctl$control.bits<5,5>();
    UInt<1> ctl$_T_205 = ctl$control.bits<4,4>();
    UInt<2> ctl$_T_206 = ctl$_T_204.cat(ctl$_T_205);
    UInt<2> ctl$_T_207 = ~ctl$_T_206;
    UInt<2> ctl$clear = ctl$_T_203 & ctl$_T_207;
    UInt<1> ctl$_T_210 = ~ctl.readerSyncOld;
    UInt<1> ctl$_T_211 = ctl$_T_210 & ctl.readerSync;
    UInt<1> ctl$_T_212 = ctl$control.bits<3,3>();
    UInt<1> ctl$_T_213 = ctl$_T_211 | ctl$_T_212;
    UInt<1> ctl$_T_214 = ctl$control.bits<1,1>();
    UInt<1> ctl$_T_215 = ctl$_T_213 & ctl$_T_214;
    UInt<1> ctl$_T_217 = ~ctl.writerSyncOld;
    UInt<1> ctl$_T_218 = ctl$_T_217 & ctl.writerSync;
    UInt<1> ctl$_T_219 = ctl$control.bits<2,2>();
    UInt<1> ctl$_T_220 = ctl$_T_218 | ctl$_T_219;
    UInt<1> ctl$_T_221 = ctl$control.bits<0,0>();
    UInt<1> ctl$_T_222 = ctl$_T_220 & ctl$_T_221;
    UInt<32> ctl$ClearCSR$$inst$io_clear = ctl$clear.pad<32>();
    UInt<32> ctl$StatusCSR$$inst$io_value = ctl.status.pad<32>();
    if (update_registers) ctl.status = ctl.addressGeneratorRead.busy.cat(ctl.addressGeneratorWrite.busy);
    if (update_registers) ctl.readerSyncOld = ctl.readerSync;
    if (update_registers) ctl.readerSync = io_sync_readerSync;
    if (update_registers) ctl.writerSyncOld = ctl.writerSync;
    if (update_registers) ctl.writerSync = io_sync_writerSync;
    UInt<2> ctl$addressGeneratorRead$_GEN_1 = ctl.readerStart ? UInt<2>(0x1) : ctl.addressGeneratorRead.state;
    UInt<32> ctl$addressGeneratorRead$_GEN_2 = ctl.readerStart ? ctl.SimpleCSR$$inst.reg : ctl.addressGeneratorRead.address_i;
    UInt<32> ctl$addressGeneratorRead$_GEN_3 = ctl.readerStart ? ctl.SimpleCSR_1.reg : ctl.addressGeneratorRead.length_i;
    UInt<32> ctl$addressGeneratorRead$_GEN_4 = ctl.readerStart ? ctl.SimpleCSR_2.reg : ctl.addressGeneratorRead.lineCount;
    UInt<32> ctl$addressGeneratorRead$_GEN_5 = ctl.readerStart ? ctl.SimpleCSR_3.reg : ctl.addressGeneratorRead.lineGap;
    if (update_registers) ctl.readerStart = reset ? UInt<1>(0x0) : ctl$_T_215;
    UInt<2> ctl$addressGeneratorWrite$_GEN_1 = ctl.writerStart ? UInt<2>(0x1) : ctl.addressGeneratorWrite.state;
    UInt<32> ctl$addressGeneratorWrite$_GEN_2 = ctl.writerStart ? ctl.SimpleCSR_4.reg : ctl.addressGeneratorWrite.address_i;
    UInt<32> ctl$addressGeneratorWrite$_GEN_3 = ctl.writerStart ? ctl.SimpleCSR_5.reg : ctl.addressGeneratorWrite.length_i;
    UInt<32> ctl$addressGeneratorWrite$_GEN_4 = ctl.writerStart ? ctl.SimpleCSR_6.reg : ctl.addressGeneratorWrite.lineCount;
    UInt<32> ctl$addressGeneratorWrite$_GEN_5 = ctl.writerStart ? ctl.SimpleCSR_7.reg : ctl.addressGeneratorWrite.lineGap;
    if (update_registers) ctl.writerStart = reset ? UInt<1>(0x0) : ctl$_T_222;
    UInt<1> ctl$addressGeneratorRead$_T_46 = ctl.addressGeneratorRead.state == UInt<2>(0x0);
    UInt<1> ctl$addressGeneratorRead$_GEN_0 = ctl$addressGeneratorRead$_T_46 ? UInt<1>(0x0) : UInt<1>(0x1);
    UInt<1> ctl$addressGeneratorRead$_T_49 = UInt<2>(0x0) == ctl.addressGeneratorRead.state;
    UInt<1> ctl$addressGeneratorRead$_T_51 = UInt<2>(0x1) == ctl.addressGeneratorRead.state;
    UInt<35> ctl$addressGeneratorRead$_T_54 = (ctl.addressGeneratorRead.length_i * UInt<32>(0x4)).tail<29>();
    UInt<35> ctl$addressGeneratorRead$_GEN_28 = ctl.addressGeneratorRead.address_i.pad<35>();
    UInt<36> ctl$addressGeneratorRead$_T_55 = ctl$addressGeneratorRead$_GEN_28 + ctl$addressGeneratorRead$_T_54;
    UInt<35> ctl$addressGeneratorRead$_T_56 = ctl$addressGeneratorRead$_T_55.tail<1>();
    UInt<35> ctl$addressGeneratorRead$_T_58 = (ctl.addressGeneratorRead.lineGap * UInt<32>(0x4)).tail<29>();
    UInt<36> ctl$addressGeneratorRead$_T_59 = ctl$addressGeneratorRead$_T_56 + ctl$addressGeneratorRead$_T_58;
    UInt<35> ctl$addressGeneratorRead$_T_60 = ctl$addressGeneratorRead$_T_59.tail<1>();
    UInt<33> ctl$addressGeneratorRead$_T_62 = ctl.addressGeneratorRead.lineCount - UInt<32>(0x1);
    UInt<32> ctl$addressGeneratorRead$_T_64 = ctl$addressGeneratorRead$_T_62.tail<1>();
    UInt<1> ctl$addressGeneratorRead$_T_65 = UInt<2>(0x2) == ctl.addressGeneratorRead.state;
    UInt<1> ctl$addressGeneratorRead$_GEN_9 = ctl$addressGeneratorRead$_T_65 ? UInt<1>(0x0) : ctl.addressGeneratorRead.valid;
    UInt<2> ctl$addressGeneratorRead$_GEN_11 = ctl$addressGeneratorRead$_T_65 ? ctl$addressGeneratorRead$_GEN_8 : ctl.addressGeneratorRead.state;
    UInt<1> ctl$addressGeneratorRead$_GEN_12 = ctl$addressGeneratorRead$_T_51 | ctl$addressGeneratorRead$_GEN_9;
    UInt<32> ctl$addressGeneratorRead$_GEN_13 = ctl$addressGeneratorRead$_T_51 ? ctl.addressGeneratorRead.address_i : ctl.addressGeneratorRead.address_o;
    UInt<32> ctl$addressGeneratorRead$_GEN_14 = ctl$addressGeneratorRead$_T_51 ? ctl.addressGeneratorRead.length_i : ctl.addressGeneratorRead.length_o;
    UInt<35> ctl$addressGeneratorRead$_GEN_15 = ctl$addressGeneratorRead$_T_51 ? ctl$addressGeneratorRead$_T_60 : (ctl.addressGeneratorRead.address_i.pad<35>());
    UInt<32> ctl$addressGeneratorRead$_GEN_16 = ctl$addressGeneratorRead$_T_51 ? ctl$addressGeneratorRead$_T_64 : ctl.addressGeneratorRead.lineCount;
    UInt<2> ctl$addressGeneratorRead$_GEN_17 = ctl$addressGeneratorRead$_T_51 ? UInt<2>(0x2) : ctl$addressGeneratorRead$_GEN_11;
    UInt<2> ctl$addressGeneratorRead$_GEN_19 = ctl$addressGeneratorRead$_T_49 ? ctl$addressGeneratorRead$_GEN_1 : ctl$addressGeneratorRead$_GEN_17;
    UInt<35> ctl$addressGeneratorRead$_GEN_20 = ctl$addressGeneratorRead$_T_49 ? (ctl$addressGeneratorRead$_GEN_2.pad<35>()) : ctl$addressGeneratorRead$_GEN_15;
    UInt<32> ctl$addressGeneratorRead$_GEN_21 = ctl$addressGeneratorRead$_T_49 ? ctl$addressGeneratorRead$_GEN_3 : ctl.addressGeneratorRead.length_i;
    UInt<32> ctl$addressGeneratorRead$_GEN_22 = ctl$addressGeneratorRead$_T_49 ? ctl$addressGeneratorRead$_GEN_4 : ctl$addressGeneratorRead$_GEN_16;
    UInt<32> ctl$addressGeneratorRead$_GEN_23 = ctl$addressGeneratorRead$_T_49 ? ctl$addressGeneratorRead$_GEN_5 : ctl.addressGeneratorRead.lineGap;
    UInt<1> ctl$addressGeneratorRead$_GEN_25 = ctl$addressGeneratorRead$_T_49 ? ctl.addressGeneratorRead.valid : ctl$addressGeneratorRead$_GEN_12;
    UInt<32> ctl$addressGeneratorRead$_GEN_26 = ctl$addressGeneratorRead$_T_49 ? ctl.addressGeneratorRead.address_o : ctl$addressGeneratorRead$_GEN_13;
    UInt<32> ctl$addressGeneratorRead$_GEN_27 = ctl$addressGeneratorRead$_T_49 ? ctl.addressGeneratorRead.length_o : ctl$addressGeneratorRead$_GEN_14;
    if (update_registers) ctl.addressGeneratorRead.state = reset ? UInt<2>(0x0) : ctl$addressGeneratorRead$_GEN_19;
    if (update_registers) ctl.addressGeneratorRead.lineCount = reset ? UInt<32>(0x0) : ctl$addressGeneratorRead$_GEN_22;
    if (update_registers) ctl.addressGeneratorRead.lineGap = reset ? UInt<32>(0x0) : ctl$addressGeneratorRead$_GEN_23;
    if (update_registers) ctl.addressGeneratorRead.address_o = reset ? UInt<32>(0x0) : ctl$addressGeneratorRead$_GEN_26;
    if (update_registers) ctl.addressGeneratorRead.address_i = reset ? UInt<32>(0x0) : (ctl$addressGeneratorRead$_GEN_20.bits<31,0>());
    if (update_registers) ctl.addressGeneratorRead.length_o = reset ? UInt<32>(0x0) : ctl$addressGeneratorRead$_GEN_27;
    if (update_registers) ctl.addressGeneratorRead.length_i = reset ? UInt<32>(0x0) : ctl$addressGeneratorRead$_GEN_21;
    if (update_registers) ctl.addressGeneratorRead.valid = reset ? UInt<1>(0x0) : ctl$addressGeneratorRead$_GEN_25;
    UInt<1> ctl$InterruptController$$inst$_T_64 = ~ctl.InterruptController$$inst.readBusy;
    UInt<1> ctl$InterruptController$$inst$_T_65 = ctl.InterruptController$$inst.readBusyOld & ctl$InterruptController$$inst$_T_64;
    if (update_registers) ctl.InterruptController$$inst.readBusyOld = ctl.InterruptController$$inst.readBusy;
    if (update_registers) ctl.InterruptController$$inst.readBusy = ctl.addressGeneratorRead.busy;
    if (update_registers) ctl.addressGeneratorRead.busy = reset ? UInt<1>(0x0) : ctl$addressGeneratorRead$_GEN_0;
    UInt<1> ctl$addressGeneratorWrite$_T_46 = ctl.addressGeneratorWrite.state == UInt<2>(0x0);
    UInt<1> ctl$addressGeneratorWrite$_GEN_0 = ctl$addressGeneratorWrite$_T_46 ? UInt<1>(0x0) : UInt<1>(0x1);
    UInt<1> ctl$addressGeneratorWrite$_T_49 = UInt<2>(0x0) == ctl.addressGeneratorWrite.state;
    UInt<1> ctl$addressGeneratorWrite$_T_51 = UInt<2>(0x1) == ctl.addressGeneratorWrite.state;
    UInt<35> ctl$addressGeneratorWrite$_T_54 = (ctl.addressGeneratorWrite.length_i * UInt<32>(0x4)).tail<29>();
    UInt<35> ctl$addressGeneratorWrite$_GEN_28 = ctl.addressGeneratorWrite.address_i.pad<35>();
    UInt<36> ctl$addressGeneratorWrite$_T_55 = ctl$addressGeneratorWrite$_GEN_28 + ctl$addressGeneratorWrite$_T_54;
    UInt<35> ctl$addressGeneratorWrite$_T_56 = ctl$addressGeneratorWrite$_T_55.tail<1>();
    UInt<35> ctl$addressGeneratorWrite$_T_58 = (ctl.addressGeneratorWrite.lineGap * UInt<32>(0x4)).tail<29>();
    UInt<36> ctl$addressGeneratorWrite$_T_59 = ctl$addressGeneratorWrite$_T_56 + ctl$addressGeneratorWrite$_T_58;
    UInt<35> ctl$addressGeneratorWrite$_T_60 = ctl$addressGeneratorWrite$_T_59.tail<1>();
    UInt<33> ctl$addressGeneratorWrite$_T_62 = ctl.addressGeneratorWrite.lineCount - UInt<32>(0x1);
    UInt<32> ctl$addressGeneratorWrite$_T_64 = ctl$addressGeneratorWrite$_T_62.tail<1>();
    UInt<1> ctl$addressGeneratorWrite$_T_65 = UInt<2>(0x2) == ctl.addressGeneratorWrite.state;
    UInt<1> ctl$addressGeneratorWrite$_T_69 = ctl.addressGeneratorWrite.lineCount > UInt<32>(0x0);
    UInt<2> ctl$addressGeneratorWrite$_GEN_7 = ctl$addressGeneratorWrite$_T_69 ? UInt<2>(0x1) : UInt<2>(0x0);
    UInt<2> ctl$addressGeneratorWrite$_GEN_8 = ctl.transferSplitterWrite._T_60 ? ctl$addressGeneratorWrite$_GEN_7 : ctl.addressGeneratorWrite.state;
    UInt<1> ctl$addressGeneratorWrite$_GEN_9 = ctl$addressGeneratorWrite$_T_65 ? UInt<1>(0x0) : ctl.addressGeneratorWrite.valid;
    UInt<2> ctl$addressGeneratorWrite$_GEN_11 = ctl$addressGeneratorWrite$_T_65 ? ctl$addressGeneratorWrite$_GEN_8 : ctl.addressGeneratorWrite.state;
    UInt<1> ctl$addressGeneratorWrite$_GEN_12 = ctl$addressGeneratorWrite$_T_51 | ctl$addressGeneratorWrite$_GEN_9;
    UInt<32> ctl$addressGeneratorWrite$_GEN_13 = ctl$addressGeneratorWrite$_T_51 ? ctl.addressGeneratorWrite.address_i : ctl.addressGeneratorWrite.address_o;
    UInt<32> ctl$addressGeneratorWrite$_GEN_14 = ctl$addressGeneratorWrite$_T_51 ? ctl.addressGeneratorWrite.length_i : ctl.addressGeneratorWrite.length_o;
    UInt<35> ctl$addressGeneratorWrite$_GEN_15 = ctl$addressGeneratorWrite$_T_51 ? ctl$addressGeneratorWrite$_T_60 : (ctl.addressGeneratorWrite.address_i.pad<35>());
    UInt<32> ctl$addressGeneratorWrite$_GEN_16 = ctl$addressGeneratorWrite$_T_51 ? ctl$addressGeneratorWrite$_T_64 : ctl.addressGeneratorWrite.lineCount;
    UInt<2> ctl$addressGeneratorWrite$_GEN_17 = ctl$addressGeneratorWrite$_T_51 ? UInt<2>(0x2) : ctl$addressGeneratorWrite$_GEN_11;
    UInt<2> ctl$addressGeneratorWrite$_GEN_19 = ctl$addressGeneratorWrite$_T_49 ? ctl$addressGeneratorWrite$_GEN_1 : ctl$addressGeneratorWrite$_GEN_17;
    UInt<35> ctl$addressGeneratorWrite$_GEN_20 = ctl$addressGeneratorWrite$_T_49 ? (ctl$addressGeneratorWrite$_GEN_2.pad<35>()) : ctl$addressGeneratorWrite$_GEN_15;
    UInt<32> ctl$addressGeneratorWrite$_GEN_21 = ctl$addressGeneratorWrite$_T_49 ? ctl$addressGeneratorWrite$_GEN_3 : ctl.addressGeneratorWrite.length_i;
    UInt<32> ctl$addressGeneratorWrite$_GEN_22 = ctl$addressGeneratorWrite$_T_49 ? ctl$addressGeneratorWrite$_GEN_4 : ctl$addressGeneratorWrite$_GEN_16;
    UInt<32> ctl$addressGeneratorWrite$_GEN_23 = ctl$addressGeneratorWrite$_T_49 ? ctl$addressGeneratorWrite$_GEN_5 : ctl.addressGeneratorWrite.lineGap;
    UInt<1> ctl$addressGeneratorWrite$_GEN_25 = ctl$addressGeneratorWrite$_T_49 ? ctl.addressGeneratorWrite.valid : ctl$addressGeneratorWrite$_GEN_12;
    UInt<32> ctl$addressGeneratorWrite$_GEN_26 = ctl$addressGeneratorWrite$_T_49 ? ctl.addressGeneratorWrite.address_o : ctl$addressGeneratorWrite$_GEN_13;
    UInt<32> ctl$addressGeneratorWrite$_GEN_27 = ctl$addressGeneratorWrite$_T_49 ? ctl.addressGeneratorWrite.length_o : ctl$addressGeneratorWrite$_GEN_14;
    if (update_registers) ctl.addressGeneratorWrite.state = reset ? UInt<2>(0x0) : ctl$addressGeneratorWrite$_GEN_19;
    if (update_registers) ctl.addressGeneratorWrite.lineCount = reset ? UInt<32>(0x0) : ctl$addressGeneratorWrite$_GEN_22;
    if (update_registers) ctl.addressGeneratorWrite.lineGap = reset ? UInt<32>(0x0) : ctl$addressGeneratorWrite$_GEN_23;
    UInt<32> ctl$transferSplitterWrite$_GEN_0 = ctl.addressGeneratorWrite.valid ? ctl.addressGeneratorWrite.address_o : ctl.transferSplitterWrite._T_42;
    if (update_registers) ctl.addressGeneratorWrite.address_o = reset ? UInt<32>(0x0) : ctl$addressGeneratorWrite$_GEN_26;
    if (update_registers) ctl.addressGeneratorWrite.address_i = reset ? UInt<32>(0x0) : (ctl$addressGeneratorWrite$_GEN_20.bits<31,0>());
    UInt<32> ctl$transferSplitterWrite$_GEN_1 = ctl.addressGeneratorWrite.valid ? ctl.addressGeneratorWrite.length_o : ctl.transferSplitterWrite._T_45;
    if (update_registers) ctl.addressGeneratorWrite.length_o = reset ? UInt<32>(0x0) : ctl$addressGeneratorWrite$_GEN_27;
    if (update_registers) ctl.addressGeneratorWrite.length_i = reset ? UInt<32>(0x0) : ctl$addressGeneratorWrite$_GEN_21;
    UInt<2> ctl$transferSplitterWrite$_GEN_3 = ctl.addressGeneratorWrite.valid ? UInt<2>(0x1) : ctl.transferSplitterWrite._T_65;
    if (update_registers) ctl.addressGeneratorWrite.valid = reset ? UInt<1>(0x0) : ctl$addressGeneratorWrite$_GEN_25;
    UInt<1> ctl$InterruptController$$inst$_T_59 = ~ctl.InterruptController$$inst.writeBusy;
    UInt<1> ctl$InterruptController$$inst$_T_60 = ctl.InterruptController$$inst.writeBusyOld & ctl$InterruptController$$inst$_T_59;
    if (update_registers) ctl.InterruptController$$inst.writeBusyOld = ctl.InterruptController$$inst.writeBusy;
    if (update_registers) ctl.InterruptController$$inst.writeBusy = ctl.addressGeneratorWrite.busy;
    if (update_registers) ctl.addressGeneratorWrite.busy = reset ? UInt<1>(0x0) : ctl$addressGeneratorWrite$_GEN_0;
    UInt<1> ctl$transferSplitterWrite$_T_66 = UInt<2>(0x0) == ctl.transferSplitterWrite._T_65;
    UInt<1> ctl$transferSplitterWrite$_T_68 = UInt<2>(0x1) == ctl.transferSplitterWrite._T_65;
    UInt<1> ctl$transferSplitterWrite$_T_71 = ctl.transferSplitterWrite._T_45 > UInt<32>(0x100);
    UInt<33> ctl$transferSplitterWrite$_T_74 = ctl.transferSplitterWrite._T_45 - UInt<32>(0x100);
    UInt<32> ctl$transferSplitterWrite$_T_76 = ctl$transferSplitterWrite$_T_74.tail<1>();
    UInt<12> ctl$transferSplitterWrite$_T_79 = (UInt<9>(0x100) * UInt<9>(0x4)).tail<6>();
    UInt<32> ctl$transferSplitterWrite$_GEN_33 = ctl$transferSplitterWrite$_T_79.pad<32>();
    UInt<33> ctl$transferSplitterWrite$_T_80 = ctl.transferSplitterWrite._T_42 + ctl$transferSplitterWrite$_GEN_33;
    UInt<32> ctl$transferSplitterWrite$_T_81 = ctl$transferSplitterWrite$_T_80.tail<1>();
    UInt<35> ctl$transferSplitterWrite$_T_84 = (ctl.transferSplitterWrite._T_45 * UInt<32>(0x4)).tail<29>();
    UInt<35> ctl$transferSplitterWrite$_GEN_34 = ctl.transferSplitterWrite._T_42.pad<35>();
    UInt<36> ctl$transferSplitterWrite$_T_85 = ctl$transferSplitterWrite$_GEN_34 + ctl$transferSplitterWrite$_T_84;
    UInt<35> ctl$transferSplitterWrite$_T_86 = ctl$transferSplitterWrite$_T_85.tail<1>();
    UInt<32> ctl$transferSplitterWrite$_GEN_4 = ctl$transferSplitterWrite$_T_71 ? UInt<32>(0x100) : ctl.transferSplitterWrite._T_45;
    UInt<32> ctl$transferSplitterWrite$_GEN_5 = ctl$transferSplitterWrite$_T_71 ? ctl$transferSplitterWrite$_T_76 : UInt<32>(0x0);
    UInt<35> ctl$transferSplitterWrite$_GEN_6 = ctl$transferSplitterWrite$_T_71 ? (ctl$transferSplitterWrite$_T_81.pad<35>()) : ctl$transferSplitterWrite$_T_86;
    UInt<1> ctl$transferSplitterWrite$_T_87 = UInt<2>(0x2) == ctl.transferSplitterWrite._T_65;
    UInt<1> ctl$transferSplitterWrite$_GEN_11 = ctl$transferSplitterWrite$_T_87 ? UInt<1>(0x0) : ctl.transferSplitterWrite._T_63;
    UInt<2> ctl$transferSplitterWrite$_GEN_13 = ctl$transferSplitterWrite$_T_87 ? ctl$transferSplitterWrite$_GEN_9 : ctl.transferSplitterWrite._T_65;
    UInt<1> ctl$transferSplitterWrite$_GEN_14 = ctl$transferSplitterWrite$_T_87 ? ctl$transferSplitterWrite$_GEN_10 : ctl.transferSplitterWrite._T_60;
    UInt<32> ctl$transferSplitterWrite$_GEN_15 = ctl$transferSplitterWrite$_T_68 ? ctl.transferSplitterWrite._T_42 : ctl.transferSplitterWrite._T_48;
    UInt<1> ctl$transferSplitterWrite$_GEN_17 = ctl$transferSplitterWrite$_T_68 | ctl$transferSplitterWrite$_GEN_11;
    UInt<2> ctl$transferSplitterWrite$_GEN_18 = ctl$transferSplitterWrite$_T_68 ? UInt<2>(0x2) : ctl$transferSplitterWrite$_GEN_13;
    UInt<32> ctl$transferSplitterWrite$_GEN_19 = ctl$transferSplitterWrite$_T_68 ? ctl$transferSplitterWrite$_GEN_4 : ctl.transferSplitterWrite._T_51;
    UInt<32> ctl$transferSplitterWrite$_GEN_20 = ctl$transferSplitterWrite$_T_68 ? ctl$transferSplitterWrite$_GEN_5 : ctl.transferSplitterWrite._T_45;
    UInt<35> ctl$transferSplitterWrite$_GEN_21 = ctl$transferSplitterWrite$_T_68 ? ctl$transferSplitterWrite$_GEN_6 : (ctl.transferSplitterWrite._T_42.pad<35>());
    UInt<1> ctl$transferSplitterWrite$_GEN_23 = ctl$transferSplitterWrite$_T_68 ? ctl.transferSplitterWrite._T_60 : ctl$transferSplitterWrite$_GEN_14;
    UInt<1> ctl$transferSplitterWrite$_GEN_24 = ctl$transferSplitterWrite$_T_66 ? UInt<1>(0x0) : ctl$transferSplitterWrite$_GEN_23;
    UInt<35> ctl$transferSplitterWrite$_GEN_25 = ctl$transferSplitterWrite$_T_66 ? (ctl$transferSplitterWrite$_GEN_0.pad<35>()) : ctl$transferSplitterWrite$_GEN_21;
    UInt<32> ctl$transferSplitterWrite$_GEN_26 = ctl$transferSplitterWrite$_T_66 ? ctl$transferSplitterWrite$_GEN_1 : ctl$transferSplitterWrite$_GEN_20;
    UInt<2> ctl$transferSplitterWrite$_GEN_28 = ctl$transferSplitterWrite$_T_66 ? ctl$transferSplitterWrite$_GEN_3 : ctl$transferSplitterWrite$_GEN_18;
    UInt<32> ctl$transferSplitterWrite$_GEN_29 = ctl$transferSplitterWrite$_T_66 ? ctl.transferSplitterWrite._T_48 : ctl$transferSplitterWrite$_GEN_15;
    UInt<1> ctl$transferSplitterWrite$_GEN_31 = ctl$transferSplitterWrite$_T_66 ? ctl.transferSplitterWrite._T_63 : ctl$transferSplitterWrite$_GEN_17;
    UInt<32> ctl$transferSplitterWrite$_GEN_32 = ctl$transferSplitterWrite$_T_66 ? ctl.transferSplitterWrite._T_51 : ctl$transferSplitterWrite$_GEN_19;
    if (update_registers) ctl.transferSplitterWrite._T_42 = reset ? UInt<32>(0x0) : (ctl$transferSplitterWrite$_GEN_25.bits<31,0>());
    if (update_registers) ctl.transferSplitterWrite._T_45 = reset ? UInt<32>(0x0) : ctl$transferSplitterWrite$_GEN_26;
    if (update_registers) ctl.transferSplitterWrite._T_48 = reset ? UInt<32>(0x0) : ctl$transferSplitterWrite$_GEN_29;
    if (update_registers) ctl.transferSplitterWrite._T_51 = reset ? UInt<32>(0x0) : ctl$transferSplitterWrite$_GEN_32;
    if (update_registers) ctl.transferSplitterWrite._T_60 = reset ? UInt<1>(0x0) : ctl$transferSplitterWrite$_GEN_24;
    if (update_registers) ctl.transferSplitterWrite._T_63 = reset ? UInt<1>(0x0) : ctl$transferSplitterWrite$_GEN_31;
    if (update_registers) ctl.transferSplitterWrite._T_65 = reset ? UInt<2>(0x0) : ctl$transferSplitterWrite$_GEN_28;
    UInt<32> ctl$ClearCSR$$inst$_T_29 = ~ctl$ClearCSR$$inst$io_clear;
    UInt<32> ctl$ClearCSR$$inst$_T_30 = ctl.ClearCSR$$inst.reg & ctl$ClearCSR$$inst$_T_29;
    UInt<32> ctl$ClearCSR$$inst$_GEN_0 = csr$io_csr_0_dataWrite ? csr$io_csr_0_dataOut : ctl$ClearCSR$$inst$_T_30;
    if (update_registers) ctl.ClearCSR$$inst.reg = reset ? UInt<32>(0x0) : ctl$ClearCSR$$inst$_GEN_0;
    if (update_registers) ctl.StatusCSR$$inst.reg = ctl$StatusCSR$$inst$io_value;
    UInt<32> ctl$InterruptController$$inst$mask = ctl.InterruptController$$inst.SimpleCSR$$inst.reg;
    UInt<1> ctl$InterruptController$$inst$_T_61 = ctl$InterruptController$$inst$mask.bits<0,0>();
    UInt<1> ctl$InterruptController$$inst$_T_62 = ctl$InterruptController$$inst$_T_60 & ctl$InterruptController$$inst$_T_61;
    UInt<1> ctl$InterruptController$$inst$_T_66 = ctl$InterruptController$$inst$mask.bits<1,1>();
    UInt<1> ctl$InterruptController$$inst$_T_67 = ctl$InterruptController$$inst$_T_65 & ctl$InterruptController$$inst$_T_66;
    UInt<2> ctl$InterruptController$$inst$irq = ctl.InterruptController$$inst.readBusyIrq.cat(ctl.InterruptController$$inst.writeBusyIrq);
    UInt<32> ctl$InterruptController$$inst$SetCSR$$inst$io_set = ctl$InterruptController$$inst$irq.pad<32>();
    if (update_registers) ctl.InterruptController$$inst.writeBusyIrq = reset ? UInt<1>(0x0) : ctl$InterruptController$$inst$_T_62;
    if (update_registers) ctl.InterruptController$$inst.readBusyIrq = reset ? UInt<1>(0x0) : ctl$InterruptController$$inst$_T_67;
    UInt<32> ctl$InterruptController$$inst$SimpleCSR$$inst$_GEN_0 = csr$io_csr_2_dataWrite ? csr$io_csr_2_dataOut : ctl.InterruptController$$inst.SimpleCSR$$inst.reg;
    if (update_registers) ctl.InterruptController$$inst.SimpleCSR$$inst.reg = reset ? UInt<32>(0x0) : ctl$InterruptController$$inst$SimpleCSR$$inst$_GEN_0;
    UInt<32> ctl$InterruptController$$inst$SetCSR$$inst$_T_29 = ~csr$io_csr_3_dataOut;
    UInt<32> ctl$InterruptController$$inst$SetCSR$$inst$_T_30 = ctl.InterruptController$$inst.SetCSR$$inst.reg & ctl$InterruptController$$inst$SetCSR$$inst$_T_29;
    UInt<32> ctl$InterruptController$$inst$SetCSR$$inst$_T_31 = ctl$InterruptController$$inst$SetCSR$$inst$_T_30 | ctl$InterruptController$$inst$SetCSR$$inst$io_set;
    UInt<32> ctl$InterruptController$$inst$SetCSR$$inst$_T_32 = ctl.InterruptController$$inst.SetCSR$$inst.reg | ctl$InterruptController$$inst$SetCSR$$inst$io_set;
    UInt<32> ctl$InterruptController$$inst$SetCSR$$inst$_GEN_0 = csr$io_csr_3_dataWrite ? ctl$InterruptController$$inst$SetCSR$$inst$_T_31 : ctl$InterruptController$$inst$SetCSR$$inst$_T_32;
    if (update_registers) ctl.InterruptController$$inst.SetCSR$$inst.reg = reset ? UInt<32>(0x0) : ctl$InterruptController$$inst$SetCSR$$inst$_GEN_0;
    UInt<32> ctl$SimpleCSR$$inst$_GEN_0 = csr$io_csr_4_dataWrite ? csr$io_csr_4_dataOut : ctl.SimpleCSR$$inst.reg;
    if (update_registers) ctl.SimpleCSR$$inst.reg = reset ? UInt<32>(0x0) : ctl$SimpleCSR$$inst$_GEN_0;
    UInt<32> ctl$SimpleCSR_1$_GEN_0 = csr$io_csr_5_dataWrite ? csr$io_csr_5_dataOut : ctl.SimpleCSR_1.reg;
    if (update_registers) ctl.SimpleCSR_1.reg = reset ? UInt<32>(0x0) : ctl$SimpleCSR_1$_GEN_0;
    UInt<32> ctl$SimpleCSR_2$_GEN_0 = csr$io_csr_6_dataWrite ? csr$io_csr_6_dataOut : ctl.SimpleCSR_2.reg;
    if (update_registers) ctl.SimpleCSR_2.reg = reset ? UInt<32>(0x0) : ctl$SimpleCSR_2$_GEN_0;
    UInt<32> ctl$SimpleCSR_3$_GEN_0 = csr$io_csr_7_dataWrite ? csr$io_csr_7_dataOut : ctl.SimpleCSR_3.reg;
    if (update_registers) ctl.SimpleCSR_3.reg = reset ? UInt<32>(0x0) : ctl$SimpleCSR_3$_GEN_0;
    UInt<32> ctl$SimpleCSR_4$_GEN_0 = csr$io_csr_8_dataWrite ? csr$io_csr_8_dataOut : ctl.SimpleCSR_4.reg;
    if (update_registers) ctl.SimpleCSR_4.reg = reset ? UInt<32>(0x0) : ctl$SimpleCSR_4$_GEN_0;
    UInt<32> ctl$SimpleCSR_5$_GEN_0 = csr$io_csr_9_dataWrite ? csr$io_csr_9_dataOut : ctl.SimpleCSR_5.reg;
    if (update_registers) ctl.SimpleCSR_5.reg = reset ? UInt<32>(0x0) : ctl$SimpleCSR_5$_GEN_0;
    UInt<32> ctl$SimpleCSR_6$_GEN_0 = csr$io_csr_10_dataWrite ? csr$io_csr_10_dataOut : ctl.SimpleCSR_6.reg;
    if (update_registers) ctl.SimpleCSR_6.reg = reset ? UInt<32>(0x0) : ctl$SimpleCSR_6$_GEN_0;
    UInt<32> ctl$SimpleCSR_7$_GEN_0 = csr$io_csr_11_dataWrite ? csr$io_csr_11_dataOut : ctl.SimpleCSR_7.reg;
    if (update_registers) ctl.SimpleCSR_7.reg = reset ? UInt<32>(0x0) : ctl$SimpleCSR_7$_GEN_0;
    UInt<32> ctl$SimpleCSR_8$_GEN_0 = csr$io_csr_12_dataWrite ? csr$io_csr_12_dataOut : ctl.SimpleCSR_8.reg;
    if (update_registers) ctl.SimpleCSR_8.reg = reset ? UInt<32>(0x0) : ctl$SimpleCSR_8$_GEN_0;
    UInt<32> ctl$SimpleCSR_9$_GEN_0 = csr$io_csr_13_dataWrite ? csr$io_csr_13_dataOut : ctl.SimpleCSR_9.reg;
    if (update_registers) ctl.SimpleCSR_9.reg = reset ? UInt<32>(0x0) : ctl$SimpleCSR_9$_GEN_0;
    UInt<32> ctl$SimpleCSR_10$_GEN_0 = csr$io_csr_14_dataWrite ? csr$io_csr_14_dataOut : ctl.SimpleCSR_10.reg;
    if (update_registers) ctl.SimpleCSR_10.reg = reset ? UInt<32>(0x0) : ctl$SimpleCSR_10$_GEN_0;
    UInt<32> ctl$SimpleCSR_11$_GEN_0 = csr$io_csr_15_dataWrite ? csr$io_csr_15_dataOut : ctl.SimpleCSR_11.reg;
    if (update_registers) ctl.SimpleCSR_11.reg = reset ? UInt<32>(0x0) : ctl$SimpleCSR_11$_GEN_0;
    UInt<1> queue$do_enq = queue$io_enq_ready & readerFrontend$io_dataOut_valid;
    UInt<1> queue$do_deq = writerFrontend$io_dataIn_ready & queue$io_deq_valid;
    UInt<10> queue$_T_52 = queue.value + UInt<9>(0x1);
    UInt<9> queue$_T_53 = queue$_T_52.tail<1>();
    UInt<9> queue$_GEN_5 = queue$do_enq ? queue$_T_53 : queue.value;
    UInt<10> queue$_T_56 = queue.value_1 + UInt<9>(0x1);
    UInt<9> queue$_T_57 = queue$_T_56.tail<1>();
    UInt<9> queue$_GEN_6 = queue$do_deq ? queue$_T_57 : queue.value_1;
    UInt<1> queue$_T_58 = queue$do_enq != queue$do_deq;
    UInt<1> queue$_GEN_7 = queue$_T_58 ? queue$do_enq : queue.maybe_full;
    if (update_registers && (queue$io_enq_ready & readerFrontend$io_dataOut_valid) && UInt<1>(0x1)) queue.ram[queue.value.as_single_word()] = io_read_tdata;
    if (update_registers) queue.value = reset ? UInt<9>(0x0) : queue$_GEN_5;
    if (update_registers) queue.value_1 = reset ? UInt<9>(0x0) : queue$_GEN_6;
    if (update_registers) queue.maybe_full = reset ? UInt<1>(0x0) : queue$_GEN_7;
  
  } 
  
//define debug functions here

void print_registers(){
    std::cout<<"   io_control_aw_awaddr   "<<io_control_aw_awaddr<<std::endl;     
    std::cout<<"   io_control_w_wdata     "<<io_control_w_wdata  <<std::endl;   
    std::cout<<"   io_control_ar_araddr   "<<io_control_ar_araddr<<std::endl;   
    std::cout<<"   io_control_r_rdata     "<<io_control_r_rdata  <<std::endl<<std::endl;
    std::cout<<"   io_write_aw_awid       "<<io_write_aw_awid    <<std::endl;    
    std::cout<<"   io_read_tdata          "<<io_read_tdata       <<std::endl;   
    std::cout<<"   io_write_aw_awaddr     "<<io_write_aw_awaddr  <<std::endl;
    std::cout<<"   io_write_w_wdata       "<<io_write_w_wdata    <<std::endl;
    std::cout<<"   io_write_ar_araddr     "<<io_write_ar_araddr  <<std::endl;
    std::cout<<"   io_write_r_rdata       "<<io_write_r_rdata    <<std::endl;
  }
} AXITop;

#endif  // DMATOP_H_
