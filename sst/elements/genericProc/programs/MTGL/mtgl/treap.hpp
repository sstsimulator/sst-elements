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
/*! \file treap.hpp

    \brief An MTA/XMT treap data structure.

    \author Kamesh Madduri

    \date 10/20/2009
*/
/****************************************************************************/

#ifndef MTGL_TREAP
#define MTGL_TREAP

#include <cstdlib>
#include <cstdio>

#include <mtgl/util.hpp>

namespace mtgl {

class Randomizer {
private:
  enum { length = (1 << 20) - 1 };
  unsigned* vector;

public:
  Randomizer();
  unsigned randomize(unsigned uv) const;
};

extern Randomizer randomizer;

#define priority(x) randomizer.randomize(unsigned(x))

#define gt_priority(x, y) (priority(x) > priority(y) ||  \
                          (priority(x) == priority(y) && \
                           unsigned(x) > unsigned(y)))

#define ge_priority(x, y) ((x) == (y) || gt_priority(x, y))


Randomizer::Randomizer()
{
  vector = new unsigned[length];

  #pragma mta assert parallel
  for (unsigned i = 0; i < length; i++) vector[i] = random();
}


unsigned Randomizer::randomize(unsigned uv) const
{
  unsigned hv = vector[uv % length];
  return hv ^ uv;
}

Randomizer randomizer = Randomizer();

template <class T>
class Treap {
private:
  struct TreapNode;
  TreapNode* root;

  enum { hash_size = (1 << 16) - 1 };

  static TreapNode** table;
  static TreapNode* newTreapNode(Treap lt, T key, unsigned n, Treap gt);
  static void free_node(TreapNode* n);

  static TreapNode** retable;
  static TreapNode* recycled_node();
  static void recycle_node(TreapNode*);

  static unsigned split(Treap* lp, Treap* gp, Treap r, T key);
  static Treap join(Treap lt, Treap gt);
  static Treap subdiff(Treap r1, Treap r2, bool subr2);

  Treap(Treap lt, T key, unsigned n, Treap gt);

public:
  Treap();
  explicit Treap(T);
  Treap(T, unsigned);
  Treap(const Treap&);
  Treap& operator=(const Treap&);
  ~Treap();

  Treap(T*, unsigned);

  unsigned size() const;                  // O(1) time
  bool empty() const;                     // O(1) time
  T chooseone() const;                    // O(1) time

  unsigned member(T) const;               // O(log n) time
  Treap include(T) const;                 // O(log n) time
  Treap exclude(T) const;                 // O(log n) time

  Treap operator<<(T) const;              // Insert.
  Treap operator>>(T) const;              // Delete.

  Treap operator&(Treap) const;           // Intersect.
  Treap operator|(Treap) const;           // Union.
  Treap operator-(Treap) const;           // Difference.
  Treap operator^(Treap) const;           // Xor.

  Treap& operator&=(Treap);
  Treap& operator|=(Treap);
  Treap& operator-=(Treap);
  Treap& operator^=(Treap);
  Treap& operator<<=(T);
  Treap& operator>>=(T);

#ifndef __MTA__
  TreapNode* readfe(TreapNode** vp);
  void writeef(TreapNode** vp, TreapNode* val);
#endif

  operator unsigned() const;
};

template <class T>
struct Treap<T>::TreapNode {
  unsigned refs;
  unsigned hnum;
  TreapNode* relink;
  TreapNode* next;
  Treap lt;
  Treap gt;
  unsigned size;
  unsigned count;
  T key;
};

template <class T>
Treap<T>::Treap()
{
  root = 0;
}

template <class T>
Treap<T>::Treap(T m)
{
  root = newTreapNode(Treap(), m, 1, Treap());
}

template <class T>
Treap<T>::Treap(T m, unsigned n)
{
  root = newTreapNode(Treap(), m, n, Treap());
}

template <class T>
Treap<T>::Treap(Treap lt, T key, unsigned n, Treap gt)
{
  root = newTreapNode(lt, key, n, gt);
}

template <class T>
Treap<T>::Treap(const Treap& s)
{
  root = s.root;
  if (root) (void) mt_incr(root->refs, 1);
}

template <class T>
Treap<T>::Treap(T* v, unsigned n)
{
  if (n == 0)
  {
    root = 0;
  }
  else if (n == 1)
  {
    root = newTreapNode(Treap(), v[0], 1, Treap());
  }
#ifdef __MTA__
  else if (n > 20)
  {
    unsigned n2 = n / 2;
    Treap s1;
    future void ready$;
    future ready$(& s1, v, n2)
    {
      s1 = Treap(v, n2);
    }
    Treap s2 = Treap(v + n2, n - n2);
    touch(&ready$);
    Treap s = s1 | s2;
    root = s.root;
    (void) mt_incr(root->refs, 1);
  }
#endif
  else
  {
    unsigned n2 = n / 2;
    Treap s = Treap(v, n2) | Treap(v + n2, n - n2);
    root = s.root;
    (void) mt_incr(root->refs, 1);
  }
}

