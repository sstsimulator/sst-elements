#include "ArbiterCanRoute.h"



int ArbiterCanRoute::getUpPort(ArbiterRequestMsg* rmsg, int lev){

	return route(rmsg);
}



int ArbiterCanRoute::getDownPort(ArbiterRequestMsg* rmsg, int lev){

	return route(rmsg);
}
