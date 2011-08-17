#include <omnetpp.h>
#include "packetstat.h"
#include "messages_m.h"

class End : public cSimpleModule
{
	public:
		End();
		virtual ~End();

	protected:
		virtual void initialize ();
		virtual void handleMessage (cMessage *msg);
};
