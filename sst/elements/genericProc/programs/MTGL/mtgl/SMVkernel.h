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
/*! \file SMVkernel.h

    \brief Thread-safe sparse matrix data structures and algorithms.

    \author Robert Heaphy

    \date 2005
*/
/****************************************************************************/

#ifndef MTGL_SMVKERNEL_H
#define MTGL_SMVKERNEL_H

#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <mtgl/util.hpp>

// Disable the warning about unknown pragmas.
#ifndef __MTA__
#pragma warning ( disable : 4068 )
#endif

#ifdef __MTA__
 #define BRAKET <size_type, T>
 #include <sys/mta_task.h>
 #include <machine/runtime.h>
#else
 #define BRAKET <size_type, T>
#endif

// Current ISSUE: error handling is erratic.
// Current ISSUE: look for NOT IMPLEMENTED and implement.

// Base classes-forward declarations.
template <typename size_type, typename T> class VectorBase;
template <typename size_type, typename T> class MatrixBase;

// Derived classes-forward declarations.
template <typename size_type, typename T> class DenseVector;

template <typename size_type, typename T> class SparseMatrixCSR;
template <typename size_type, typename T> class SparseMatrixCSC;
template <typename size_type, typename T> class SparseMatrixCOO;

template <typename size_type, typename T>
DenseVector<size_type, T>
operator*(const SparseMatrixCSR<size_type, T>& a,
          const DenseVector<size_type, T>& b);

template <typename size_type, typename T>
DenseVector<size_type, T>
diagonal(const SparseMatrixCSR<size_type, T>& a);

template <typename size_type, typename T>
DenseVector<size_type, T>
Transpose_SMVm(const SparseMatrixCSR<size_type, T>&,
               const DenseVector<size_type, T>&);

template <typename size_type, typename T>
DenseVector<size_type, T>
operator*(const SparseMatrixCSC<size_type, T>&,
           const DenseVector<size_type, T>&);

template <typename size_type, typename T>
DenseVector<size_type, T>
operator*(const SparseMatrixCOO<size_type, T>&,
          const DenseVector<size_type, T>&);

template <typename size_type, typename T>
DenseVector<size_type, T>
operator*(size_type const, const DenseVector<size_type, T>&);

template <typename size_type, typename T>
DenseVector<size_type, T>
operator*(double const, const DenseVector<size_type, T>&);

template <typename size_type, typename T>
SparseMatrixCSR<size_type, T>
operator*(size_type const, const SparseMatrixCSR<size_type, T>&);

template <typename size_type, typename T>
SparseMatrixCSR<size_type, T>
operator* (double const, const SparseMatrixCSR<size_type, T>&);

template <typename size_type, typename T>
T
operator*(const DenseVector<size_type, T>&, const DenseVector<size_type, T>&);

template <typename size_type, typename T>
DenseVector<size_type, T>
diagonal(const SparseMatrixCSR<size_type, T>&);

template <typename size_type, typename T>
DenseVector<size_type, T>
Transpose_SMVm(const SparseMatrixCSR<size_type, T>&,
               const DenseVector<size_type, T>&);

/***********************  Base Classes  ****************************/

template <typename size_type, typename T> class VectorBase {
protected:
  size_type length;
  T* values;                          // Vector elements.

public:
  VectorBase() : length(0), values(0)  {}

  VectorBase (size_type const n) : length(n)
  {
    this->values = (T*) malloc (n * sizeof(T));

    T* const this_values = this->values;    // Force MTA to place on stack.
    T const zero = T();                     // Force MTA to place on stack.
    size_type const finish = this->length;  // Force MTA to place on stack.

    #pragma mta assert parallel
    for (size_type i = 0; i < finish; i++) this_values[i] = zero;
  }

  VectorBase (const VectorBase<size_type, T>& a) : length(a.length)
  {
    this->values = (T*) malloc (a.length * sizeof(T));

    size_type const stop = a.length;       // Force MTA to place on stack.
    T* const this_values = this->values;   // Force MTA to place on stack.
    T* const a_values = a.values;          // Force MTA to place on stack.

    #pragma mta assert parallel
    for (size_type i = 0; i < stop; i++) this_values[i] = a_values[i];
  }

  ~VectorBase()
  {
    if (this->values) free (this->values);
    this->values = NULL;
  }

  void clear()
  {
    if (this->values) free(this->values);
    this->values = NULL;
  }

  void VectorPrint (char const* name) const;
};

