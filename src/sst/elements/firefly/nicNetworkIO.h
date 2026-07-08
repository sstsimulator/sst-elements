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

class NetworkIO {
    public:
        NetworkIO(Nic& nic, Params& params, Output& output);
        ~NetworkIO() {}

        // Main event handler - dispatches to READ or WRITE handlers
        void handleEvent(NicNetworkIOCmdEvent* event, int id);

    private:
        // Network storage operation handlers
        void handleNetworkIORead(NicNetworkIOReadCmdEvent* event, int id);
        void handleNetworkIOWrite(NicNetworkIOWriteCmdEvent* event, int id);
        void handleNetworkIOOpen(NicNetworkIOOpenCmdEvent* event, int id);
        void handleNetworkIOClose(NicNetworkIOCloseCmdEvent* event, int id);

        // Core references
        Nic& m_nic;
        Output& m_dbg;
        std::string m_prefix;

        std::string prefix() { return m_prefix; }
};
