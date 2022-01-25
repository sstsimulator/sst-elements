class NetworkQueue {
    struct Entry {
        Entry( RdmaNicNetworkEvent* ev, int destNode ) : ev(ev), destNode(destNode ) {}
        RdmaNicNetworkEvent* ev; 
        int destNode;
    };
  public:
    NetworkQueue( RdmaNic& nic, int numVC, int maxSize ) : nic(nic), maxSize(maxSize) {
        queues.resize( numVC );
    }
    void process();
    void add( int vc, int destNode, RdmaNicNetworkEvent* ev ) { 
        queues[vc].push( Entry( ev, destNode) ); 
    }
    bool full( int vc ) { return queues[vc].size() == maxSize; }

  private:
    RdmaNic& nic;
    std::vector< std::queue< Entry > > queues;
    int maxSize;
};