template <typename size_type, typename T> class MatrixBase {
protected:
  size_type nRow, nCol, nNonZero;  // Number of rows, columns, and non zeros.
  T* values;                       // Non-zero matrix elements.
  size_type* index;                // Generally used for sparse storage index.
  bool data_owned;

public:
  MatrixBase() : nRow(0), nCol(0), nNonZero(0), values(0), index(0),
                 data_owned(true) {}

  MatrixBase(size_type const row, size_type const col,
             size_type const count, bool own = true) :
    nRow(row), nCol(col), nNonZero(count), index(0), data_owned(own)
  {
//    printf("MatrixBase()\n");
    values = (T*) malloc (count * sizeof(T));
//    printf("MatrixBase() alloc'd\n");

    T* const this_values = this->values;      // Force MTA to place on stack.
    size_type const finish = this->nNonZero;

    #pragma mta assert parallel
    for (size_type i = 0; i < finish; i++) this_values[i] = i;
  }

  ~MatrixBase()
  {
    if (data_owned && this->values) free(this->values);
    this->values = 0;

    if (data_owned && this->index) free(this->index);
    this->index  = 0;
  }

  MatrixBase(const MatrixBase<size_type, T>& a) :
    nRow(a.nRow), nCol(a.nCol), nNonZero(a.nNonZero),
    index(0), data_owned(true)        // not debugged!
  {
    this->values = (T*) malloc (a.nNonZero * sizeof (T));
    T* const this_values = this->values;  // Force MTA to place on stack.
    T* const a_values = a.values;         // Force MTA to place on stack.
    size_type const stop = a.nNonZero;

    #pragma mta assert parallel
    for (size_type i = 0; i < stop; i++) this_values[i] = a_values[i];
  }

  void MatrixPrint (char const* name);
  size_type num_nonzero() const { return this->nNonZero; }

  T* swap_values(T* new_values)
  {
    T* tmp = values;
    values = new_values;
    return tmp;
  }

  size_type* get_index() const { return index; }
};

/***************************  DenseVector *************************************/

template <typename size_type, typename T = double>
class DenseVector : public VectorBase<size_type, T> {
public:
  DenseVector() {}
  DenseVector (size_type const n) : VectorBase<size_type, T> (n) {}
  DenseVector (const DenseVector<size_type, T>& a) :
    VectorBase<size_type, T> (a) {}

  ~DenseVector() {}

  DenseVector<size_type, T>& operator=(const DenseVector<size_type, T>& a)
  {
    if (this->length != a.length) printf("DenseVector copy assignment error\n");

    if (this != &a)
    {
      size_type const stop = this->length;   // Force MTA to place on stack.
      T*  const this_values = this->values;  // Force MTA to place on stack.
      T*  const a_values = a.values;         // Force MTA to place on stack.

      #pragma mta assert no dependence
      for (size_type i = 0; i < stop; i++) this_values[i] = a_values[i];
    }

    return *this;
  }

  const T operator[](size_type idx)
  {
    if (idx < 0 || idx > this->length)
    {
      fprintf(stderr, "error: index out of bounds in DenseVector\n");
      exit(1);
    }

    return(this->values[idx]);
  }

  DenseVector<size_type, T>& operator+=(const DenseVector<size_type, T>& a)
  {
    size_type const stop = this->length;  // Force MTA to place on stack.
    T* const this_values = this->values;  // Force MTA to place on stack.
    T* const a_values = a.values;         // Force MTA to place on stack.

    #pragma mta assert parallel
    for (size_type i = 0; i < stop; i++)
    {
//      safe_incr(this_values[i], a_values[i]);
      this_values[i] += a_values[i];
    }

    return *this;
  }

  DenseVector<size_type, T>& operator-=(const DenseVector<size_type, T>& a)
  {
    size_type const stop = this->length;    // Force MTA to place on stack.
//    T* const this_values = this->values;    // Force MTA to place on stack.
//    T* const a_values = a.values;           // Force MTA to place on stack.

    #pragma mta assert parallel
    for (size_type i = 0; i < stop; i++)
    {
      // safe_incr(this->values[i], -a.values[i]);
      this->values[i] += -a.values[i];
    }

    return *this;
  }

  DenseVector<size_type, T>& operator() ()
  {
    size_type const stop = this->length;  // Force MTA to place on stack.
    T* const this_values = this->values;  // Force MTA to place on stack.
    T const zero = T();

    #pragma mta assert no dependence
    for (size_type i = 0; i < stop; i++) this_values[i] = zero;

    return *this;
  }

  T norm2 () const
  {
    T temp = T();
    size_type const stop = this->length;
    T* const this_values = this->values;  // Force MTA to place on stack.

    #pragma mta assert parallel
    for (size_type i = 0; i < stop; i++)
    {
      // safe_incr(temp, this_values[i] * this_values[i]);
      temp += this_values[i] * this_values[i];
    }

    return (T) sqrt(temp);
  }

  T norm_inf () const
  {
    T temp = T();
    size_type const stop = this->length;
    T* const this_values = this->values;  // Force MTA to place on stack.

    #pragma mta assert parallel
    for (size_type i = 0; i < stop; i++)
    {
      if (fabs(this_values[i]) > temp) temp = (T) fabs(this_values[i]);
    }

    return temp;
  }

  size_type VectorLength() const { return this->length; }

  friend DenseVector<size_type, T>
  operator* BRAKET (const SparseMatrixCSR<size_type, T>&,
                    const DenseVector<size_type, T>&);

  DenseVector<size_type, T> friend
  operator* BRAKET (const SparseMatrixCSC<size_type, T>&,
                    const DenseVector<size_type, T>&);

  DenseVector<size_type, T> friend
  operator* BRAKET (const SparseMatrixCOO<size_type, T>&,
                    const DenseVector<size_type, T>&);

