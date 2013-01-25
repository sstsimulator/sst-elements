
#ifndef LISTENER
#define LISTENER

namespace McOpteron{ //Scoggin: Added a namespace to reduce possible conflicts as library
class Listener{
 public:
   virtual void notify(void *obj) {};
};
}//End namespace McOpteron
#endif

