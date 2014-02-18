
#include <sst_config.h>
#include <sst/core/serialization.h>

#include <assert.h>

#include <sst/core/output.h>
#include <sst/core/element.h>
#include <sst/core/params.h>

using namespace SST;
using namespace SST::Hermes;

namespace SST {
namespace Ember {

class Ember1DHaloExchange : public SST::Component {

public:
	Ember1DHaloExchange(SST::ComponentId_t id, SST::Params& params);
	void setup();
	void finish();
	void init(unsigned int phase);

private:
	~Ember1DHaloExchange();
	Ember1DHaloExchange(); // Only implement for serialization
	Ember1DHaloExchange(const Ember1DHaloExchange&); // Do not implement
	void operator=(const Ember1DHaloExchange&); // Do not implement

	void handleSendLeftFunction(int val);
	void handleSendRightFunction(int val);
	void handleIrecvLeftFunction(int val);
	void handleIrecvRightFunction(int val);
	void handleWaitLeftFunction(int val);
	void handleWaitRightFunction(int val);

	void handleInitFunction(int val);
	void handleFinalizeFunction(int val);

	typedef Arg_Functor<Ember1DHaloExchange, int> DerivedFunctor;
	uint32_t maxSendCount;
	uint32_t curSendCount;
	uint32_t nanoSecCompute;
	uint32_t msgSize;

	DerivedFunctor sendLeftFunctor;
	DerivedFunctor sendRightFunctor;
	DerivedFunctor irecvLeftFunctor;
	DerivedFunctor irecvRightFunctor;
	DerivedFunctor waitLeftFunctor;
	DerivedFunctor waitRightFunctor;

	DerivedFunctor initFunctor;
	DerivedFunctor finalizeFunctor;

	SST::Link* selfLink;

  	////////////////////////////////////////////////////////
  	friend class boost::serialization::access;
  	template<class Archive>
  	void save(Archive & ar, const unsigned int version) const
  	{
    		ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
  	}

  	template<class Archive>
  	void load(Archive & ar, const unsigned int version)
  	{
		ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	}

	BOOST_SERIALIZATION_SPLIT_MEMBER()
};

}
}
