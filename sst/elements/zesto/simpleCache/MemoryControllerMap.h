#ifndef MEMORYCONTROLLERMAP_H
#define MEMORYCONTROLLERMAP_H


class MemoryControllerMap {
public:
    virtual ~MemoryControllerMap() {}
    virtual int lookup(paddr_t) = 0;
};


#endif //MEMORYCONTROLLERMAP_H
