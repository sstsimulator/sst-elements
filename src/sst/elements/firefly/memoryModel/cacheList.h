// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#if 1
#define myPrintf(x,...)
#else
#define myPrintf(x,...) printf("List." x, __VA_ARGS__ )
#endif

template< class T >
        class List {

            struct Item {
                Item( ) {}
                Item( T value ) : value(value) { }
                T value;
                Item* prev;
                Item* next;
            };

          public:

            typedef Item* Entry;

            List() {
                m_head.prev = NULL;
                m_head.next = &m_tail;
				m_tail.prev = &m_head;
				m_tail.next = NULL;
                myPrintf("%s() head=%p tail=%p\n",__func__, &m_head,&m_tail);
            }
			~List() {
                clear();
            }
            void clear() {
				Item* cur = m_head.next;
				while ( cur != &m_tail ) {
					Item* tmp = cur;
					cur = tmp->next;
					delete tmp;
				}
			}


            void pop_front() {
                assert( m_head.next != &m_head );
                Item* tmp = m_head.next;
                myPrintf("%s() ptr=%p value=%#" PRIx64 "\n",__func__,tmp,tmp->value);
                unlink( tmp );
                delete tmp;
            }

            void push_back( Hermes::Vaddr addr ) {
				Item* x = new Item( addr );
                myPrintf("%s() ptr=%p %#" PRIx64 "\n",__func__,x,addr);
                push_back( x );
            }

            void move_to_back( Entry e ) {
                myPrintf("%s() e=%p value=%#" PRIx64 "\n",__func__,e,e->value);
                print();
				if ( end() != e ) {
                	unlink( e );
                	push_back( e );
				}
                myPrintf("%s()\n",__func__);
            }

            T& get_front_value() {
                myPrintf("%s()\n",__func__);
				print();
                return m_head.next->value;
            }

            Entry front() { return m_head.next; }
            Entry end() { return m_tail.prev; }

          private:

            void unlink( Entry e ) {
                myPrintf("%s() ptr=%p value=%#" PRIx64 "\n",__func__,e,e->value);
                Item* prev = e->prev;
                Item* next = e->next;
                prev->next = next;
                next->prev = prev;
                print();
            }

            void push_back( Item* item ) {
                myPrintf("%s() ptr=%p value=%#" PRIx64 "\n",__func__,item, item->value);

                Item* old_tail = m_tail.prev;

                item->next = &m_tail;
                item->prev = old_tail;

                m_tail.prev = item;
                old_tail->next = item;

                print();
            }
            void print() {
#ifdef DO_PRINT
                Item* ptr = m_head.next;

                while( NULL != ptr->next ) {
                    myPrintf("%s() %p %p %p %#" PRIx64 "\n",__func__,ptr, ptr->prev, ptr->next, ptr->value);
                    ptr = ptr->next;
                }
#endif
            }

            Item m_head;
            Item m_tail;
        };