  DenseVector<size_type, T> friend
  operator* BRAKET (size_type const, const DenseVector<size_type, T>&);

  DenseVector<size_type, T> friend
  operator* BRAKET (double const, const DenseVector<size_type, T>&);

  T friend
  operator* BRAKET (const DenseVector<size_type, T>&,
                    const DenseVector<size_type, T>&);

  DenseVector<size_type, T> friend
  diagonal BRAKET (const SparseMatrixCSR<size_type, T>&);

  DenseVector<size_type, T> friend
  Transpose_SMVm BRAKET (const SparseMatrixCSR<size_type, T>&,
                         const DenseVector<size_type, T>&);

  // Approximate solver, asolve, derived from  asolve.c found with linbcg.c
  // in "Numerical Recipes in C", second edition, Press, Vetterling, Teukolsky,
  // Flannery, pp 86-89.
  DenseVector<size_type, T> asolve(DenseVector<size_type, T>& a)
  {
    #pragma mta trace "asolve(DenseVector) start"
    DenseVector<size_type, T> temp(this->length);

    size_type const stop = this->length;
    T* const temp_values = temp.values;   // Force MTA to place on stack.
    T* const this_values = this->values;  // Force MTA to place on stack.
    T* const a_values = a.values;         // Force MTA to place on stack.

    #pragma mta assert no dependence
    for (size_type i = 0; i < stop; i++)
    {
      T const zero = T();
      temp_values[i] = (this_values[i] != zero) ?
                       a_values[i] / this_values[i] : a_values[i];
    }

    #pragma mta trace "asolve(DenseVector) stop"

    return temp;
  }

  // The following methods are for development and may be removed / modified.
  /* DenseVector::fill() */
  void fill(T const* const val)
  {
    size_type const stop = this->length;
    T* const this_values = this->values;  // Force MTA to place on stack.

    #pragma mta assert no dependence
    for (size_type i = 0; i < stop; i++) this_values[i] = val[i];
  }

  void VectorPrint(char const* name)
  {
    printf("DenseVector Print %s length %d\n", name, this->length);

    size_type minimum = (this->length < 10) ? this->length : 10;

    printf("DenseVector Values: ");
    for (size_type i = 0; i < minimum; i++)
    {
      printf("%g, ", this->values[i]);
    }
    printf("\n");
  }
};

template <typename size_type, typename T> DenseVector<size_type, T> inline
operator*(size_type const a, const DenseVector<size_type, T>& b)
{
  #pragma mta trace "operator* (int, DenseVector) start"

  DenseVector<size_type, T> temp (b.length);

  size_type const stop = b.length;
  T* const temp_values = temp.values;  // Force MTA to place on stack.
  T* const b_values = b.values;        // Force MTA to place on stack.

  #pragma mta assert no dependence
  for (size_type i = 0; i < stop; i++) temp_values[i] = a * b_values[i];

  #pragma mta trace "operator* (int, DenseVector) stop"

  return temp;
}

template <typename size_type, typename T> DenseVector<size_type, T> inline
operator*(double const a, const DenseVector<size_type, T>& b)
{
  DenseVector<size_type, T> temp (b.length);

  size_type const stop = b.length;
  T* const temp_values = temp.values;  // Force MTA to place on stack.
  T* const b_values = b.values;        // Force MTA to place on stack.

  #pragma mta assert no dependence
  for (size_type i = 0; i < stop; i++) temp_values[i] = a * b_values[i];

  return temp;
}

template <typename size_type, typename T> T inline
operator* (const DenseVector<size_type, T>& a,
           const DenseVector<size_type, T>& b)
{
  T temp = T();
  size_type const stop = a.length;
  T* const a_values = a.values;   // Force MTA to place on stack.
  T* const b_values = b.values;   // Force MTA to place on stack.

  #pragma mta assert parallel
  for (size_type i = 0; i < stop; i++)
  {
    //safe_incr(temp, a_values[i] * b_values[i]) ;
    temp += a_values[i] * b_values[i];
  }

  return temp;
}

template <typename size_type, typename T>
DenseVector<size_type, T>
operator-(const DenseVector<size_type, T>& a,
          const DenseVector<size_type, T>& b)
{
  DenseVector<size_type, T> temp(a);
  temp -= b;
  return temp;
}

template <typename size_type, typename T>
DenseVector<size_type, T>
operator+(const DenseVector<size_type, T>& a,
          const DenseVector<size_type, T>& b)
{
  DenseVector<size_type, T> temp(a);
  temp += b;
  return temp;
}

/***************************** CSR  SparseMatrix ******************************/
/* CSR : Compressed Sparse Row                                                */
template <typename size_type, typename T = double>
class SparseMatrixCSR : public MatrixBase<size_type, T> {
private:
  size_type* columns;

public:
  typedef size_type* column_iterator;
  typedef T* value_iterator;

  SparseMatrixCSR() : MatrixBase<size_type, T>(), columns(0) {}

