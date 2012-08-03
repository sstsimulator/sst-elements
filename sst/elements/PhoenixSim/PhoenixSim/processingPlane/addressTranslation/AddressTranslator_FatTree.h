//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#ifndef ADDRESSTRANSLATOR_FATTREE_H_
#define ADDRESSTRANSLATOR_FATTREE_H_

#include "AddressTranslator.h"

#define DRAM_SLOT "DRAM"
#define PROC_SLOT "PROC"

#define NET1_SLOT "NET1"
#define NET2_SLOT "NET2"
#define NET3_SLOT "NET3"
#define NET4_SLOT "NET4"

class AddressTranslator_FatTree : public AddressTranslator{
public:
	AddressTranslator_FatTree();
	virtual ~AddressTranslator_FatTree();

	virtual NetworkAddress* translateAddr(ApplicationData* adata);

};

#endif