template <class T>
Treap<T>&Treap<T>::operator=(const Treap& s)
{
  if (root == s.root) return *this;

  free_node(root);
  root = s.root;

  if (root) (void) mt_incr(root->refs, 1);

  return *this;
}

template <class T>
Treap<T>::~Treap()
{
  free_node(root);
}

template <class T>
Treap<T>::operator unsigned() const
{
  return unsigned(root);
}

template <class T>
unsigned Treap<T>::size() const
{
  if (root)
  {
    return root->size;
  }
  else
  {
    return 0;
  }
}

template <class T>
bool Treap<T>::empty() const
{
  return root == 0;
}

template <class T>
T Treap<T>::chooseone() const
{
  if (root)
  {
    return root->key;
  }
  else
  {
    return -1; // throw Empty_Treap_Error();
  }
}

template <class T>
unsigned Treap<T>::member(T m) const
{
  if (root)
  {
    if (gt_priority(m, root->key)) return 0;

    if (unsigned(m) < unsigned(root->key))
    {
      return root->lt.member(m);
    }
    else if (unsigned(m) > unsigned(root->key))
    {
      return root->gt.member(m);
    }
    else
    {
      return root->count;
    }
  }
  else
  {
    return 0;
  }
}

template <class T>
Treap<T> Treap<T>::operator&(Treap r2) const
{
  Treap r1 = *this;

  if (r1.empty() || r2.empty()) return Treap();

  if (r1 == r2) return r1;

  if (gt_priority(r2.root->key, r1.root->key))
  {
    Treap r = r1;
    r1 = r2;
    r2 = r;
  }

  Treap lt0, gt0, lt1, gt1;
  unsigned n = split(&lt0, &gt0, r2, r1.root->key);

#ifdef __MTA__
  if (r1.size() + r2.size() > 30)
  {
    future void ready$;
    future ready$(& lt1, & r1, & lt0)
    {
      lt1 = r1.root->lt & lt0;
    }
    gt1 = r1.root->gt & gt0;
    touch(&ready$);
  }
  else
#endif
  {
    lt1 = r1.root->lt & lt0;
    gt1 = r1.root->gt & gt0;
  }

  if (n > r1.root->count) n = r1.root->count;

  if (n > 0)
  {
    return Treap(lt1, r1.root->key, n, gt1);
  }
  else
  {
    return join(lt1, gt1);
  }
}

template <class T>
Treap<T> Treap<T>::operator^(Treap r2) const
{
  Treap r1 = *this;
  if (r1.empty()) return r2;
  if (r2.empty()) return r1;
  if (r1 == r2) return Treap();

  if (gt_priority(r2.root->key, r1.root->key))
  {
    Treap r = r1;
    r1 = r2;
    r2 = r;
  }

  Treap lt0, gt0, lt1, gt1;
  unsigned n = split(&lt0, &gt0, r2, r1.root->key);

#ifdef __MTA__
  if (r1.size() + r2.size() > 30)
  {
    future void ready$;
    future ready$(& lt1, & r1, & lt0)
    {
      lt1 = r1.root->lt ^ lt0;
    }
    gt1 = r1.root->gt ^ gt0;
    touch(&ready$);
  }
  else
#endif
  {
    lt1 = r1.root->lt ^ lt0;
    gt1 = r1.root->gt ^ gt0;
  }

  if (n > r1.root->count)
  {
    return Treap(lt1, r1.root->key, n - r1.root->count, gt1);
  }
  else if (n < r1.root->count)
  {
    return Treap(lt1, r1.root->key, r1.root->count - n, gt1);
  }
  else
  {
    return join(lt1, gt1);
  }
}

template <class T>
Treap<T> Treap<T>::operator|(Treap r2) const
{
  Treap r1 = *this;

  if (r1.empty()) return r2;
  if (r2.empty()) return r1;
  if (r1 == r2) return r1;

  if (gt_priority(r2.root->key, r1.root->key))
  {
    Treap r = r1;
    r1 = r2;
    r2 = r;
  }

  Treap lt0, gt0, lt1, gt1;
  unsigned n = split(&lt0, &gt0, r2, r1.root->key);

#ifdef __MTA__
  if (r1.size() + r2.size() > 30)
  {
    future void ready$;
    future ready$(& lt1, & r1, & lt0)
    {
      lt1 = r1.root->lt | lt0;
    }
    gt1 = r1.root->gt | gt0;
    touch(&ready$);
  }
  else
#endif
  {
    lt1 = r1.root->lt | lt0;
    gt1 = r1.root->gt | gt0;
  }

  n += r1.root->count;

  return Treap(lt1, r1.root->key, n, gt1);
}

