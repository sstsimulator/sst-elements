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

#ifndef _H_HERMES
#define _H_HERMES

#include <sst/core/module.h>
#include <sst/core/subcomponent.h>
#include <sst/core/component.h>
#include <sst/core/subcomponent.h>
#include "sst/elements/thornhill/detailedCompute.h"
#include "sst/elements/thornhill/memoryHeapLink.h"

#include "functor.h"

using namespace SST;

namespace SST {

namespace Hermes {

typedef std::function<void(int)> Callback;

typedef uint64_t Vaddr;

class Value {
  public:
    typedef enum { Empty, Double, Float, Int, Long, LongDouble, LongLong, Short } Type;

    Value() : m_type( Empty ), m_ptr(NULL), m_length(0) { }
    Value(const Value& obj ) : m_type( Empty ), m_ptr(NULL), m_length(0) { copy(*this,obj); }

    Value& operator=(const Value& v) {
        copy( *this, v );
        return *this;
    }

    Value( Type type ) : m_type( type ) {
        m_length = getLength( type );
        m_value.resize(m_length);
        m_ptr = &m_value[0];
    }

    Value( float v ) : m_type(Float), m_length(sizeof(float)), m_value(sizeof(float)) {
        m_ptr = &m_value[0];
        set( v );
    }

    Value( double v ) : m_type(Double), m_length(sizeof(double)), m_value(sizeof(double)) {
        m_ptr = &m_value[0];
        set( v );
    }

    Value( long double v ) : m_type(LongDouble), m_length(sizeof(long double)), m_value(sizeof(long double)) {
        m_ptr = &m_value[0];
        set( v );
    }

    Value( short v ) : m_type(Short), m_length(sizeof(short)), m_value(sizeof(short)) {
        m_ptr = &m_value[0];
        set( v );
    }

    Value( int v ) : m_type(Int), m_length(sizeof(int)), m_value(sizeof(int)) {
        m_ptr = &m_value[0];
        set( v );
    }

    Value( long v ) : m_type(Long), m_length(sizeof(long)), m_value(sizeof(long)) {
        m_ptr = &m_value[0];
        set( v );
    }

    Value( long long v ) : m_type(LongLong), m_length(sizeof(long long )), m_value(sizeof(long long)) {
        m_ptr = &m_value[0];
        set( v );
    }


    Value( float* ptr )       : m_type(Float),      m_length(sizeof(float)), m_ptr(ptr) {}
    Value( double* ptr )      : m_type(Double),     m_length(sizeof(double)), m_ptr(ptr) {}
    Value( long double* ptr ) : m_type(LongDouble), m_length(sizeof(long double)), m_ptr(ptr) {}

    Value( short* v )     : m_type(Short),    m_length(sizeof(short)),    m_ptr(v) {}
    Value( int* v )       : m_type(Int),      m_length(sizeof(int)),    m_ptr(v) {}
    Value( long* v )      : m_type(Long),     m_length(sizeof(long)),    m_ptr(v) {}
    Value( long long* v ) : m_type(LongLong), m_length(sizeof(long long)),    m_ptr(v) {}

    Value( Type type, void* ptr ) : m_type(type), m_length(getLength(type)), m_ptr(ptr) {}

#define MATH_OP(op)\
        assert( getType() == rh.getType() );\
        switch( getType() ) {\
          case Short:\
            set( get<short>() op rh.get<short>() );\
            break;\
          case Int:\
            set( get<int>() op rh.get<int>() );\
            break;\
          case Long:\
            set( get<long>() op rh.get<long>() );\
            break;\
          case LongLong:\
            set( get<long long>() op rh.get<long long>() );\
            break;\
          case Float:\
            set( get<float>() op rh.get<float>() );\
            break;\
          case Double:\
            set( get<double>() op rh.get<double>() );\
            break;\
          case LongDouble:\
            set( get<long double>() op rh.get<long double>() );\
            break;\
          default:\
            assert(0);\
        }\
        return *this;\

