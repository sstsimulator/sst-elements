       template< class T >
        class List {

            struct Item {
                Item( ) {}
                Item( T value ) : value(value) { assert(value);}
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
#ifdef FOOBAR
                printf(":0:HostCacheUnit::%s() head=%p tai=%p\n",__func__, &m_head,&m_tail);
#endif
            }
			~List() {
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
#ifdef FOOBAR
                printf(":0:HostCacheUnit::%s() ptr=%p value=%#lx\n",__func__,tmp,tmp->value);
#endif
                unlink( tmp );
                delete tmp;
            }
#ifdef FOOBAR
			void check( Entry e ) {

                Item* ptr = m_head.next;
				
                while( NULL != ptr->next ) {
					assert( e!= ptr ); 
                    ptr = ptr->next;
                }
			}
#endif

            void push_back( Hermes::Vaddr addr ) {
				Item* x = new Item( addr );
#ifdef FOOBAR
                printf(":0:HostCacheUnit::%s() ptr=%p %#lx\n",__func__,x,addr);
#endif
                push_back( x ); 
            }

            void move_to_back( Entry e ) {
#ifdef FOOBAR
                printf(":0:HostCacheUnit::%s() e=%p value=%#lx\n",__func__,e,e->value);
                print();
#endif
				if ( end() != e ) {
                	unlink( e );
                	push_back( e );
				}
#ifdef F00
                printf(":0:HostCacheUnit::%s()\n",__func__);
#endif
            }

            T& get_front_value() {
#ifdef F00
                printf(":0:HostCacheUnit::%s()\n",__func__);
				print();
#endif
                return m_head.next->value;
            }

            Entry front() { return m_head.next; }
            Entry end() { return m_tail.prev; }

          private:

            void unlink( Entry e ) {
#ifdef FOOBAR
                printf(":0:HostCacheUnit::%s() ptr=%p value=%#lx\n",__func__,e,e->value);
#endif
                Item* prev = e->prev;
                Item* next = e->next;
                prev->next = next;
                next->prev = prev;
#ifdef FOOBAR
                print();
#endif
            }

            void push_back( Item* item ) {
#ifdef FOOBAR
                printf(":0:HostCacheUnit::%s() ptr=%p value=%lx\n",__func__,item, item->value);
#endif
				
                Item* old_tail = m_tail.prev;

                item->next = &m_tail;
                item->prev = old_tail;

                m_tail.prev = item;
                old_tail->next = item;
                
#ifdef FOOBAR
                print();
#endif
            }
#ifdef FOOBAR 
            void print() {
                Item* ptr = m_head.next;

                while( NULL != ptr->next ) {
                    printf(":0:HostCacheUnit::%s() %p %p %p %#lx\n",__func__,ptr, ptr->prev, ptr->next, ptr->value);
                    ptr = ptr->next;
                }
            }
#endif

            Item m_head;
            Item m_tail;
        };
