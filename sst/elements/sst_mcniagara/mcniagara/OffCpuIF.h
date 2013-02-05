
#ifndef OFFCPUIF_H
#define OFFCPUIF_H

/// @brief Off-CPU interface (abstract) class
///
/// This class provides an interface for the simulator to use to
/// generate off-cpu events
///

namespace McNiagara{
class OffCpuIF
{
  public:
     enum access_mode {AM_READ,AM_WRITE};
     virtual ~OffCpuIF() {};
     virtual void  memoryAccess(access_mode mode, unsigned long long address, unsigned long data_size){};
     virtual void  NICAccess(access_mode mode, unsigned long data_size){};
};
}//End namespace McNiagara
#endif
