   class MemOp {
	  public:
        enum Op { NotInit, BusLoad, BusStore, LocalLoad, LocalStore, HostLoad, HostStore, HostCopy, BusDmaToHost, BusDmaFromHost };

        MemOp( ) : addr(0), length(0), type(NotInit), offset(0) {}
        MemOp( Hermes::Vaddr addr, size_t length, Op op) : addr(addr), length(length), type(op), offset(0) {}
        MemOp( Hermes::Vaddr dest, Hermes::Vaddr src, size_t length, Op op) : 
				dest(dest), src(src), length(length), type(op), offset(0), chunk(0)  {
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

        size_t   length;
		size_t offset;

        const char* getName( ) {
            switch( type ) {
            case BusLoad: return "BusLoad";
            case BusStore: return "BusStore";
            case LocalLoad: return "LocalLoad";
            case LocalStore: return "LocalStore";
            case HostLoad: return "HostLoad";
            case HostStore: return "HostStore";
            case HostCopy: return "HostCopy";
            case BusDmaToHost: return "BusDmaToHost";
            case BusDmaFromHost: return "BusDmaFromHost";
            }
        }
     private: 
        Op type;
    };
