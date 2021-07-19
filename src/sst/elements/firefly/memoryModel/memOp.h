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

   class MemOp {

        typedef std::function<void()> Callback;

	  public:
        enum Op { NotInit, NoOp, BusLoad, BusStore, LocalLoad, LocalStore, HostLoad, HostStore, HostCopy, BusDmaToHost, BusDmaFromHost, HostBusWrite, HostBusRead };

        MemOp( ) : addr(0), length(0), type(NotInit), offset(0), callback(NULL), m_pending(0) {}
        MemOp( Op op ) : addr(0), length(0), type(op), offset(0), callback(NULL), m_pending(0) {}
        MemOp( Hermes::Vaddr addr, size_t length, Op op, Callback callback = NULL ) :
                addr(addr), length(length), type(op), offset(0), callback(callback), m_pending(0) {}
        MemOp( Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, Op op, Callback callback = NULL ) :
				dest(dest), src(src), length(length), type(op), offset(0), chunk(0), callback(callback), m_pending(0) {
			//printf("%s() dest=%#lx src=%#lx length=%lu\n",__func__,dest,src,length);
		}

		Hermes::Vaddr getCurrentAddr() {
			if ( HostCopy == type ) {
				if ( 0 == chunk % 2 ) {
					//printf("%s() src=%#lx offset=%lu\n",__func__,src, offset);
					return src + offset;
				} else {
					//printf("%s() dest=%#lx offset=%lu\n",__func__,dest, offset);
					return dest + offset;
				}
			} else {
				return addr + offset;
			}
		}
		size_t getCurrentLength(  size_t chunkSize ) {
			size_t val = length - offset < chunkSize ? length - offset : chunkSize;
			//printf("%s() max=%lu val=%lu\n",__func__,chunkSize,val);
			return val;
		}

		void incOffset( size_t chunkSize ) {
			if ( HostCopy == type ) {
				//printf("%s() chunk=%d\n",__func__,chunk);
				if ( 1 == chunk % 2 ) {
					//printf("%s() chunk=%d inc\n",__func__,chunk);
					offset += chunkSize;
				}
				++chunk;
			} else {
				offset += chunkSize;
			}
            ++m_pending;
		}

		bool isWrite() {
			return type == HostBusWrite;
		}
		bool isLoad() {
			if ( HostCopy == type ) {
				if ( 1 == chunk % 2 ) {
					//printf("%s() Load\n",__func__);
					return true;
				} else {
					//printf("%s() Store\n",__func__);
					return false;
				}

			} else {
				switch ( type ) {
					case BusLoad:
					case LocalLoad:
					case HostLoad:
					case BusDmaFromHost:
						return true;
					default:
						return false;
				}
			}
		}

        void decPending() {
            --m_pending;
        }
        bool canBeRetired() {
            return isDone() && 0 == m_pending;
        }
		bool isDone() {
			return offset == length;
		}
		Op getOp( ) {

			if ( HostCopy == type ) {
				if ( 1 == chunk %2 ) {
					//printf("%s() BusLoad\n",__func__);
					return BusLoad;
				} else {
					//printf("%s() BusStore\n",__func__);
					return BusStore;
				}
			} else {
				return type;
			}
		}

		int chunk;
        Hermes::Vaddr addr;
        Hermes::Vaddr src;
        Hermes::Vaddr dest;
        Callback callback;
        int m_pending;

        size_t   length;
		size_t offset;

        const char* getName( ) {
            switch( type ) {
            case NotInit: return "NotInit";
            case NoOp: return "NoOp";
            case BusLoad: return "BusLoad";
            case BusStore: return "BusStore";
            case LocalLoad: return "LocalLoad";
            case LocalStore: return "LocalStore";
            case HostLoad: return "HostLoad";
            case HostStore: return "HostStore";
            case HostCopy: return "HostCopy";
            case BusDmaToHost: return "BusDmaToHost";
            case BusDmaFromHost: return "BusDmaFromHost";
            case HostBusWrite: return "HostBusWrite";
            case HostBusRead: return "HostBusRead";
            }
        }
     private:
        Op type;
    };
