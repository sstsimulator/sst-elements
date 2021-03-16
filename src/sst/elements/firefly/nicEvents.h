// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#endif

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

    virtual int getPid() { return -1; }
    NotSerializable(NicCmdBaseEvent)
};

class NicShmemCmdEvent : public NicCmdBaseEvent {
  public:

    enum Type { Init, RegMem, Fence, Put, Putv, Get, Getv, Wait, Add, Fadd, Swap, Cswap } type;
    std::string getTypeStr( ) {
        switch( type ) {
            case Init:
            return "Init";
            case RegMem:
            return "RegMem";
            case Fence:
            return "Fence";
            case Put:
            return "Put";
            case Putv:
            return "Putv";
            case Get:
            return "Get";
            case Getv:
            return "Getv";
            case Wait:
            return "Wait";
            case Add:
            return "Add";
            case Fadd:
            return "Fadd";
            case Swap:
            return "Swap";
            case Cswap:
            return "Cswap";
        }
    }

    NicShmemCmdEvent( Type type ) :
        NicCmdBaseEvent( Shmem ), type(type) {}
    virtual int getNode() { assert(0); }

    NotSerializable(NicShmemCmdEvent)
};

class NicShmemInitCmdEvent : public NicShmemCmdEvent {
  public:
    typedef std::function<void()> Callback;
    NicShmemInitCmdEvent( Hermes::Vaddr addr, Callback callback ) :
        NicShmemCmdEvent( Init ), addr(addr), callback(callback) {}

    Hermes::Vaddr addr;
    Callback      callback;
    virtual int getNode() { return -1; }

    NotSerializable(NicShmemInitCmdEvent)
};

class NicShmemFenceCmdEvent : public NicShmemCmdEvent {
  public:
    typedef std::function<void()> Callback;
    NicShmemFenceCmdEvent( Callback callback ) :
        NicShmemCmdEvent( Fence ), callback(callback) {}

    Callback      callback;
    virtual int getNode() { return -1; }

    NotSerializable(NicShmemInitCmdEvent)
};

class NicShmemRegMemCmdEvent : public NicShmemCmdEvent {
  public:
    typedef std::function<void()> Callback;
    NicShmemRegMemCmdEvent( Hermes::MemAddr& addr, Hermes::Vaddr realAddr, size_t len, Callback callback ) :
        NicShmemCmdEvent( RegMem ), addr(addr), realAddr(realAddr), len(len), callback(callback) {}

    virtual int getNode() { return -1; }
    Hermes::MemAddr addr;
	Hermes::Vaddr   realAddr;
    size_t          len;
    Callback        callback;

    NotSerializable(NicShmemRegMemCmdEvent)
};


class NicShmemSendCmdEvent : public NicShmemCmdEvent {
  public:
    NicShmemSendCmdEvent( Type type, int vnic, int node ) :
        NicShmemCmdEvent( type ), vnic(vnic), node(node) {}

    virtual ~NicShmemSendCmdEvent() {}
    virtual int getPid() { return vnic; }
    int getVnic() { return vnic; }
    int getNode() { return node; }
    virtual Hermes::Vaddr getMyAddr()  { assert(0); }
    virtual Hermes::Vaddr getFarAddr() = 0;
    virtual void* getBacking() = 0;
    virtual size_t getLength() = 0;
    virtual Hermes::Value::Type   getDataType() { assert(0); }
    virtual Hermes::Shmem::ReduOp getOp() { return Hermes::Shmem::MOVE; }
    virtual Hermes::Value& getValue() { assert(0); }

  private:
    int vnic;
    int node;
	NotSerializable(NicShmemSendCmdEvent)
};

class NicShmemPutvCmdEvent : public NicShmemSendCmdEvent {
  public:
    typedef std::function<void()> Callback;
    NicShmemPutvCmdEvent( int vnic, int node, Hermes::Vaddr addr, Hermes::Value& value ) :
        NicShmemSendCmdEvent( Putv, vnic, node ), destAddr(addr), value(value) {}

    Hermes::Vaddr getFarAddr()  override { return destAddr; }
    size_t getLength()          override { return value.getLength(); }
    void* getBacking()          override { return value.getPtr(); }
    Hermes::Value::Type   getDataType() override { return value.getType(); }
    Hermes::Value& getValue() { return value; }

  private:

    Hermes::Vaddr   destAddr;
    Hermes::Value   value;

	NotSerializable(NicShmemPutvCmdEvent)
};

class NicShmemPutCmdEvent : public NicShmemSendCmdEvent {
  public:
    typedef std::function<void()> Callback;
    NicShmemPutCmdEvent( int vnic, int node, Hermes::Vaddr dest, Hermes::Vaddr src,
            size_t length, Callback callback, Hermes::Shmem::ReduOp op = Hermes::Shmem::MOVE ) :
        NicShmemSendCmdEvent( Put, vnic, node ), destAddr(dest), srcAddr(src),
        length(length), callback(callback), op(op)
    { }
    NicShmemPutCmdEvent( int vnic, int node, Hermes::Vaddr dest, Hermes::Vaddr src,
        size_t length, Hermes::Shmem::ReduOp op, Hermes::Value::Type dataType,  Callback callback  ) :
        NicShmemSendCmdEvent( Put, vnic, node ), destAddr(dest), srcAddr(src),
        length(length),  callback(callback), op(op), dataType(dataType)
    { }

    Hermes::Vaddr getFarAddr()  override { return destAddr; }
    size_t getLength()          override { return length; }
    void* getBacking()          override { assert(0); }

    Hermes::Value::Type getDataType()   override { return dataType; }
    Hermes::Shmem::ReduOp getOp()       override { return op; }

