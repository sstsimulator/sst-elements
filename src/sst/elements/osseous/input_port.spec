#Full signal names along with complete alias path for external object reference (E.g. ldut.cbus.fixer.TLMonitor$$inst.plusarg_reader$$inst.out)
Input_sig:
io_ins_0
io_ins_1
io_ins_2
io_ins_3
io_load
io_shift

#Interface Ports floating out in RTL C-Model
AXIPort 
#AXI Protocol to use. Will support AXIStream, AXI4, AXI4Lite
AXIProtocol:AXIStream
#Config - Master or Slave
config:Slave
#Mode - Streaming or Burst mode is supported. Current design supports only streaming mode
Mode:Streaming 
#Interface Signals
AXI_sig:
io_nasti_aw_ready
io_nasti_ar_ready
io_nasti_w_ready
io_nasti_b_valid
io_nasti_b_bits_id
io_nasti_b_bits_resp
io_nasti_r_valid 
io_nasti_r_bits_id 
io_nasti_r_bits_resp
io_nasti_r_bits_last
io_nasti_r_bits_data

