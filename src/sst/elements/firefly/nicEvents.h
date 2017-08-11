// Copyright 2013-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

class NicInitEvent : public Event {

  public:
    int node;
    int vNic;
    int num_vNics;

    NicInitEvent() :
        Event() {}

    NicInitEvent( int _node, int _vNic, int _num_vNics ) :
        Event(),
        node( _node ),
        vNic( _vNic ),
        num_vNics( _num_vNics )
    {
    }

public:
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & node;
        ser & vNic;
        ser & num_vNics;
    }

    ImplementSerializable(SST::Firefly::NicInitEvent);
};

class NicCmdBaseEvent : public Event {

  public:
    enum Type { Shmem, Msg } base_type;

    NicCmdBaseEvent( Type type ) : Event(), base_type(type) {}

    NotSerializable(NicCmdBaseEvent)
};

class NicShmemCmdEvent : public NicCmdBaseEvent {
  public:

    enum Type { RegMem, Fence, Put, Putv, Get, Getv, Wait, Fadd, Swap, Cswap } type;

    NicShmemCmdEvent( Type type ) :
        NicCmdBaseEvent( Shmem ), type(type) {}

    NotSerializable(NicShmemCmdEvent)
};

class NicShmemRegMemCmdEvent : public NicShmemCmdEvent {
  public:
    typedef std::function<void()> Callback;
    NicShmemRegMemCmdEvent( Hermes::MemAddr& addr, size_t len, Callback callback ) :
        NicShmemCmdEvent( RegMem ), addr(addr), len(len), callback(callback) {}

    Hermes::MemAddr addr;
    size_t          len;
    Callback        callback;

    NotSerializable(NicShmemRegMemCmdEvent)
};

    
class NicShmemSendCmdEvent : public NicShmemCmdEvent {
  public:
    NicShmemSendCmdEvent( Type type, int vnic, int node ) :
        NicShmemCmdEvent( type ), vnic(vnic), node(node) {}

    virtual ~NicShmemSendCmdEvent() {}
    int getVnic() { return vnic; }
    int getNode() { return node; }
    virtual Hermes::Vaddr getFarAddr() = 0;
    virtual void* getBacking() = 0;
    virtual size_t getLength() = 0;
    virtual Hermes::Value::Type   getDataType() { assert(0); }

  private:
    int vnic;
    int node;
	NotSerializable(NicShmemSendCmdEvent)
};

class NicShmemPutvCmdEvent : public NicShmemSendCmdEvent {
  public:
    typedef std::function<void()> Callback;
    NicShmemPutvCmdEvent( int vnic, int node, Hermes::Vaddr addr, Hermes::Value& value, Callback callback ) :
        NicShmemSendCmdEvent( Putv, vnic, node ), destAddr(addr), value(value), callback(callback) {}

    Hermes::Vaddr getFarAddr() { return destAddr; } 
    size_t getLength()          { return value.getLength(); } 
    void* getBacking()          { return value.getPtr(); } 
    Callback getCallback()      { return callback; } 

  private:

    Hermes::Vaddr   destAddr;
    Hermes::Value   value;
    Callback        callback;

	NotSerializable(NicShmemPutvCmdEvent)
};

class NicShmemPutCmdEvent : public NicShmemSendCmdEvent {
  public:
    typedef std::function<void()> Callback;
    NicShmemPutCmdEvent( int vnic, int node, Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, Callback callback ) :
        NicShmemSendCmdEvent( Put, vnic, node ), destAddr(dest), srcAddr(src), length(length), callback(callback) {}

    Hermes::Vaddr getFarAddr() { return destAddr; } 
    size_t getLength()          { return length; } 
    void* getBacking()          { assert(0); } 
    Callback getCallback()      { return callback; } 

    Hermes::Vaddr getMyAddr()   { return srcAddr; } 

  private:

    Hermes::Vaddr   destAddr;
    Hermes::Vaddr   srcAddr;
    size_t          length;
    Callback        callback;

	NotSerializable(NicShmemPutCmdEvent)	
};


class NicShmemGetCmdEvent : public NicShmemSendCmdEvent {
  public:

    typedef std::function<void( )> Callback;