  // SparseMatrixCSR Constructor #1.
  SparseMatrixCSR(size_type const row, size_type const col,
                  size_type const count) :
    MatrixBase<size_type, T> (row, col, count, true)
  {
    this->columns = (size_type*) malloc(count * sizeof(size_type));

    this->index = (size_type*) malloc((this->nRow + 1) * sizeof(size_type));

    // Force MTA to place on stack.
    size_type* const this_columns = this->columns;
    size_type const finish = this->nNonZero;

    #pragma mta assert parallel
    for (size_type i = 0; i < finish; i++) this_columns[i] = 0;

    // Force MTA to place on stack.
    size_type* const this_index = this->index;
    size_type const stop = this->nRow + 1;

    #pragma mta assert parallel
    for (size_type i = 0; i < stop; i++) this_index[i] = 0;
  }

  // SparseMatrixCSR Constructor #2.
  SparseMatrixCSR(const size_type row, const size_type col,
                  const size_type count, size_type* indx, T* val,
                  size_type*  cols) :
    MatrixBase<size_type, T>(row, col, count, false),
    MatrixBase<size_type, T>::index(indx), columns(cols) {}

  // SparseMatrixCSR Copy Constructor.
  SparseMatrixCSR(const SparseMatrixCSR<size_type, T>& a) :
    MatrixBase<size_type, T> (a)
  {
    columns = (size_type*) malloc(this->nNonZero * sizeof (size_type));

    this->index  = (size_type*) malloc((this->nRow + 1) * sizeof (size_type));

    size_type const stop = this->nNonZero;

    // Force MTA to place on stack.
    size_type* const this_columns = this->columns;

    size_type* const a_columns = a.columns;  // Force MTA to place on stack.
    size_type* const a_index = a.index;      // Force MTA to place on stack.

    #pragma mta assert parallel
    for (size_type i = 0; i < stop; i++) this_columns[i] = a_columns[i];

    size_type* const this_index = this->index;  // Force MTA to place on stack.
    size_type const end = this->nRow + 1;       // Force MTA to place on stack.

    #pragma mta assert parallel
    for (size_type i = 0; i < end; i++) this_index [i] = a_index[i];
  }

  ~SparseMatrixCSR()
  {
    if (this->data_owned && this->columns) free (this->columns);
    if (this->data_owned && this->index) free (this->index);

    this->columns = 0;
    this->index   = 0;
  }

  size_type* get_index() const { return this->index; }

  void init(const size_type row, const size_type col, const size_type count,
            size_type* indx, T* vals, size_type*  cols)
  {
    clear();

    this->nRow = row;
    this->nCol = col;
    this->nNonZero = count;
    this->index = indx;
    this->columns = cols;
    this->values = vals;
    this->data_owned = false;
  }

  void clear()
  {
    if (this->data_owned && this->values) free(this->values);
    this->values = 0;

    if (this->data_owned && this->index) free(this->index);
    this->index  = 0;
  }

  SparseMatrixCSR<size_type, T>&
  operator=(const SparseMatrixCSR<size_type, T>& a)
  {
    if (this != &a)
    {
      this->nRow = a.nRow;
      this->nCol = a.nCol;
      this->nNonZero = a.nNonZero;

      if (columns) free (columns);
      columns = (size_type*) malloc(this->nNonZero * sizeof(size_type));

      if (this->index) free (this->index);
      this->index = (size_type*) malloc((this->nRow + 1) * sizeof(size_type));

      if (this->values) free (this->values);
      this->values = (T*) malloc(this->nNonZero * sizeof(T));

      // Force MTA to place on stack.
      size_type const stop = this->nNonZero;
      size_type* const this_columns = this->columns;
      T* const this_values = this->values;
      size_type* const a_columns = a.columns;
      T* const a_values = a.values;

      #pragma mta assert parallel
      for (size_type i = 0; i < stop; i++)
      {
        this_columns[i] = a_columns[i];
        this_values[i] = a_values[i];
      }

      // Force MTA to place on stack.
      size_type const end = this->nRow + 1;
      size_type* const this_index = this->index;
      size_type* const a_index = a.index;

      #pragma mta assert parallel
      for (size_type i = 0; i < end; i++) this_index[i] = a_index[i];
    }

    return *this;
  }

  SparseMatrixCSR<size_type, T>&
  operator=(const SparseMatrixCSC<size_type, T>& a)
  {
    printf("NOT IMPLEMENTED Converting from CSC to CSR\n");
    return *this;
  }

  DenseVector<size_type, T> friend
  operator* <size_type, T> (const SparseMatrixCSR<size_type, T>&,
                            const DenseVector<size_type, T>&);

  SparseMatrixCSR<size_type, T> friend
  operator* <size_type, T> (double const,
                            const SparseMatrixCSR<size_type, T>&);

  SparseMatrixCSR<size_type, T> friend
  operator* <size_type, T> (size_type const,
                            const SparseMatrixCSR<size_type, T>&);

  DenseVector<size_type, T> friend
  diagonal BRAKET (const SparseMatrixCSR<size_type, T>&);

  DenseVector<size_type, T> friend
  Transpose_SMVm BRAKET (const SparseMatrixCSR<size_type, T>&,
                         const DenseVector<size_type, T>&);

