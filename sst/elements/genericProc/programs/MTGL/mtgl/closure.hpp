/*  _________________________________________________________________________
 *
 *  MTGL: The MultiThreaded Graph Library
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README file in the top MTGL directory.
 *  _________________________________________________________________________
 */

/****************************************************************************/
/*! \file closure.hpp

    \brief These classes provide the mechanism used by algorithms to
           interact with basic data structures in order to
           provide automatic and hidden parallelism.

    \deprecated Algorithm classes now use inner classes.  Shiloach-Vishkin
                still uses this, but shouldn't.

    \author Jon Berry (jberry@sandia.gov)
    \author Curt Janssen (cljanss@sandia.gov)

    \date 6/18/2005

    Defines the infrastructure "closure" facilities, following the C++
    standard convensions in <functional>, but not using them directly.  Most
    function pointer passing involves two argument methods, and the standard
    stops at unary methods.
*/
/****************************************************************************/

//#include <functional>  // would give mem_fun_t, mem_fun1_t, mem_fun,
// unary_function, binary_function

#ifndef MTGL_CLOSURE_HPP
#define MTGL_CLOSURE_HPP

namespace mtgl {

/// This class wraps a pointer to a unary non-member function.
template <typename res_type, typename arg_type>
class unary_fun {
public:
  unary_fun(res_type(*f)(arg_type)) : f_(f) {}

  res_type operator()(arg_type a) { return f(a); }

private:
  res_type (* f_)(arg_type);
};

/// This class wraps a pointer to a binary non-member function.
template <typename res_type, typename arg1_type, typename arg2_type>
class binary_fun {
public:
  binary_fun(res_type(*f)(arg1_type, arg2_type)) : f_(f) {}

  res_type operator()(arg1_type a1, arg2_type a2) { return f_(a1, a2); }

private:
  res_type (* f_)(arg1_type, arg2_type);
};

/// This class wraps a pointer to a unary member function.
template <typename obj_type, class res_type, typename arg_type>
class unary_mfun {
public:
  unary_mfun(obj_type* o, res_type(obj_type::* m)(arg_type) const) :
    o_(o), m_(m) {}

  unary_mfun(res_type(obj_type::* m)(arg_type) const) : o_(NULL), m_(m) {}

  res_type operator()(arg_type r) { return (o_->*m_)(r); }

  res_type operator()(obj_type* o, arg_type r) { return (o_->*m_)(r); }

private:
  res_type (obj_type::* m_)(arg_type) const;
  obj_type* o_;
};

/*! \brief This class wraps a pointer t a binary member function.

    There are two ways to construct and use instances of this
    class. The first way is to explicitly pass a pointer to an
    object in addition to a pointer to a method.  This encapsulates
    the object, obviating the need to pass it during method
    invocation.  However, it is also possible to construct
    a binary_mfun without an object, and to pass a pointer to
    an object at method invocation time.
 */
template <class obj_type, typename res_type, typename arg1_type,
          typename arg2_type>
class binary_mfun {
public:
  typedef res_type (obj_type::* mtype)(arg1_type, arg2_type) const;

  binary_mfun(obj_type* o,
              res_type(obj_type::* m)(arg1_type, arg2_type) const) :
    o_(o), m_(m) {}

  binary_mfun(res_type(obj_type::* m)(arg1_type, arg2_type) const) :
    o_(NULL), m_(m) {}

  mtype getMethod() {return m_;}

  obj_type* getObject() { return o_; }

  res_type operator()(arg1_type a1, arg2_type a2) { return (o_->*m_)(a1, a2); }

  res_type operator()(obj_type* o, arg1_type a1, arg2_type a2)
  {
    return (o_->*m_)(a1, a2);
  }

public: // need to figure out how to declare accessor methods!
  res_type (obj_type::* m_)(arg1_type, arg2_type) const;
  obj_type* o_;
};

/*! \brief This function provides a way to obtain a unary_mfun object.
    \param o A pointer to an object
    \param pm A pointer to a method of that object

    This function constructs a unary_mfun object that encapsulates
    an object reference.
 */
template <class obj, typename res_type, typename arg_type>
unary_mfun<obj, res_type, arg_type> mem_fun(
    obj* o, res_type (obj::* pm)(arg_type) const)
{
  return unary_mfun<obj, res_type, arg_type>(o, pm);
}

/*! \brief This function provides a way to obtain a unary_mfun object.
    \param pm A pointer to a method

    This function constructs a unary_mfun object that does not
    encapsulate an object reference. The object reference (pointer)
    will be passed at invocation time.
 */
template <class obj, typename res_type, typename arg_type>
unary_mfun<obj, res_type, arg_type> mem_fun(
    res_type (obj::* pm)(arg_type) const)
{
  return unary_mfun<obj, res_type, arg_type>(pm);
}

/*! \brief This function provides a way to obtain a unary_fun object.
    \param pf A pointer to a non-member function

    This function constructs a unary_fun object.
 */
template <typename res_type, typename arg_type>
unary_fun<res_type, arg_type> fun(res_type (* pf)(arg_type))
{
  return unary_fun<res_type, arg_type>(pf);
}

/*! \brief This function provides a way to obtain a binary_mfun object.
    \param pm A pointer to a method

    This function constructs a binary_mfun object that does not
    encapsulate an object reference. The object reference (pointer)
    will be passed at invocation time.
 */
template <class obj, typename res_type, typename arg1_type, typename arg2_type>
binary_mfun<obj, res_type, arg1_type, arg2_type>
mem_fun(res_type (obj::* pm)(arg1_type, arg2_type) const)
{
  return binary_mfun<obj, res_type, arg1_type, arg2_type>(pm);
}

/*! \brief This function provides a way to obtain a binary_mfun object
           that encapsulate an object reference, and hence
           does not need to be passed it at method invocation time.
    \param o A pointer to a method
    \param pm A pointer to a method

    This function constructs a binary_mfun object that does not
    encapsulate an object reference. The object reference (pointer)
    will be passed at invocation time.
 */
template <class obj, typename res_type, typename arg1_type, typename arg2_type>
binary_mfun<obj, res_type, arg1_type, arg2_type>
mem_fun(obj* o, res_type (obj::* pm)(arg1_type, arg2_type) const)
{
  return binary_mfun<obj, res_type, arg1_type, arg2_type>(o, pm);
}

/*! \brief This function provides a way to obtain a binary_fun object.
    \param pm A pointer to a non-member function
 */
template <typename res_type, typename arg1_type, typename arg2_type>
binary_fun<res_type, arg1_type, arg2_type>
fun(res_type (* pf)(arg1_type, arg2_type))
{
  return binary_fun<res_type, arg1_type, arg2_type>(pf);
}

/*! \brief This class provides an interface to two child classes: mclosure
           (which encapsulates a method and some environment inforamtion),
           and fclosure (which encapsulates a non-member function and some
           environment inforamtion).
 */
template <typename arg_type, typename env_type = int>
class closure {
public:
  closure(const closure<arg_type, env_type>& cl) : data(cl.data),
                                                   cptr(cl.cptr) {}