    Value& operator-=( const Value& rh ) {
        MATH_OP(-)
    }
    Value& operator+=( const Value& rh ) {
        MATH_OP(+)
    }
    Value& operator*=( const Value& rh ) {
        MATH_OP(*)
    }

#define BIT_OP(op)\
        assert( getType() == rh.getType() );\
        switch( getType() ) {\
          case Short:\
            set( get<short>() op rh.get<short>() );\
            break;\
          case Int:\
            set( get<int>() op rh.get<int>() );\
            break;\
          case Long:\
            set( get<long>() op rh.get<long>() );\
            break;\
          case LongLong:\
            set( get<long long>() op rh.get<long long>() );\
            break;\
          default:\
            assert(0);\
        }\
        return *this;\

    Value& operator&=( const Value& rh ) {
        BIT_OP(&)
    }
    Value& operator|=( const Value& rh ) {
        BIT_OP(|)
    }
    Value& operator^=( const Value& rh ) {
        BIT_OP(^)
    }

    template< class TYPE >
    TYPE get( ) const { return *(TYPE*) m_ptr; }

    void set( float value ) { *(float*) m_ptr = value; }
    void set( double value ) { *(double*) m_ptr = value; }
    void set( long double value ) { *(long double*) m_ptr = value; }

    void set( short value )     { *(short*) m_ptr = value; }
    void set( int value )       { *(int*) m_ptr = value; }
    void set( long value )      { *(long*) m_ptr = value; }
    void set( long long value ) { *(long long*) m_ptr = value; }

    Type getType() const { return m_type; }
    void* getPtr()       { return m_ptr; }

    size_t getLength() const { return m_length; }
    static size_t getLength( Type type )  {
        switch ( type ) {
            case Float: return sizeof(float);
            case Double: return sizeof(double);
            case LongDouble: return sizeof(long double);
            case Short: return sizeof(short);
            case Int: return sizeof(int);
            case Long: return sizeof(long);
            case LongLong: return sizeof(long long);
            default: assert(0);
        }
    }

    friend std::ostream& operator<<(std::ostream& stream, const Value& value );

  private:
    void copy( Value& dest, const Value& src ) {

		if ( NULL == src.m_ptr ) {
			return;
		}
        if ( dest.m_type == Empty ) {
            dest.m_type = src.m_type;
            dest.m_length = src.getLength();
            if ( src.m_value.size() ) {
                dest.m_value.resize( src.getLength() );
                dest.m_ptr = &dest.m_value[0];
            } else {
                dest.m_ptr = src.m_ptr;
            }
        }
        assert( dest.m_ptr );
        assert( dest.m_type == src.m_type );

        if ( dest.m_ptr != src.m_ptr ) {
            memcpy( dest.m_ptr, src.m_ptr, getLength(dest.m_type) );
        }
    }

    Type    m_type;
    void*   m_ptr;
    size_t  m_length;
    std::vector<uint8_t> m_value;
};

inline std::ostream& operator<<(std::ostream& stream, const Value& value ) {
    switch ( value.getType() ) {
        case Value::Short:
            stream << value.get<short>();
            break;
        case Value::Int:
            stream << value.get<int>();
            break;
        case Value::Long:
            stream << value.get<long>();
            break;
        case Value::LongLong:
            stream << value.get<long long>();
            break;
        case Value::Float:
            stream << value.get<float>();
            break;
        case Value::Double:
            stream << value.get<double>();
            break;
        case Value::LongDouble:
            stream << value.get<long double>();
            break;
        default:
            assert(0);
    }
    return stream;
}

#define OPERATOR(op) \
    assert( lh.getType() == rh.getType() );\
    switch ( lh.getType() ) {\
      case Value::Float:\
        return lh.get<float>() op rh.get<float>();\
      case Value::Double:\
        return lh.get<double>() op rh.get<double>();\
      case Value::LongDouble:\
        return lh.get<long double>() op rh.get<long double>();\
      case Value::Short:\
        return lh.get<short>() op rh.get<short>();\
      case Value::Int:\
        return lh.get<int>() op rh.get<int>();\
      case Value::Long:\
        return lh.get<long>() op rh.get<long>();\
      case Value::LongLong:\
        return lh.get<long long>() op rh.get<long long>();\
      default:\
        assert(0);\
    }\

inline bool operator!=(const Value& lh, const Value& rh ) {
    OPERATOR(!=)
}

inline bool operator<=(const Value& lh, const Value& rh ) {
    OPERATOR(<=)
}

inline bool operator<(const Value& lh, const Value& rh ) {
    OPERATOR(<)
}

inline bool operator==(const Value& lh, const Value& rh ) {
    OPERATOR(==)
}

inline bool operator>(const Value& lh, const Value& rh ) {
    OPERATOR(>)
}

inline bool operator>=(const Value& lh, const Value& rh ) {
    OPERATOR(>=)
}

class MemAddr {

