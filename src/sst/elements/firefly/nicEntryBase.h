// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

class EntryBase {
  public:
    EntryBase( ) :
        m_currentVec(0),
        m_currentPos(0),
        m_currentLen(0)
    {}
    virtual ~EntryBase() {}

    virtual size_t totalBytes() {
        size_t bytes = 0;
        for ( unsigned int i = 0; i < ioVec().size(); i++ ) {
            bytes += ioVec().at(i).len;
        }
        return bytes;
    }
    virtual void copyOut( Output&, int numBytes,
               FireflyNetworkEvent&, std::vector<MemOp>& vec );
    virtual bool copyIn( Output&,
               FireflyNetworkEvent&, std::vector<MemOp>& vec );
    virtual bool isDone()       { return ( currentVec() == ioVec().size() ); }

    virtual size_t& currentLen() { return m_currentLen; }
  private:
    virtual std::vector<IoVec>& ioVec() = 0;
    virtual size_t& currentVec() { return m_currentVec; }
    virtual size_t& currentPos() { return m_currentPos; }

    size_t m_currentVec;
    size_t m_currentPos;
    size_t m_currentLen;
};