  virtual void operator()(arg_type x) { cptr->operator()(x, data); }

  virtual void operator()(arg_type x, env_type d) {}

  void set_data(env_type d) { data = d; }

protected:
  closure(closure<arg_type, env_type>* cp, env_type d) : data(d), cptr(cp) {}
  env_type data;
  closure<arg_type, env_type>* cptr;
};

/*! \brief This class encapsulates the inforamtion needed to store a method
           and some environment information for later invocation.  At
           invocation time, an argument will be expected, and will become the
           first argument to the method call.  The second argument will be
           the environment data.
 */
template <class obj_type, typename arg_type, typename env_type = int>
class mclosure {
protected:
  void (obj_type::* m_)(arg_type, env_type) const;
  obj_type* o_;         // Can't declare as object - incomplete type.
  env_type data;
  bool delete_obj;

public:
  mclosure(const binary_mfun<obj_type, void, arg_type, env_type>& mf,
           env_type d) : o_(mf.o_), m_(mf.m_), data(d) , delete_obj(false) {}

  mclosure(const mclosure& mc) : m_(mc.m_), data(mc.data)
  {
    o_ = new obj_type(*mc.o_);
    delete_obj = true;
  }

  ~mclosure() { if (delete_obj) delete o_; }

  void operator()(arg_type a1) const { return (o_->*m_)(a1, data); }

  void operator()(arg_type x, env_type d) { return (o_->*m_)(x, d); }

  void set_data(env_type d) { data = d; }
};

/*! \brief This class encapsulates the inforamtion needed to store a
           non-member function and some environment information for later
           invocation.  At invocation time, an argument will be expected, and
           will become the first argument to the method call.  The second
           argument will be the environment data.
 */
template <typename arg_type, typename env_type = int>
class fclosure : public closure<arg_type, env_type> {
protected:
  binary_fun<void, arg_type, env_type> fcn;

public:
  fclosure(const binary_fun<void, arg_type, env_type>& mf, env_type d) :
    fcn(mf), closure<arg_type, env_type>(this, d) {}

  void operator()(arg_type x) { fcn(x, closure<arg_type, env_type>::data); }

  void operator()(arg_type x, env_type d) { fcn(x, d); }
};

}

#endif

// ***********************************************************************
// ** example usage: *****************************************************
// ***********************************************************************
/*
  using namespace mtgl;

  void func(int i, double d)
  {
    cout << i << "," << d << endl;
  }

  int main()
  {
    A alg;

    // The Stroustrup way
    unary_mfun<A,void,int> mf1 = mem_fun(&A::g);
    mf1(&alg, 3);
    // output:  A::g(3)

    // The Janssen way
    unary_mfun<A,void,int> mf1b = mem_fun(&alg, &A::g);
    mf1b(3);
    // output:  A::g(3)

    binary_mfun<A, void, int, double> mf2 = mem_fun(&alg, &A::h);
    mf2(8, 23.3);  // output: A::h(8,23.3);
    binary_mfun<A, void, int, double> mf2b = mem_fun(&A::h);
    mf2b(&alg, 8, 23.3);  // output: A::h(8,23.3);


    mclosure<A,int,double> testmf(mem_fun(&alg, &A::h), 23.3);
    mclosure<A,const int&,double> testmf2(mem_fun(&alg, &A::f), 23.3);
    fclosure<int,double> testf(fun(&func), 23.3);
    dynamic_array<int> a(3);
    for (int i=0; i<3; i++) a[i] = i;
    a.visit(testmf);
    a.visit(testmf2);
    a.visit(testf);
    return 0;
  }
 */
