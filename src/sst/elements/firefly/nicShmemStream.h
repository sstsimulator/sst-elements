// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

class ShmemStream : public StreamBase {
  public:
    typedef std::function<void()> Callback;

	struct Ret {
    	SimTime_t delay;
    	Callback  callback;
	};

    ShmemStream( Output&, FireflyNetworkEvent*, RecvMachine& );

  private:
    Ret processAck( ShmemMsgHdr&, FireflyNetworkEvent*, int, int );
    Ret processPut( ShmemMsgHdr&, FireflyNetworkEvent*, int, int );
    Ret processGetResp( ShmemMsgHdr&, FireflyNetworkEvent*, int, int );
    Ret processGet( ShmemMsgHdr&, FireflyNetworkEvent*, int, int );
    Ret processFadd( ShmemMsgHdr&, FireflyNetworkEvent*, int, int );
    Ret processAdd( ShmemMsgHdr&, FireflyNetworkEvent*, int, int );
    Ret processCswap( ShmemMsgHdr&, FireflyNetworkEvent*, int, int );
    Ret processSwap( ShmemMsgHdr&, FireflyNetworkEvent*, int, int );
    ShmemMsgHdr m_shmemHdr;
};