  public:
    enum Type { Normal, Shmem };
    MemAddr( uint64_t sim, void* back, Type type = Normal) :
        type(type), simVAddr(sim), backing(back) {}
    MemAddr( Type type = Normal) : type(type), simVAddr(0), backing(NULL) {}
    MemAddr( void* backing ) : type(Normal), simVAddr(0), backing(backing) {}

    char& operator[](size_t index) {
        return ((char*)backing)[index];
    }

    template < class TYPE = char >
    TYPE& at(size_t index) {
        return ((TYPE*)backing)[index];
    }

    template < class TYPE = char >
    MemAddr offset( size_t val ) {
        MemAddr addr = *this;
        addr.simVAddr += val * sizeof(TYPE);
        if ( addr.backing ) {
            addr.backing = (char*) addr.backing + (val*sizeof(TYPE));
        }
        return addr;
    }

    template< class TYPE >
    uint64_t getSimVAddr( size_t offset = 0 ) const {
        return simVAddr + offset * sizeof( TYPE );
    }

    uint64_t getSimVAddr( size_t offset = 0) const {
        return simVAddr + offset;
    }

    void setSimVAddr( uint64_t addr ) {
        simVAddr = addr;
    }

    void* getBacking( size_t offset = 0 ) const {
	   	void* ptr = NULL;
		if ( backing ) {
        	ptr =  (uint8_t*) backing + offset;
		}
		return ptr;
    }
    void setBacking( void* ptr ) {
        backing = ptr;
    }

  private:
    Type type;
    uint64_t simVAddr;
    void*	 backing;
};

class NodePerf : public Module {
  public:
    virtual double getFlops() { assert(0); }
    virtual double getBandwidth() { assert(0); }
    virtual double calcTimeNS_flops( int instructions ) { assert(0); }
    virtual double calcTimeNS_bandwidth( int bytes ) { assert(0); }
};

class OS : public SubComponent {
  public:

	SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Hermes::OS)

	OS( ComponentId_t id, Params& ) : SubComponent( id ) {}

    virtual void _componentInit( unsigned int phase ) {}
    virtual void _componentSetup( void ) {}
    virtual void printStatus( Output& ) {}
    virtual int  getRank() { assert(0); }
    virtual int  getNodeNum() { assert(0); }
    virtual void finish() {}
    virtual NodePerf* getNodePerf() { return NULL; }
    virtual Thornhill::DetailedCompute* getDetailedCompute() { return NULL; }
    virtual Thornhill::MemoryHeapLink*  getMemHeapLink() { return NULL; }
};

class Interface : public SubComponent {
  public:
	SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Hermes::Interface)
    Interface( ComponentId_t id ) : SubComponent(id) {}
    virtual void setup() {}
    virtual void finish() {}
    virtual void setOS( OS* ) { assert(0); }
    virtual std::string getName() { assert(0); }
	virtual std::string getType() { return ""; }
};

}

}

#endif
