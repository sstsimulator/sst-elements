/**
Copyright 2009-2025 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2025, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#pragma once

//#include <sstmac/common/sstmac_config.h>

#ifdef SST_HG_OTF2_ENABLED

#include <sstmac/common/stats/stat_collector.h>
#include <dumpi/libotf2dump/otf2writer.h>
#include <sumi-mpi/mpi_comm/mpi_comm_fwd.h>
#include <sumi-mpi/mpi_api_fwd.h>

namespace SST::MASKMPI {

class OTF2Writer : public SST::Statistics::CustomStatistic
{
 public:
    SST_ELI_REGISTER_CUSTOM_STATISTIC(
      OTF2Writer,
      "macro",
      "otf2writer",
      SST_ELI_ELEMENT_VERSION(1,0,0),
      "Writes OTF2 traces capturing the simulation")

  OTF2Writer(SST::BaseComponent* parent, const std::string& name,
             const std::string& subNname, SST::Params& params);

  dumpi::OTF2_Writer& writer(){
    return writer_;
  }

  void dumpLocalData();

  void dumpGlobalData();

  void reduce(OTF2Writer* stat);

  void init(uint64_t start, uint64_t stop, int rank, int size);

  void addComm(MpiComm* comm, dumpi::mpi_comm_t parent_comm);

  void assignGlobalCommIds(MpiApi* mpi); 

 private:
  dumpi::OTF2_Writer writer_;
  int rank_;
  int size_;
  uint64_t min_time_;
  uint64_t max_time_;
  std::vector<int> event_counts_;
  std::vector<dumpi::OTF2_MPI_Comm::shared_ptr> all_comms_;

};

class OTF2Output : public sstmac::StatisticOutput
{
 public:
  SST_ELI_REGISTER_DERIVED(
    SST::Statistics::StatisticOutput,
    OTF2Output,
    "macro",
    "otf2",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "Writes OTF2 traces capturing the simulation")

  OTF2Output(SST::Params& params);

  void registerStatistic(SST::Statistics::StatisticBase* stat) override {}

  void startOutputGroup(SST::Statistics::StatisticGroup * grp) override;
  void stopOutputGroup() override;

  void output(SST::Statistics::StatisticBase* statistic, bool endOfSimFlag) override;

  bool checkOutputParameters() override { return true; }
  void startOfSimulation() override {}
  void endOfSimulation() override {}
  void printUsage() override {}

 private:
  OTF2Writer* first_in_grp_;

};

}

#endif
