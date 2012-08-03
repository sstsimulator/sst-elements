/*
 * DRAM_Cfg.cc
 *
 *  Created on: Feb 18, 2009
 *      Author: SolidSnake
 */

#include "DRAM_Cfg.h"

DRAM_Cfg::DRAM_Cfg(int n, int pC) {

	procConc = pC;

	numX = sqrt(n / pC);
	numY = sqrt(n / pC);

}

DRAM_Cfg::~DRAM_Cfg() {

}

void DRAM_Cfg::init() {

	for (int i = 0; i < numX * numY; i++) {
		int n = get_bank_id(i, numX, numY);
		for (int j = 0; j < procConc; j++) {
			bankMap[i*procConc + j] = n;
		}
	}

	for (int i = 0; i < getNumDRAM(); i++) {
		int n = procConc * get_access_node(i);
		accessMap[i] = n;
	}

}

int DRAM_Cfg::getAccessId(int coreId) {

	return accessMap[bankMap[coreId]];
}

int DRAM_Cfg::getAccessCore(int dramId) {

	return accessMap[dramId];
}