  /* SparseMatrixCSR fill()
   *
   */
  void fill(size_type const* const indx, T const* const val,
            size_type const* const cols)
  {
    // Force MTA to place on stack.
    size_type const stop = this->nRow;
    size_type* const this_index = this->index;

    #pragma mta assert no dependence
    for (size_type rows = 0; rows < stop; rows++) this_index[rows] = indx[rows];

    this_index[this->nRow] = this->nNonZero;

    // Force MTA to place on stack.
    size_type const end = this->nNonZero;
    T* const this_values = this->values;
    size_type* const this_columns = this->columns;

    #pragma mta assert no dependence
    for (size_type i = 0; i < end; i++)
    {
      this_values[i]  = val[i];
      this_columns[i] = cols[i];
    }
  }

  size_type col_index(size_type row) const { return this->index[row]; }

  T* col_values_begin(size_type row)
  {
    if ((row >= 0) && (row < this->nRow))
    {
      return &this->values[this->index[row]];
    }
    else
    {
      return 0;
    }
  }

  T* col_values_end(size_type row)
  {
    size_type ind = row + 1;

    if ((ind >= 0) && (ind <= this->nRow))
    {
      return &this->values[this->index[ind]];
    }
    else
    {
      return 0;
    }
  }

  size_type* col_indices_begin(size_type row)
  {
    if ((row >= 0) && (row < this->nRow))
    {
      return &this->columns[this->index[row]];
    }
    else
    {
      return 0;
    }
  }

  size_type column(size_type j)
  {
    //if ((j >= 0) && (row < this->nCol))
    return this->columns[j];
    //else
    //  return 0;
  }

  size_type* col_indices_end(size_type row)
  {
    size_type ind = row + 1;

    if ((ind >= 0) && (ind <= this->nRow))
    {
      return &this->columns[this->index[ind]];
    }
    else
    {
      return 0;
    }
  }

  void MatrixPrint (char const* name) const
  {
    printf("SparseMatrixCSR Print %s row %d col %d\n",
           name, this->nRow, this->nCol);
  }

  size_type MatrixRows() const { return this->nRow; }
  size_type MatrixCols() const { return this->nCol; }
};

template <typename size_type, typename T>
DenseVector<size_type, T>
operator*(const SparseMatrixCSR<size_type, T>& a,
          const DenseVector<size_type, T>& b)
{
  #pragma mta trace "operator* (SparseMatrixCSR, DenseVector) start"

  if (a.nCol != b.length)
  {
    printf("INCOMPATIBLE SparseMatrixCSR * DenseVector multiplication\n");
    exit(1);
  }

  DenseVector<size_type, T> temp(b.length);
  T* const temp_values = temp.values;   // force MTA to place on stack
  T const zero = T();
  size_type const finish = temp.length;

  #pragma mta assert parallel
  for (size_type i = 0; i < finish; i++) temp_values[i] = zero;

#ifdef __MTA__
  size_type starttimer = mta_get_clock(0);
#endif

  size_type const stop = a.nRow;
  size_type* const a_index = a.index;      // Force MTA to place on stack.
  T* const a_values = a.values;            // Force MTA to place on stack.
  T* const b_values = b.values;            // Force MTA to place on stack.
  size_type* const a_columns = a.columns;  // Force MTA to place on stack.

  #pragma mta assert parallel
  for (size_type row = 0; row < stop; row++)
  {
    #pragma mta trace "next_row"

    size_type const start  = a_index[row];
    size_type const finish = a_index[row + 1];

    for (size_type i = start; i < finish; i++)
    {
      temp_values[row] += a_values[i] * b_values[ a_columns[i] ];
    }
  }

#ifdef __MTA__
  size_type stoptimer = mta_get_clock(starttimer);
  // printf("MVm total time %g\n", stoptimer/220000000.0);
#endif

  #pragma mta trace "operator* (SparseMatrixCSR, DenseVector) stop"

  return temp;
}

template <typename size_type, typename T>
SparseMatrixCSR<size_type, T>
operator*(size_type const a, const SparseMatrixCSR<size_type, T>& b)
{
  SparseMatrixCSR<size_type, T> temp (b.nRow, b.nCol, b.nNonZero);

  size_type const stop = b.nNonZero;
  T* const temp_values = temp.values;   // Force MTA to place on stack.
  T* const b_values = b.values;         // Force MTA to place on stack.

  #pragma mta assert no dependence
  for (size_type i = 0; i < stop; i++) temp_values[i] = a * b_values[i];

  return temp;
}

template <typename size_type, typename T> SparseMatrixCSR<size_type, T>
operator*(double const a, const SparseMatrixCSR<size_type, T>& b)
{
  SparseMatrixCSR<size_type, T> temp (b.nRow, b.nCol, b.nNonZero);

  size_type const stop = b.nNonZero;
  T* temp_values = temp.values;   // Force MTA to place on stack.
  T* b_values = b.values;         // Force MTA to place on stack.

  #pragma mta assert no dependence
  for (size_type i = 0; i < stop; i++) temp_values[i] = a * b_values[i];

  return temp;
}

