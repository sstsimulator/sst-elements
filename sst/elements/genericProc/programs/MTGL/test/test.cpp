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
/*! \file test.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

template <typename T1, typename T2>
class A {
public:
  A(T1 t1, T2 t2) : data1(t1), data2(t2) {}

  void set(T1 a, T2 b)
  {
    data1 = a;
    data2 = b;
  }

  template <typename U, typename V>
  friend inline void f(A<U, V>& ref, U a, T2 b)
  {
    ref.set(a, b);
  }

  void operator()(T1 a, T2 b)
  {
    f(*this, a, b);
  }

private:
  T1 data1;
  T2 data2;
};

template <typename U>
inline void f(A<U, int>& ref, U a, int b)
{
  ref.set(a, b);
}

int main()
{
  A<int, double> a1(1, 2.3);
  A<int, int> a2(1, 2);
  a1(2, 4.6);
  a2(3, 8);

  return 0;
}
