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

class ShmemRecvMove {
  public:
    virtual ~ShmemRecvMove() {}
    virtual bool copyIn( Output& dbg, FireflyNetworkEvent&, std::vector<DmaVec>& ) = 0;
    virtual bool isDone() = 0;
};

class ShmemRecvMoveMem : public ShmemRecvMove {

  public:
    ShmemRecvMoveMem( void* ptr, size_t length, Shmem* shmem, Hermes::Vaddr addr ) :
        m_ptr((uint8_t*)ptr), m_length( length ), m_shmem(shmem), m_addr( addr ), m_offset(0)  {}

    bool copyIn( Output& dbg, FireflyNetworkEvent&, std::vector<DmaVec>& );
    bool isDone() { return m_offset == m_length; }

  private:
    uint8_t*  m_ptr;
    size_t m_length;
    size_t m_offset;

    Shmem* m_shmem;
    Hermes::Vaddr m_addr;
};

class ShmemRecvMoveValue : public ShmemRecvMove {

  public:
    ShmemRecvMoveValue( Hermes::Value& value ) : 
        m_value(value), m_offset(0)  {}

    bool copyIn( Output& dbg, FireflyNetworkEvent&, std::vector<DmaVec>& );
    bool isDone() { return m_offset == m_value.getLength(); }

  private:
    Hermes::Value&  m_value;
    size_t          m_offset;
};

class ShmemSendMove {
  public:
    virtual ~ShmemSendMove() {}
    virtual void copyOut( Output& dbg, int vc, int numBytes,
            FireflyNetworkEvent&, std::vector<DmaVec>& ) = 0;

    virtual bool isDone() = 0;
};

class ShmemSendMoveMem : public ShmemSendMove {

  public:
    ShmemSendMoveMem( void* ptr, size_t length ) :
        m_ptr((uint8_t*)ptr), m_length( length ), m_offset(0)  { }

    virtual void copyOut( Output& dbg, int vc, int numBytes,
            FireflyNetworkEvent&, std::vector<DmaVec>& );
    bool isDone() { return m_offset == m_length; }

  private:
    uint8_t*  m_ptr;
    size_t m_length;
    size_t m_offset;
};

class ShmemSendMoveValue : public ShmemSendMove {

  public:
    ShmemSendMoveValue( Hermes::Value& value ) :
        m_value( value ), m_offset(0)  { }

    virtual void copyOut( Output& dbg, int vc, int numBytes,
            FireflyNetworkEvent&, std::vector<DmaVec>& );
    bool isDone() { return m_offset == m_value.getLength(); }

  private:
    Hermes::Value& m_value;
    size_t m_offset;
};

class ShmemSendMove2Value : public ShmemSendMove {

  public:
    ShmemSendMove2Value( Hermes::Value& value1, Hermes::Value& value2 ) :
        m_value1( value1 ), m_value2( value2), m_offset(0)  { }

    virtual void copyOut( Output& dbg, int vc, int numBytes,
            FireflyNetworkEvent&, std::vector<DmaVec>& );

    bool isDone() { return m_offset == getLength(); }

  private:
    size_t getLength() { return 2 * m_value1.getLength(); }
    Hermes::Value& m_value1;
    Hermes::Value& m_value2;
    size_t m_offset;
};