template <typename size_type, typename T> DenseVector<size_type, T> diagonal (const SparseMatrixCSR<size_type, T>& a)
{
  #pragma mta trace "diagonal(SparseMatrixCSR) start"
  if (a.nRow != a.nCol)
  {
    printf("diagonal called on non square matrix\n");
    //printf("nRow=%d, nCol=%d\n", a.nRow, a.nCol);
    exit(1);
  }

  DenseVector<size_type, T> temp(a.nRow);
  temp();

  size_type const finish = a.nRow;
  size_type* const a_index = a.index;      // Force MTA to place on stack.
  size_type* const a_columns = a.columns;  // Force MTA to place on stack.
  T* const a_values = a.values;            // Force MTA to place on stack.
  T* const temp_values = temp.values;      // Force MTA to place on stack.

  #pragma mta assert parallel
  #pragma mta loop future
  for (size_type row = 0; row < finish; row++)
  {
    size_type const start = a_index[row];
    size_type const stop  = a_index[row + 1];

    #pragma mta assert parallel
    for (size_type i = start; i < stop; i++)
    {
      if (row == a_columns[i])
      {
        temp_values[row] = a_values[i];
#ifndef __MTA__
        break;
#endif
      }
    }
  }

  #pragma mta trace "diagonal(SparseMatrixCSR) stop"

  return temp;
}

template <typename size_type, typename T>
DenseVector<size_type, T>
Transpose_SMVm (const SparseMatrixCSR<size_type, T>& a,
                const DenseVector<size_type, T>& b)
{
  #pragma mta trace "Transpose_SMVm start"
  if (a.nCol != b.length)
  {
    printf("INCOMPATIBLE Transpose (SparseMatrixCSR) * DenseVector "
           "multiplication\n");
    exit(1);
  }

  DenseVector<size_type, T> temp(b.length);
  T* const temp_values = temp.values;      // Force MTA to place on stack.
  T* const a_values = a.values;            // Force MTA to place on stack.
  T* const b_values = b.values;            // Force MTA to place on stack.
  size_type* const a_index = a.index;      // Force MTA to place on stack.
  size_type* const a_columns = a.columns;  // Force MTA to place on stack.
  size_type const stop = temp.length;

  #pragma mta assert no dependence
  for (size_type i = 0; i < stop; i++)
  {
    temp_values[i] = T();
  }

  size_type const finish = temp.length;

  #pragma mta assert parallel
  for (size_type row = 0; row < finish; row++)
  {
    size_type const start = a_index[row];     // Force MTA to place on stack.
    size_type const stop = a_index[row + 1];  // Force MTA to place on stack.

    for (size_type i = start; i < stop; i++)
    {
      T temp_i = mt_readfe(temp_values[ a_columns[i] ]);
      temp_i += a_values[i] * b_values[row];
      mt_write(temp_values[ a_columns[i] ], temp_i);
      //mt_incr(temp_values[a_columns[i]], a_values[i] * b_values[row]);
      //temp_values[ a_columns[i] ] += a_values[i] * b_values[row];
    }
  }

  #pragma mta trace "Transpose_SMVm start"

  return temp;
}

/***************************** CSC  SparseMatrix ******************************/

template <typename size_type, typename T = double>
class SparseMatrixCSC : MatrixBase<size_type, T> {
private:
  size_type* rows;

public:
  SparseMatrixCSC<size_type, T> () : MatrixBase<size_type, T> ()
  { rows = 0; }

  SparseMatrixCSC<size_type, T>(size_type const row, size_type const col,
                                size_type const count) :
    MatrixBase<size_type, T> (row, col, count)
  {
    rows  = (size_type*) malloc (count * sizeof(size_type));

    this->index = (size_type*) malloc ((this->nCol + 1) * sizeof(size_type));
  }

  SparseMatrixCSC<size_type, T>(const SparseMatrixCSC<size_type, T>&a) :
    MatrixBase<size_type, T> (a)
  {
    rows = (size_type*) malloc(this->nNonZero * sizeof(size_type));

    this->index = (size_type*) malloc((this->nCol + 1) * sizeof(size_type));

    size_type* const a_rows = a.rows;         // Force MTA to place on stack.
    size_type* const this_rows = this->rows;  // Force MTA to place on stack.
    size_type const stop = this->nNonZero;

    #pragma mta assert parallel
    for (size_type i = 0; i < stop; i++) this_rows[i] = a_rows[i];

    size_type* const this_index = this->index;  // Force MTA to place on stack.
    size_type* const a_index = a.index;         // Force MTA to place on stack.
    size_type const end = this->nCol + 1;

    #pragma mta assert parallel
    for (size_type i = 0; i < end; i++) this_index[i] = a_index[i];
  }

  ~SparseMatrixCSC<size_type, T> ()
  {
    if (this->data_owned && this->rows) free (this->rows);
    if (this->data_owned && this->index) free (this->index);

    this->rows  = 0;
    this->index = 0;
  }

  SparseMatrixCSC<size_type, T>&
  operator=(const SparseMatrixCSR<size_type, T>& a)
  {
    printf("NOT IMPLEMENTED: Converting from CSR to CSC\n");
    return *this;
  }

