

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