template <class T>
Treap<T> Treap<T>::operator-(Treap s) const
{
  return subdiff(*this, s, true);
}

template <class T>
Treap<T> Treap<T>::include(T m) const
{
  if (!root)
  {
    return Treap(m);
  }
  else if (unsigned(m) < unsigned(root->key))
  {
    Treap nlt = root->lt.include(m);

    if (ge_priority(root->key, nlt.root->key))
    {
      return Treap(nlt, root->key, root->count, root->gt);
    }
    else
    {
      return Treap(nlt.root->lt, nlt.root->key, nlt.root->count,
                   Treap(nlt.root->gt, root->key, root->count, root->gt));
    }
  }
  else if (unsigned(m) > unsigned(root->key))
  {
    Treap ngt = root->gt.include(m);

    if (ge_priority(root->key, ngt.root->key))
    {
      return Treap(root->lt, root->key, root->count, ngt);
    }
    else
    {
      return Treap(Treap(root->lt, root->key, root->count, ngt.root->lt),
                   ngt.root->key, ngt.root->count, ngt.root->gt);
    }
  }
  else
  {
    return Treap(root->lt, root->key, root->count + 1, root->gt);
  }
}

template <class T>
Treap<T> Treap<T>::operator<<(T m) const
{
  return include(m);
}

template <class T>
Treap<T> Treap<T>::exclude(T m) const
{
  if (!root || gt_priority(m, root->key))
  {
    return *this;
  }
  else if (unsigned(m) < unsigned(root->key))
  {
    return Treap(root->lt.exclude(m), root->key, root->count, root->gt);
  }
  else if (unsigned(m) > unsigned(root->key))
  {
    return Treap(root->lt, root->key, root->count, root->gt.exclude(m));
  }
  else if (root->count > 1)
  {
    return Treap(root->lt, root->key, root->count - 1, root->gt);
  }
  else
  {
    return join(root->lt, root->gt);
  }
}

template <class T>
Treap<T> Treap<T>::operator>>(T m) const
{
  return exclude(m);
}

template <class T>
Treap<T>&Treap<T>::operator&=(Treap s)
{
  *this = *this & s;
  return *this;
}

template <class T>
Treap<T>&Treap<T>::operator|=(Treap s)
{
  *this = *this | s;
  return *this;
}

template <class T>
Treap<T>&Treap<T>::operator-=(Treap s)
{
  *this = *this - s;
  return *this;
}

template <class T>
Treap<T>&Treap<T>::operator^=(Treap s)
{
  *this = *this ^ s;
  return *this;
}

template <class T>
Treap<T>&Treap<T>::operator<<=(T m)
{
  *this = include(m);
  return *this;
}

template <class T>
Treap<T>&Treap<T>::operator>>=(T m)
{
  *this = exclude(m);
  return *this;
}

//-----------------------------------------------------------------------------
// hash consing support

template <class T>
typename Treap<T>::TreapNode * *Treap<T>::table =
  (TreapNode**) calloc(hash_size, sizeof(TreapNode*));

template <class T>
typename Treap<T>::TreapNode*
Treap<T>::newTreapNode(Treap lt, T key, unsigned n, Treap gt)
{
  TreapNode* q = recycled_node();

  unsigned hash = priority(lt.root) + priority(gt.root) + priority(key) +
                  priority(n);

  hash %= hash_size;

  TreapNode* tempNode = mt_readfe(table[hash]);
  TreapNode* p = tempNode;

  while (p)
  {
    unsigned r = mt_incr(p->refs, 1);

    if (r > 0 && p->key == key && p->lt == lt && p->gt == gt && p->count == n)
    {
      mt_write(table[hash], tempNode);
      recycle_node(q);

      return p;
    }
    else if (r == 0)
    {
      (void) mt_incr(p->refs, -1);
    }
    else
    {
      free_node(p);
    }

    p = p->next;
  }

  q->next = tempNode;
  tempNode = q;
  q->lt = lt;
  q->gt = gt;
  q->key = key;
  q->count = n;
  q->refs = 1;
  q->hnum = hash;
  q->size = lt.size() + gt.size() + n;

  mt_write(table[hash], tempNode);

  return q;
}

template <class T>
void Treap<T>::free_node(TreapNode* p)
{
  if (p)
  {
    unsigned r = mt_incr(p->refs, -1);
    if (r == 1) recycle_node(p);
  }
}

template <class T>
#ifdef __MTA__
typename Treap<T>::TreapNode * *Treap<T>::retable =
  (TreapNode**) calloc(128, sizeof(TreapNode*));