    NicShmemGetCmdEvent( int vnic, int node,
            Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, 
            Callback callback ) :
        NicShmemSendCmdEvent( Get, vnic, node ), dest(dest), src(src), length( length ), 
        callback(callback)
    {}

    Hermes::Vaddr getFarAddr()  { return src; } 
    size_t getLength()          { return length; } 
    void* getBacking()          { return NULL; } 
    Callback getCallback()      { return callback; } 
    Hermes::Vaddr getMyAddr()   { return dest; } 

  private:
    Hermes::Vaddr src;
    Hermes::Vaddr dest;
    Callback callback;
    size_t length;

	NotSerializable(NicShmemGetCmdEvent)
};

class NicShmemGetvCmdEvent : public NicShmemSendCmdEvent {
  public:
    typedef std::function<void( Hermes::Value& )> Callback;
    NicShmemGetvCmdEvent( int vnic, int node, Hermes::Vaddr addr, Hermes::Value::Type type, Callback callback ) :
        NicShmemSendCmdEvent( Getv, vnic, node ), srcAddr(addr), dataType(type), callback(callback) {}

    Hermes::Vaddr getFarAddr()  { return srcAddr; } 
    size_t getLength()          { return Hermes::Value::getLength( dataType); } 
    void* getBacking()          { return NULL; } 
    Callback getCallback()      { return callback; } 
    Hermes::Value::Type   getDataType() { return dataType; }

    Callback        callback;
  private:

    Hermes::Vaddr   srcAddr;
    Hermes::Value::Type   dataType;

	NotSerializable(NicShmemGetvCmdEvent)
};

class NicShmemFaddCmdEvent : public NicShmemSendCmdEvent {
  public:
    typedef std::function<void( Hermes::Value& )> Callback;
    NicShmemFaddCmdEvent( int vnic, int node, Hermes::Vaddr addr, Hermes::Value& value, Callback callback ) :
        NicShmemSendCmdEvent( Fadd, vnic, node ), srcAddr(addr), data(value), callback(callback) {}

    Hermes::Vaddr getFarAddr()  { return srcAddr; } 
    size_t getLength()          { return data.getLength(); } 
    void* getBacking()          { return data.getPtr(); } 
    Callback getCallback()      { return callback; } 
    Hermes::Value::Type   getDataType() { return data.getType(); }

    Callback        callback;
  private:

    Hermes::Vaddr   srcAddr;
    Hermes::Value  data;

	NotSerializable(NicShmemFaddCmdEvent)
};

class NicShmemSwapCmdEvent : public NicShmemSendCmdEvent {
  public:
    typedef std::function<void( Hermes::Value& )> Callback;
    NicShmemSwapCmdEvent( int vnic, int node, Hermes::Vaddr addr, Hermes::Value& value, Callback callback ) :
        NicShmemSendCmdEvent( Swap, vnic, node ), srcAddr(addr), data(value), callback(callback) {}

    Hermes::Vaddr getFarAddr()  { return srcAddr; } 
    size_t getLength()          { return data.getLength(); } 
    void* getBacking()          { return data.getPtr(); } 
    Callback getCallback()      { return callback; } 
    Hermes::Value::Type   getDataType() { return data.getType(); }

    Callback        callback;
    Hermes::Vaddr   srcAddr;
    Hermes::Value  data;

	NotSerializable(NicShmemSwapCmdEvent)
};

class NicShmemCswapCmdEvent : public NicShmemSendCmdEvent {
  public:
    typedef std::function<void( Hermes::Value& )> Callback;
    NicShmemCswapCmdEvent( int vnic, int node, Hermes::Vaddr addr, Hermes::Value& cond, Hermes::Value& value, Callback callback ) :
        NicShmemSendCmdEvent( Cswap, vnic, node ), srcAddr(addr), cond(cond), data(value), callback(callback) {}

    Hermes::Vaddr getFarAddr()  { return srcAddr; } 
    size_t getLength()          { return data.getLength(); } 
    void* getBacking()          { return data.getPtr(); } 
    Callback getCallback()      { return callback; } 
    Hermes::Value::Type   getDataType() { return data.getType(); }


