/*********************************************************************************
*  Copyright (c) 2010-2011, Elliott Cooper-Balis
*                             Paul Rosenfeld
*                             Bruce Jacob
*                             University of Maryland 
*                             dramninjas [at] gmail [dot] com
*  All rights reserved.
*  
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*  
*     * Redistributions of source code must retain the above copyright notice,
*        this list of conditions and the following disclaimer.
*  
*     * Redistributions in binary form must reproduce the above copyright notice,
*        this list of conditions and the following disclaimer in the documentation
*        and/or other materials provided with the distribution.
*  
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
*  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
*  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
*  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
*  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*********************************************************************************/
#include "SystemConfiguration.h"
#include "AddressMapping.h"

namespace DRAMSim
{

void addressMapping(uint64_t physicalAddress, unsigned &newTransactionChan, unsigned &newTransactionRank, unsigned &newTransactionBank, unsigned &newTransactionRow, unsigned &newTransactionColumn)
{
  unsigned transactionSize    = (JEDEC_DATA_BUS_BITS/8)*BL; 
  uint64_t transactionMask    = transactionSize - 1; //ex: (64 bit bus width) x (8 Burst Length) - 1 = 64 bytes - 1 = 63 = 0x3f mask
  unsigned channelBitWidth    = dramsim_log2(NUM_CHANS);
  unsigned vaultBitWidth      = dramsim_log2(NUM_VAULTS);
  unsigned rankBitWidth       = dramsim_log2(NUM_RANKS);
  unsigned bankBitWidth       = dramsim_log2(NUM_BANKS);
  unsigned rowBitWidth        = dramsim_log2(NUM_ROWS);
  unsigned colBitWidth        = dramsim_log2(NUM_COLS);
  unsigned byteOffsetWidth    = dramsim_log2((JEDEC_DATA_BUS_BITS/8));
  unsigned cacheLineBitWidth  = dramsim_log2(transactionSize);

  unsigned colLowBitWidth     = colBitWidth - (cacheLineBitWidth - byteOffsetWidth);
  unsigned colHighBitWidth    = colBitWidth - colLowBitWidth;

  if ((physicalAddress & transactionMask) != 0) {
    DEBUG("WARNING: address 0x"<<std::hex<<physicalAddress<<std::dec<<" is not aligned to the request size of "<<transactionSize); 
  }

  uint64_t physicalAddress_old = physicalAddress;

  uint64_t tempA, tempB;
  if (addressMappingScheme == Scheme1) { // Cube | ColHigh | Row | Rank  | Vault | Bank  | ColLow | Byte
    tempA = physicalAddress;
    physicalAddress >>= bankBitWidth;
    tempB = physicalAddress << bankBitWidth;
    newTransactionBank = tempA ^ tempB;

    tempA = physicalAddress;
    physicalAddress >>= vaultBitWidth;
    tempB = physicalAddress << vaultBitWidth;
    newTransactionChan = tempA ^ tempB;

    tempA = physicalAddress;
    physicalAddress >>= rankBitWidth;
    tempB = physicalAddress << rankBitWidth;
    newTransactionRank = tempA ^ tempB;

    tempA = physicalAddress;
    physicalAddress >>= rowBitWidth;
    tempB = physicalAddress << rowBitWidth;
    newTransactionRow = tempA ^ tempB;

    tempA = physicalAddress;
    physicalAddress >>= colHighBitWidth;
    tempB = physicalAddress << colHighBitWidth;
    newTransactionColumn = tempA ^ tempB;
  } else if (addressMappingScheme == Scheme2) {
    // Cube | ColHigh | Row | Vault | Bank  | Rank  | ColLow | Byte
    tempA = physicalAddress;
    physicalAddress >>= rankBitWidth;
    tempB = physicalAddress << rankBitWidth;
    newTransactionRank = tempA ^ tempB;

    tempA = physicalAddress;
    physicalAddress >>= bankBitWidth;
    tempB = physicalAddress << bankBitWidth;
    newTransactionBank = tempA ^ tempB;

    tempA = physicalAddress;
    physicalAddress >>= vaultBitWidth;
    tempB = physicalAddress << vaultBitWidth;
    newTransactionChan = tempA ^ tempB;

    tempA = physicalAddress;
    physicalAddress >>= rowBitWidth;
    tempB = physicalAddress << rowBitWidth;
    newTransactionRow = tempA ^ tempB;

    tempA = physicalAddress;
    physicalAddress >>= colHighBitWidth;
    tempB = physicalAddress << colHighBitWidth;
    newTransactionColumn = tempA ^ tempB;  
  } else if (addressMappingScheme == Scheme3) { // Cube | ColHigh | Row | Bank  | Vault | Rank  | ColLow | Byte
    tempA = physicalAddress;
    physicalAddress >>= rankBitWidth;
    tempB = physicalAddress << rankBitWidth;
    newTransactionRank = tempA ^ tempB;

    tempA = physicalAddress;
    physicalAddress >>= vaultBitWidth;
    tempB = physicalAddress << vaultBitWidth;
    newTransactionChan = tempA ^ tempB;

    tempA = physicalAddress;
    physicalAddress >>= bankBitWidth;
    tempB = physicalAddress << bankBitWidth;
    newTransactionBank = tempA ^ tempB;

    tempA = physicalAddress;
    physicalAddress >>= rowBitWidth;
    tempB = physicalAddress << rowBitWidth;
    newTransactionRow = tempA ^ tempB;

    tempA = physicalAddress;
    physicalAddress >>= colHighBitWidth;
    tempB = physicalAddress << colHighBitWidth;
    newTransactionColumn = tempA ^ tempB;
  } else if (addressMappingScheme == Scheme4) { // Cube | ColHigh | Row | Bank  | Rank  | Vault | ColLow | Byte
    tempA = physicalAddress;
    physicalAddress >>= vaultBitWidth;
    tempB = physicalAddress << vaultBitWidth;
    newTransactionChan = tempA ^ tempB;

    tempA = physicalAddress;
    physicalAddress >>= rankBitWidth;
    tempB = physicalAddress << rankBitWidth;
    newTransactionRank = tempA ^ tempB;

    tempA = physicalAddress;
    physicalAddress >>= bankBitWidth;
    tempB = physicalAddress << bankBitWidth;
    newTransactionBank = tempA ^ tempB;

    tempA = physicalAddress;
    physicalAddress >>= rowBitWidth;
    tempB = physicalAddress << rowBitWidth;
    newTransactionRow = tempA ^ tempB;

    tempA = physicalAddress;
    physicalAddress >>= colHighBitWidth;
    tempB = physicalAddress << colHighBitWidth;
    newTransactionColumn = tempA ^ tempB; 
  } else if (addressMappingScheme == Scheme5) { // Cube | ColHigh | Row | Rank  | Bank  | Vault | ColLow | Byte
    tempA = physicalAddress;
    physicalAddress >>= vaultBitWidth;
    tempB = physicalAddress << vaultBitWidth;
    newTransactionChan = tempA ^ tempB;

    tempA = physicalAddress;
    physicalAddress >>= bankBitWidth;
    tempB = physicalAddress << bankBitWidth;
    newTransactionBank = tempA ^ tempB;

    tempA = physicalAddress;
    physicalAddress >>= rankBitWidth;
    tempB = physicalAddress << rankBitWidth;
    newTransactionRank = tempA ^ tempB;

    tempA = physicalAddress;
    physicalAddress >>= rowBitWidth;
    tempB = physicalAddress << rowBitWidth;
    newTransactionRow = tempA ^ tempB;

    tempA = physicalAddress;
    physicalAddress >>= colHighBitWidth;
    tempB = physicalAddress << colHighBitWidth;
    newTransactionColumn = tempA ^ tempB;
  } else {
    ERROR("== Error - Unknown Address Mapping Scheme");
    exit(-1);
  }

  if (DEBUG_ADDR_MAP) {
    DEBUG("Mapped Ch="<<newTransactionChan<<" Rank="<<newTransactionRank
        <<" Bank="<<newTransactionBank<<" Row="<<newTransactionRow
        <<" Col="<<newTransactionColumn<<"\n"); 
  }
}
};
