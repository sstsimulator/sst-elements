#ifndef _callback_h
#define _callback_h

namespace SST {
namespace Portals4 {

class CallbackBase {
  public:
    virtual bool operator()() = 0;
    virtual ~CallbackBase() {}
};

template <class X, class Y = void > class Callback : public CallbackBase
{
    typedef bool (X::*FuncPtr)(Y*);

  public:
    Callback( X* object, FuncPtr funcPtr, Y* data = 0 ) :
        m_object( object ),
        m_funcPtr( funcPtr ),
        m_data( data ),
        done( false )
    {
    }
    virtual bool operator()() {
        return (*m_object.*m_funcPtr)( m_data );
    }
    bool done;
  private:
    X*      m_object;
    FuncPtr m_funcPtr;
    Y*      m_data;
};

}
}

#endif
