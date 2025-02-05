// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

class RdmaStream : public StreamBase {
  public:
    RdmaStream( Output&, Ctx*, int srcNode, int srcPid, int destPid, FireflyNetworkEvent* );
    bool isBlocked() {
        return  StreamBase::isBlocked() || m_blocked;
    }
  protected:
    void processPktHdr( FireflyNetworkEvent* ev );
	bool m_blocked;
};
