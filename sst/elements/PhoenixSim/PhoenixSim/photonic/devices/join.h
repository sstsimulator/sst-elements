#include <omnetpp.h>
#include <vector>
#include "packetstat.h"
#include "sim_includes.h"
#include "messages_m.h"
#include "basicelement.h"

class Join : public BasicElement
{
	private:
		virtual void Setup();

	public:
		Join();
		virtual ~Join();
		virtual void SetRoutingTable();
};