  SparseMatrixCSC<size_type, T>&
  operator=(const SparseMatrixCSC<size_type, T>& a)
  {
    if (this != &a)
    {
      this->nRow = a.nRow;
      this->nCol = a.nCol;
      this->nNonZero = a.nNonZero;

      size_type* const this_rows = this->rows;  // Force MTA to place on stack.
      size_type* const a_rows = a.rows;         // Force MTA to place on stack.
      T* const this_values = this->values;      // Force MTA to place on stack.
      T* const a_values = a.values;             // Force MTA to place on stack.
      size_type const stop = this->nNonZero;

      #pragma mta assert no dependence
      for (size_type i = 0; i < stop; i++)
      {
        this_rows[i]   = a_rows[i];
        this_values[i] = a_values[i];
      }

      size_type* const this_index = this->index; // Force MTA to place on stack.
      size_type* const a_index = a.index;        // Force MTA to place on stack.
      size_type const end = this->nCol + 1;

      #pragma mta assert no dependence
      for (size_type i = 0; i < end; i++) this_index[i] = a_index[i];
    }

    return *this;
  }

  SparseMatrixCSC<size_type, T>& operator()(SparseMatrixCSR<size_type, T>&);

  friend DenseVector<size_type, T>
  operator* <size_type, T> (const SparseMatrixCSC<size_type, T>&,
                            const DenseVector<size_type, T>&);

  void MatrixPrint (char const* name)
  {
    printf("SparseMatrixCSC Print %s row %d col %d\n",
           name, this->nRow, this->nCol);
  }

  size_type MatrixRows() const { return this->nRow; }
  size_type MatrixCols() const { return this->nCol; }
};

template <typename size_type, typename T> DenseVector<size_type, T>
operator*(const SparseMatrixCSC<size_type, T>& a,
          const DenseVector<size_type, T>& b)
{
  if (a.nCol != b.length)
  {
    printf("INCOMPATIBLE SparseMatrixCSC * DenseVector multiplication\n");
    exit(1);
  }

  DenseVector<size_type, T> temp(b.length);
  T* const temp_values = temp.values;   // force MTA to place on stack
  size_type const istop = temp.length;
  T const zero = T();

  #pragma mta assert parallel
  for (size_type i = 0; i < istop; i++) temp_values[i] = zero;

  size_type const colstop = a.nCol;
  size_type* const a_index = a.index;  // Force MTA to place on stack.
  size_type* const a_rows = a.rows;    // Force MTA to place on stack.
  T* const a_values = a.values;        // Force MTA to place on stack.
  T* const b_values = b.values;        // Force MTA to place on stack.

  #pragma mta assert parallel
  #pragma mta loop future
  for (size_type col = 0; col < colstop; col++)
  {
    size_type const start = a_index[col];
    size_type const stop  = a_index[col + 1];

    #pragma mta assert parallel
    for (size_type i = start; i < stop; i++)
    {
      //mt_incr(temp_values[a_rows[i]], a_values[i] * b_values[a_rows[i]]);
      temp_values[ a_rows[i] ] += a_values[i] * b_values[ a_rows[i] ];
    }
  }

  return temp;
}

/***************************** COO  SparseMatrix ******************************/
/* COO : */

template <typename size_type, typename T = double>
class SparseMatrixCOO : MatrixBase<size_type, T> {
private:
  size_type* columns;
  size_type* rows;

public:
  SparseMatrixCOO() : MatrixBase<size_type, T> ()
  {
    this->columns = 0;
    this->rows    = 0;
  }

  SparseMatrixCOO(size_type const row, size_type const col,
                  size_type const nnz) :
    MatrixBase<size_type, T> (row, col, nnz)
  {
    rows = (size_type*) malloc (nnz * sizeof(size_type));
    columns = (size_type*) malloc (nnz * sizeof(size_type));
  }

  SparseMatrixCOO (SparseMatrixCOO<size_type, T>& a) :
    MatrixBase<size_type, T>(a)
  {
    rows = (size_type*) malloc(a.nNonZero * sizeof(size_type));
    columns = (size_type*) malloc(a.nNonZero * sizeof(size_type));

    size_type const stop = this->nNonZero;
    T* const this_values = this->values;       // MTA -to force onto stack.
    T* const a_values = a.values;              // MTA -to force onto stack.
    size_type* const this_rows = this->rows;        // MTA -to force onto stack.
    size_type* const this_columns = this->columns;  // MTA -to force onto stack.
    size_type* const a_rows = a.rows;               // MTA -to force onto stack.
    size_type* const a_columns = a.columns;         // MTA -to force onto stack.

    #pragma mta assert parallel
    for (size_type i = 0; i < stop; i++)
    {
      this_values[i] = a_values[i];
      this_rows[i] = a_rows[i];
      this_columns[i] = a_columns[i];
    }
  }

  ~SparseMatrixCOO()
  {
    if (this->columns) free (this->columns);
    if (this->index) free (this->index);
    if (this->rows) free (this->rows);

    this->columns = 0;
    this->index = 0;
    this->rows = 0;
  }

