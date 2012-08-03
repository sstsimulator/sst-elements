g++ -c Memory_Controller.cc DRAM.cc DIMM.cc Transaction.cc PendingQueue.cc RefreshQueue.cc Command.cc TransactionQueue.cc DRAMaddress.cc PowerConfig.cc Aux_Stat.cc BIU.cc DRAM_Sim.cc utilities.cc RBRR.cc DRAM_config.cc 
ar rcu libdramsim.a *.o
ranlib libdramsim.a