#else
typename Treap<T>::TreapNode * *Treap<T>::retable =
  (TreapNode**) calloc(1, sizeof(TreapNode*));
#endif

template <class T>
typename Treap<T>::TreapNode* Treap<T>::recycled_node()
{
#ifdef __MTA__
  unsigned r = mta_clock(0) & 127;
#else
  unsigned r = 0;
#endif

  TreapNode* p = mt_readfe(retable[r]);

  if (p)
  {
    mt_write(retable[r], p->relink);
    unsigned hash = p->hnum;

    if (hash < hash_size)
    {
      TreapNode* tempNode = mt_readfe(table[hash]);

      if (p == tempNode)
      {
        mt_write(table[hash], p->next);
      }
      else
      {
        TreapNode* q = tempNode;
        while (q->next != p) q = q->next;
        q->next = p->next;
        mt_write(table[hash], tempNode);
      }
    }
  }
  else
  {
    mt_write(retable[r], p);
    p = new TreapNode;
  }

  p->hnum = hash_size;

  return p;
}

template <class T>
void Treap<T>::recycle_node(TreapNode* p)
{
#ifdef __MTA__
  unsigned r = (unsigned(p) + mta_clock(0)) & 127;
#else
  unsigned r = 0;
#endif

  TreapNode* q = mt_readfe(retable[r]);
  p->relink = q;

  (void) mt_write(retable[r], p);
}

//-----------------------------------------------------------------------------
// support for union, intersect, difference

template <class T>
unsigned Treap<T>::split(Treap* lp, Treap* gp, Treap r, T key)
{
  if (r.empty())
  {
    *lp = Treap();
    *gp = Treap();
    return 0;
  }

  if (unsigned(key) > unsigned(r.root->key))
  {
    unsigned n = split(lp, gp, r.root->gt, key);
    *lp = Treap(r.root->lt, r.root->key, r.root->count, *lp);
    return n;
  }
  else if (unsigned(key) < unsigned(r.root->key))
  {
    unsigned n = split(lp, gp, r.root->lt, key);
    *gp = Treap(*gp, r.root->key, r.root->count, r.root->gt);
    return n;
  }
  else
  {
    *lp = r.root->lt;
    *gp = r.root->gt;
    return r.root->count;
  }
}

template <class T>
Treap<T> Treap<T>::join(Treap lt, Treap gt)
{
  if (lt.empty()) return gt;
  if (gt.empty()) return lt;

  if (gt_priority(lt.root->key, gt.root->key))
  {
    return Treap(lt.root->lt, lt.root->key, lt.root->count,
                 join(lt.root->gt, gt));
  }
  else
  {
    return Treap(join(lt, gt.root->lt), gt.root->key, gt.root->count,
                 gt.root->gt);
  }
}

template <class T>
Treap<T> Treap<T>::subdiff(Treap r1, Treap r2, bool subr2)
{
  if (r1.empty() || r2.empty()) return subr2 ? r1 : r2;
  if (r1 == r2) return Treap();

  if (gt_priority(r2.root->key, r1.root->key))
  {
    Treap r = r1;
    r1 = r2;
    r2 = r;
    subr2 = !subr2;
  }

  Treap lt0, gt0, lt1, gt1;
  unsigned n = split(&lt0, &gt0, r2, r1.root->key);

#ifdef __MTA__
  if (r1.size() + r2.size() > 30)
  {
    Treap* ltp = &lt1;
    Treap z = r1.root->lt;
    future void ready$;
    future ready$(& z, & lt0, ltp, subr2)
    {
      *ltp = subdiff(z, lt0, subr2);
    }
    gt1 = subdiff(r1.root->gt, gt0, subr2);
    touch(&ready$);
  }
  else
#endif
  {
    lt1 = subdiff(r1.root->lt, lt0, subr2);
    gt1 = subdiff(r1.root->gt, gt0, subr2);
  }

  if (subr2)
  {
    if (n >= r1.root->count)
    {
      return join(lt1, gt1);
    }
    else
    {
      return Treap(lt1, r1.root->key, r1.root->count - n, gt1);
    }
  }
  else
  {
    if (n <= r1.root->count)
    {
      return join(lt1, gt1);
    }
    else
    {
      return Treap(lt1, r1.root->key, n - r1.root->count, gt1);
    }
  }
}

#ifndef __MTA__
template <class T>
typename Treap<T>::TreapNode* Treap<T>::readfe(TreapNode** vp)
{
  return *vp;
}

template <class T>
void Treap<T>::writeef(TreapNode** vp, TreapNode* val)
{
  *vp = val;
}

unsigned mt_incr(unsigned vp, int i)      // atomic fetch & add
{
  int result = vp;
  vp = result + i;
  return result;
}
#endif

}

#endif