  SparseMatrixCOO<size_type, T>&
  operator=(const SparseMatrixCOO<size_type, T>& a)
  {
    if (this != &a)
    {
      this->nRow = a.nRow;
      this->nCol = a.nCol;
      this->nNonZero = a.nNonZero;
      this->index = 0;

      if (rows) free(rows);

      rows = (size_type*) malloc(a.nNonZero * sizeof(size_type));

      if (columns) free(columns);

      columns = (size_type*) malloc(a.nNonZero * sizeof(size_type));

      if (this->values) free(this->values);

      this->values = (size_type*) malloc(a.nNonZero * sizeof(size_type));

      // MTA -to force onto stack.
      size_type const stop = this->nNonZero;
      T* const this_values = this->values;
      T* const a_values = a.values;
      size_type* const this_rows = this->rows;
      size_type* const this_columns = this->columns;
      size_type* const a_rows = a.rows;
      size_type* const a_columns = a.columns;

      #pragma mta assert no dependence
      for (size_type i = 0; i < stop; i++)
      {
        this_values[i]  = a_values[i];
        this_rows[i]    = a_rows[i];
        this_columns[i] = a_columns[i];
      }
    }

    return *this;
  }

  SparseMatrixCOO<size_type, T>&
  operator=(const SparseMatrixCSR<size_type, T>& a)
  {
    printf("NOT IMPLEMENTED: Converting from CSR to COO\n");
    return *this;
  }

  SparseMatrixCOO<size_type, T>&
  operator=(const SparseMatrixCSC<size_type, T>& a)
  {
    printf("NOT IMPLEMENTED: Converting from CSC to COO\n");
    return *this;
  }

  SparseMatrixCOO<size_type, T>&
  operator()(const SparseMatrixCSR<size_type, T>&);

  SparseMatrixCOO<size_type, T>&
  operator()(const SparseMatrixCSC<size_type, T>&);

  DenseVector<size_type, T> friend
  operator* <size_type, T> (const SparseMatrixCOO<size_type, T>&,
                            const DenseVector<size_type, T>&);

  void MatrixPrint (char const* name)
  {
    printf("SparseMatrixCOO Print %s row $d col %d\n", name,
           this->nRow, this->nCol);
  }
};

template <typename size_type, typename T>
DenseVector<size_type, T>
operator*(const SparseMatrixCOO<size_type, T>& a,
          const DenseVector<size_type, T>& b)
{
  if (a.nCol != b.length)
  {
    printf("INCOMPATIBLE SparseMatrixCoo * DenseVector multiplication\n");
    exit(1);
  }

  DenseVector<size_type, T> temp(b.length);
  size_type const stop = temp.length;
  T* const temp_values = temp.values;
  T const zero = T();

  #pragma mta assert parallel
  for (size_type i = 0; i < stop; i++) temp_values[i] = zero;

  size_type const end = a.nNonZero;
  T* const a_values = a.values;            // MTA -to force onto stack.
  T* const b_values = b.values;            // MTA -to force onto stack.
  size_type* const a_rows = a.rows;        // MTA -to force onto stack.
  size_type* const b_rows = b.rows;        // MTA -to force onto stack.
  size_type* const a_columns = a.columns;  // MTA -to force onto stack.

  #pragma mta assert parallel
  for (size_type i = 0; i < end; i++)
  {
    mt_inc (temp_values[a_rows[i]], a_values[i] * b_values[a_columns[i]]);
  }

  return temp;
}

/* biconjugate gradient solver derived from linbcg.c in "Numerical Recipes */
/* in C", second edition, Press, Vetterling, Teukolsky, Flannery, pp 86-89 */
template <typename size_type, typename T>
DenseVector<size_type, T>& linbcg (const SparseMatrixCSR<size_type, T>& A,
                                   DenseVector<size_type, T>& x,
                                   const DenseVector<size_type, T>& b,
                                   size_type const itermax,
                                   T& err,
                                   T const tol)
{
  #pragma mta trace "linbcg start"

  size_type const length = A.MatrixRows();
  double const bnorm = b.norm2();
  double ak = 0, akden = 0, bk = 0, bknum = 0, bkden = 0;

  DenseVector<size_type, T> p(length), pp(length);
  DenseVector<size_type, T> r(length), rr(length);
  DenseVector<size_type, T> z(length), zz(length);
  DenseVector<size_type, T> d = diagonal(A);

  r = b - (A * x);
  z = d.asolve (r);
  rr = r;

  for (size_type iter = 0; iter < itermax; iter++)
  {
    zz = d.asolve (rr);

    bknum = z * rr;

    if (iter == 0)
    {
      p  = z;
      pp = zz;
    }
    else
    {
      bk = bknum / bkden;
      p  = (bk * p)  + z;
      pp = (bk * pp) + zz;
    }

    bkden =  bknum;

    z = A * p;
    akden = z * pp;
    ak = bknum / akden;

    zz = Transpose_SMVm(A, pp);

    x += (ak * p);
    r -= (ak * z);
    rr -= (ak * zz);

    z = d.asolve(r);
    err = r.norm2() / bnorm;

    if (err < tol) break;
  }

  #pragma mta trace "linbcg stop"

  return x;
}

#endif
