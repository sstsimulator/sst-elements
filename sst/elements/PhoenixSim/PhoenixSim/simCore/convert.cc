#include "convert.h"

PhotonicMessage* EtoO(ElectronicMessage *msg)
{
	/*PhotonicMessage *pmsg = new PhotonicMessage;
	pmsg->setId(msg->getId());
	pmsg->setSrcId(msg->getSrcId());
	pmsg->setDstId(msg->getDstId());
	pmsg->setMsgType(msg->getMsgType());
	pmsg->setProcMsgType(msg->getProcMsgType());
	pmsg->setPacketStat(msg->getPacketStat());
	pmsg->setPayloadArraySize(msg->getPayloadArraySize());
	for(int i = 0 ; i < msg->getPayloadArraySize() ; i++)
	{
		pmsg->setPayload(i, msg->getPayload(i));
	}
	pmsg->setBitLength(msg->getBitLength());
	delete msg;
	msg = NULL;
	return pmsg;*/

	opp_error("don't use EtoO");
}

ElectronicMessage *OtoE(PhotonicMessage *msg)
{
	/*ElectronicMessage *emsg = new ElectronicMessage;
	emsg->setId(msg->getId());
	emsg->setSrcId(msg->getSrcId());
	emsg->setDstId(msg->getDstId());
	emsg->setMsgType(msg->getMsgType());
	emsg->setProcMsgType(msg->getProcMsgType());
	emsg->setPacketStat(msg->getPacketStat());
	emsg->setPayloadArraySize(msg->getPayloadArraySize());
	for(int i = 0 ; i < msg->getPayloadArraySize() ; i++)
	{
		emsg->setPayload(i, msg->getPayload(i));
	}
	emsg->setBitLength(msg->getBitLength());
	delete msg;
	msg = NULL;
	return emsg;*/

	opp_error("don't use OtoE");
}


ElectronicMessage* newElectronicMessage()
{
	return new ElectronicMessage;
}
