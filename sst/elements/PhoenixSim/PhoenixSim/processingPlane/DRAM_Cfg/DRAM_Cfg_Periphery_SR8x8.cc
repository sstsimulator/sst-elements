/*
 * DRAM_Cfg_Periphery.cc
 *
 *  Created on: Feb 18, 2009
 *      Author: Gilbert
 */

#include "DRAM_Cfg_Periphery_SR8x8.h"

DRAM_Cfg_Periphery_SR8x8::DRAM_Cfg_Periphery_SR8x8(int n, int pC) :
	DRAM_Cfg(n, pC) {
	bMap[0] = 0;
	bMap[1] = 1;
	bMap[2] = 8;
	bMap[3] = 8;
	bMap[4] = 2;
	bMap[5] = 3;
	bMap[6] = 3;
	bMap[7] = 2;
	bMap[8] = 1;
	bMap[9] = 3;
	bMap[10] = 12;
	bMap[11] = 12;
	bMap[12] = 10;
	bMap[13] = 10;
	bMap[14] = 12;
	bMap[15] = 12;

	bMap[16] = 4;
	bMap[17] = 5;
	bMap[18] = 5;
	bMap[19] = 4;
	bMap[20] = 6;
	bMap[21] = 7;
	bMap[22] = 9;
	bMap[23] = 6;
	bMap[24] = 11;
	bMap[25] = 11;
	bMap[26] = 13;
	bMap[27] = 13;
	bMap[28] = 4;
	bMap[29] = 6;
	bMap[30] = 13;
	bMap[31] = 13;

	bMap[32] = 15;
	bMap[33] = 15;
	bMap[34] = 26;
	bMap[35] = 24;
	bMap[36] = 15;
	bMap[37] = 15;
	bMap[38] = 17;
	bMap[39] = 17;
	bMap[40] = 19;
	bMap[41] = 19;
	bMap[42] = 27;
	bMap[43] = 26;
	bMap[44] = 24;
	bMap[45] = 25;
	bMap[46] = 25;
	bMap[47] = 24;

	bMap[48] = 14;
	bMap[49] = 14;
	bMap[50] = 16;
	bMap[51] = 16;
	bMap[52] = 14;
	bMap[53] = 14;
	bMap[54] = 23;
	bMap[55] = 21;
	bMap[56] = 22;
	bMap[57] = 23;
	bMap[58] = 23;
	bMap[59] = 22;
	bMap[60] = 18;
	bMap[61] = 18;
	bMap[62] = 21;
	bMap[63] = 20;

	aMap[0] = 0;
	aMap[1] = 1;
	aMap[2] = 4;
	aMap[3] = 5;
	aMap[4] = 16;
	aMap[5] = 17;
	aMap[6] = 20;
	aMap[7] = 21;
	aMap[8] = 3;
	aMap[9] = 22;
	aMap[10] = 12;
	aMap[11] = 25;
	aMap[12] = 15;
	aMap[13] = 26;
	aMap[14] = 48;
	aMap[15] = 37;
	aMap[16] = 51;
	aMap[17] = 38;
	aMap[18] = 60;
	aMap[19] = 41;
	aMap[20] = 63;
	aMap[21] = 62;
	aMap[22] = 59;
	aMap[23] = 58;
	aMap[24] = 47;
	aMap[25] = 46;
	aMap[26] = 43;
	aMap[27] = 42;
}


DRAM_Cfg_Periphery_SR8x8::~DRAM_Cfg_Periphery_SR8x8() {

}

int DRAM_Cfg_Periphery_SR8x8::get_bank_id(int coreId, int x, int y) {
	return bMap[coreId];
}

int DRAM_Cfg_Periphery_SR8x8::getNumDRAM() {
	return 28;
}

int DRAM_Cfg_Periphery_SR8x8::get_access_node(int dramId) {

	return aMap[dramId];
}
