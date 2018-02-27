
struct MemReq {
    MemReq( Hermes::Vaddr addr, size_t length, int pid = -1) :
		addr(addr), length(length), pid(pid) {}
	Hermes::Vaddr addr;
	size_t length;
    int     pid;
};
