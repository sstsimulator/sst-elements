
#ifndef _SIMPLERNGCOMPONENT_H
#define _SIMPLERNGCOMPONENT_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/rng/sstrand.h>

#include <cstring>
#include <string>

namespace SST {
namespace SimpleRNGComponent {

class simpleRNGComponent : public SST::Component {
public:

  simpleRNGComponent(SST::ComponentId_t id, SST::Component::Params_t& params);
  int Setup()  { return 0; }
  int Finish() { return 0; }

private:
  simpleRNGComponent();  // for serialization only
  simpleRNGComponent(const simpleRNGComponent&); // do not implement
  void operator=(const simpleRNGComponent&); // do not implement

  virtual bool tick( SST::Cycle_t );

  SSTRandom* rng;
  SSTRandom* rng_noseed;
  int rng_count;

  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(rng_count);
  }

  template<class Archive>
  void load(Archive & ar, const unsigned int version)
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(rng_count);
  }

  BOOST_SERIALIZATION_SPLIT_MEMBER()

};

}
}

#endif /* _SIMPLERNGCOMPONENT_H */
