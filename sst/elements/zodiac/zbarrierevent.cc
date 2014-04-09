
#include <sst_config.h>
#include "zbarrierevent.h"

using namespace SST::Hermes;
using namespace SST::Zodiac;
using namespace SST;

ZodiacBarrierEvent::ZodiacBarrierEvent(
	Communicator group) {

	msgComm = group;
}

ZodiacEventType ZodiacBarrierEvent::getEventType() {
	return Z_BARRIER;
}

Communicator ZodiacBarrierEvent::getCommunicatorGroup() {
	return msgComm;
}
