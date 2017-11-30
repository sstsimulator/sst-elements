
struct MemReq {
    MemReq( Hermes::Vaddr addr, size_t length ) :
		addr(addr), length(length) {}
	Hermes::Vaddr addr;
	size_t length;
};