    Callback        callback;
    Hermes::Vaddr   srcAddr;
    Hermes::Value  data;
    Hermes::Value  cond;

	NotSerializable(NicShmemCswapCmdEvent)
};

class NicShmemOpCmdEvent : public NicShmemCmdEvent {
  public:
    typedef std::function<void()> Callback;
    NicShmemOpCmdEvent( Hermes::Vaddr addr, Hermes::Shmem::WaitOp op, Hermes::Value& value, Callback callback ) :
        NicShmemCmdEvent( Wait ), addr(addr), op(op), value(value), callback(callback) {}

    Hermes::Vaddr getAddr()  { return addr; } 

    Hermes::Shmem::WaitOp op;
    Hermes::Vaddr   addr;
    Hermes::Value   value;
    Callback        callback;

	NotSerializable(NicShmemOpCmdEvent)
};


class NicCmdEvent : public NicCmdBaseEvent {
  public:
    enum Type { PioSend, DmaSend, DmaRecv, Put, Get, RegMemRgn } type;
    int  node;
    int dst_vNic;
    int tag;
    std::vector<IoVec> iovec;
    void* key;

    NicCmdEvent( Type _type, int _vNic, int _node, int _tag,
            std::vector<IoVec>& _vec, void* _key ) :
        NicCmdBaseEvent(Msg),
        type( _type ),
        node( _node ),
        dst_vNic( _vNic ),
        tag( _tag ),
        iovec( _vec ),
        key( _key )
    { }

    NotSerializable(NicCmdEvent)
};

class NicRespBaseEvent : public Event {
  public:
    enum Type { Shmem, Msg } base_type;
    NicRespBaseEvent( Type type ) : Event(), base_type(type) {}

    NotSerializable(NicCmdEvent)
};

class NicShmemRespBaseEvent : public NicCmdBaseEvent {
  public:

    enum Type { RegMem, Fence, Put, Putv, Get, Getv, Wait, Fadd, Swap, Cswap, FreeCmd } type;

    NicShmemRespBaseEvent( Type type ) : 
        NicCmdBaseEvent( Shmem ), type(type) {}

    virtual ~NicShmemRespBaseEvent() {}

    virtual void callback() = 0;
    NotSerializable(NicShmemRespBaseEvent)
};


class NicShmemRespEvent : public NicShmemRespBaseEvent {

  public:
    typedef std::function<void()> Callback;

    NicShmemRespEvent( Type type, Callback callback) : 
        NicShmemRespBaseEvent( type ), m_callback(callback) {}

    void callback() override { m_callback(); }
  private:
    Callback m_callback;

    NotSerializable(NicShmemRespEvent)
};

class NicShmemValueRespEvent : public NicShmemRespBaseEvent {

  public:
    typedef std::function<void(Hermes::Value&)> Callback;

    NicShmemValueRespEvent( Type type, Callback callback, Hermes::Value& value ) : 
        NicShmemRespBaseEvent( type ), m_callback(callback), m_value(value) {}

    void callback() override { m_callback(m_value); }

  private:
    Callback m_callback;
    Hermes::Value m_value;

    NotSerializable(NicShmemValueRespEvent)
};

class NicRespEvent : public NicRespBaseEvent {

  public:
    enum Type { PioSend, DmaSend, DmaRecv, Put, Get, NeedRecv } type;
    int src_vNic;
    int node;
    int tag;
    int len;
    void* key;

    NicRespEvent( Type _type, int _vNic, int _node, int _tag,
            int _len, void* _key ) :
        NicRespBaseEvent( Msg ),
        type( _type ),
        src_vNic( _vNic ),
        node( _node ),
        tag( _tag ),
        len( _len ),
        key( _key )
    {
    }

    NicRespEvent( Type _type, int _vNic, int _node, int _tag,
            int _len ) :
        NicRespBaseEvent( Msg ),
        type( _type ),
        src_vNic( _vNic ),
        node( _node ),
        tag( _tag ),
        len( _len )
    {
    }

    NicRespEvent( Type _type, void* _key ) :
        NicRespBaseEvent( Msg ),
        type( _type ),
        key( _key )
    {
    }

    NotSerializable(NicRespEvent)
};
