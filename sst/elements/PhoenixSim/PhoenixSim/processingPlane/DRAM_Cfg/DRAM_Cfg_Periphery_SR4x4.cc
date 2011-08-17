/*
 * DRAM_Cfg_Periphery.cc
 *
 *  Created on: Feb 18, 2009
 *      Author: Gilbert
 */

#include "DRAM_Cfg_Periphery_SR4x4.h"

DRAM_Cfg_Periphery_SR4x4::DRAM_Cfg_Periphery_SR4x4(int n, int pC) :
	DRAM_Cfg(n, pC) {
	bMap[0] = 0;
	bMap[1] = 1;
	bMap[2] = 4;
	bMap[3] = 4;
	bMap[4] = 2;
	bMap[5] = 3;
	bMap[6] = 5;
	bMap[7] = 5;
	bMap[8] = 7;
	bMap[9] = 6;
	bMap[10] = 11;
	bMap[11] = 10;
	bMap[12] = 6;
	bMap[13] = 6;
	bMap[14] = 8;
	bMap[15] = 9;

	aMap[0] = 0;
	aMap[1] = 1;
	aMap[2] = 4;
	aMap[3] = 5;
	aMap[4] = 3;
	aMap[5] = 6;
	aMap[6] = 12;
	aMap[7] = 9;
	aMap[8] = 15;
	aMap[9] = 14;
	aMap[10] = 11;
	aMap[11] = 10;
}

DRAM_Cfg_Periphery_SR4x4::~DRAM_Cfg_Periphery_SR4x4() {

}

int DRAM_Cfg_Periphery_SR4x4::get_bank_id(int coreId, int x, int y) {
	return bMap[coreId];
}

int DRAM_Cfg_Periphery_SR4x4::getNumDRAM() {
	return 12;
}

int DRAM_Cfg_Periphery_SR4x4::get_access_node(int dramId) {

	return aMap[dramId];
}