    Callback getCallback()      { return callback; }
    Hermes::Vaddr getMyAddr()   { return srcAddr; }

  private:

    Hermes::Vaddr   destAddr;
    Hermes::Vaddr   srcAddr;
    size_t          length;
    Callback        callback;

    Hermes::Value::Type dataType;
    Hermes::Shmem::ReduOp op;

	NotSerializable(NicShmemPutCmdEvent)
};

class NicShmemGetCmdEvent : public NicShmemSendCmdEvent {
  public:

    typedef std::function<void( )> Callback;

    NicShmemGetCmdEvent( int vnic, int node, Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, Callback callback ) :
        NicShmemSendCmdEvent( Get, vnic, node ), dest(dest), src(src), length( length ), callback(callback)
    {}

    Hermes::Vaddr getFarAddr()  override { return src; }
    size_t getLength()          override { return length; }
    void* getBacking()          override { return NULL; }
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

    Hermes::Vaddr getFarAddr()  override { return srcAddr; }
    size_t getLength()          override { return Hermes::Value::getLength( dataType); }
    void* getBacking()          override { return NULL; }
    Callback getCallback()      { return callback; }
    Hermes::Value::Type   getDataType() override { return dataType; }

    Callback        callback;
  private:

    Hermes::Vaddr   srcAddr;
    Hermes::Value::Type   dataType;

	NotSerializable(NicShmemGetvCmdEvent)
};

class NicShmemAddCmdEvent : public NicShmemSendCmdEvent {
  public:
    typedef std::function<void( )> Callback;
    NicShmemAddCmdEvent( int vnic, int node, Hermes::Vaddr addr, Hermes::Value& value ) :
        NicShmemSendCmdEvent( Add, vnic, node ), srcAddr(addr), data(value) {}

    Hermes::Vaddr getFarAddr()  override { return srcAddr; }
    size_t getLength()          override { return data.getLength(); }
    void* getBacking()          override { return data.getPtr(); }
    Hermes::Value::Type getDataType() override { return data.getType(); }
    Hermes::Value& getValue() { return data; }

  private:

    Hermes::Vaddr  srcAddr;
    Hermes::Value  data;

	NotSerializable(NicShmemAddCmdEvent)
};

class NicShmemFaddCmdEvent : public NicShmemSendCmdEvent {
  public:
    typedef std::function<void( Hermes::Value& )> Callback;
    NicShmemFaddCmdEvent( int vnic, int node, Hermes::Vaddr addr, Hermes::Value& value, Callback callback ) :
        NicShmemSendCmdEvent( Fadd, vnic, node ), srcAddr(addr), data(value), callback(callback) {}

    Hermes::Vaddr getFarAddr()  override { return srcAddr; }
    size_t getLength()          override { return data.getLength(); }
    void* getBacking()          override { return data.getPtr(); }
    Callback getCallback()      { return callback; }
    Hermes::Value::Type getDataType() override { return data.getType(); }
    Hermes::Value& getValue() { return data; }

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

    Hermes::Vaddr getFarAddr()  override { return srcAddr; }
    size_t getLength()          override { return data.getLength(); }
    void* getBacking() override { return data.getPtr(); }
    Callback getCallback()      { return callback; }
    Hermes::Value::Type   getDataType() override { return data.getType(); }
    Hermes::Value& getValue() { return data; }

    Callback        callback;

  private:
    Hermes::Vaddr   srcAddr;
    Hermes::Value  data;

	NotSerializable(NicShmemSwapCmdEvent)
};

class NicShmemCswapCmdEvent : public NicShmemSendCmdEvent {
  public:
    typedef std::function<void( Hermes::Value& )> Callback;
    NicShmemCswapCmdEvent( int vnic, int node, Hermes::Vaddr addr, Hermes::Value& cond, Hermes::Value& value, Callback callback ) :
        NicShmemSendCmdEvent( Cswap, vnic, node ), srcAddr(addr), cond(cond), data(value), callback(callback) {}

    Hermes::Vaddr getFarAddr()  override { return srcAddr; }
    size_t getLength()          override { return data.getLength(); }
    void* getBacking()          override { return data.getPtr(); }
    Callback getCallback()      { return callback; }
    Hermes::Value::Type   getDataType() override { return data.getType(); }
    Hermes::Value& getValue() { return data; }
    Hermes::Value& getCond() { return cond; }


    Callback        callback;

 private:
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
    virtual int getNode() { return -1; }

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
    int vn;

    NicCmdEvent( Type _type, int _vNic, int _node, int _tag,
            std::vector<IoVec>& _vec, void* _key, int vn = 0 ) :
        NicCmdBaseEvent(Msg),
        type( _type ),
        node( _node ),
        dst_vNic( _vNic ),
        tag( _tag ),
        iovec( _vec ),
        key( _key ),
        vn(vn)
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

    NicShmemRespBaseEvent( ) :
        NicCmdBaseEvent( Shmem ) {}

    virtual ~NicShmemRespBaseEvent() {}

    virtual void callback() = 0;
    NotSerializable(NicShmemRespBaseEvent)
};


class NicShmemRespEvent : public NicShmemRespBaseEvent {

  public:
    typedef std::function<void()> Callback;

    NicShmemRespEvent( Callback callback) :
        NicShmemRespBaseEvent( ), m_callback(callback) {}

    void callback() override { m_callback(); }
  private:
    Callback m_callback;

    NotSerializable(NicShmemRespEvent)
};

class NicShmemValueRespEvent : public NicShmemRespBaseEvent {

  public:
    typedef std::function<void(Hermes::Value&)> Callback;

    NicShmemValueRespEvent(  Callback callback, Hermes::Value& value ) :
        NicShmemRespBaseEvent( ), m_callback(callback), m_value(value) {}

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

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
