// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.



//Scoggin: This file was created to actually create the global
//          that is incuded as an extern in OpteronDefs.h
#include "OpteronDefs.h"
#include <string>
using std::string;
namespace McOpteron{
	unsigned int Debug;

	inline string tokenize(string *str,string& aFormat)
	{
		//static iFormat = " ";
		static int idx =0, previdx = 0;
		static string *tokenstring;
		if(str){
			tokenstring=str;
			idx=0;
			previdx=0;
		}
		idx = tokenstring->find_first_of(aFormat, previdx);
		string retval = tokenstring->substr(previdx, (idx - previdx));
		previdx = idx;
		return retval;
	}
}
