// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

class Tree {

  public:
    Tree( int degree, int myRank, int size, int root  ) :
        m_degree( degree ),
        m_myRank( myRank ),
        m_size( size ),
        m_numChildren(0),
        m_myVirtRank( myRank ),
        m_root( root ),
        m_parent( -1 )
    {
        if ( root > 0 ) {
            if ( root == m_myRank ) {
                m_myVirtRank = 0;
            } else if ( 0 == m_myRank )  {
                m_myVirtRank = root;
            }
        }

        for ( int i = 0; i < m_degree; i++ ) {
            if ( calcChild( i ) < m_size ) {
                ++m_numChildren;
            }
        }

        if ( m_myVirtRank > 0 ) {
            int tmp = m_myVirtRank % m_degree;
            tmp = 0 == tmp ? m_degree : tmp ;
            m_parent = (m_myVirtRank - tmp ) / m_degree;

            if ( m_parent == 0 ) {
                m_parent = m_root;
            } else if ( m_parent == m_root ) {
                m_parent = 0;
            }
        }
    }

    int myRank() { return m_myRank; }

    int size() { return m_size; }

    int parent() { return m_parent; }

    size_t numChildren() { return m_numChildren; }

    int calcChild( int i ) {
        int child = (m_myVirtRank * m_degree) + i + 1;
        // ummm, child can never be 0
        if ( child == 0 ) {
            child = m_root;
        }  else if ( child == m_root ) {
            child = 0;
        }
        return child;
    }

  private:

    int m_degree;
    int m_myRank;
    int m_size;
    int m_numChildren;
    int m_myVirtRank;
    int m_root;
    int m_parent;
};
