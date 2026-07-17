// Copyright 2013-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// SPDX-FileCopyrightText: Copyright Hewlett Packard Enterprise Development LP
// SPDX-License-Identifier: BSD-3-Clause

// NetworkStream: Handles incoming network storage operations
class NetworkIOStream : public StreamBase {
  public:
    NetworkIOStream( Output&, Ctx*, int srcNode, int srcPid, int destPid, FireflyNetworkEvent* );
    ~NetworkIOStream() {
        m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,"NetworkIOStream destroyed\n");
    }
    
    void setDone() {
        m_ctx->deleteStream( this );
    }

  private:
    void processAck(FireflyNetworkEvent* ev, unsigned char* bufPtr, uint8_t op);
    void processStorageOp(FireflyNetworkEvent* ev, unsigned char* bufPtr);
    void processOpenClose(FireflyNetworkEvent* ev, unsigned char* bufPtr, uint8_t op);

    uint64_t m_offset;
    Hermes::Vaddr m_src;
    size_t m_length;
};


