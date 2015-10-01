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

#include <iostream>
using std::cout;
using std::endl;
#include "stringutil.h"
using std::string;
using std::vector;
using std::stringstream;
namespace StringUtil{
vector<string> &split_string(const string &s, char delim, vector<string> &elems) {
	stringstream ss(s);
	string item;
	while(getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}


vector<string> split_string(const string &s, char delim) {
	vector<string> elems;
	return split_string(s, delim, elems);
}

string stringToken(string *str,string aFormat){
	static size_t idx =0, previdx = 0;
	static string *tokenstring;
	//cout<<"stringToken!!!!!"<<endl;
	if(str){
		tokenstring=str;
		idx=0;
		previdx=0;
	}
	idx =1+ tokenstring->find_first_of(aFormat, previdx);
	if(!idx) idx=tokenstring->npos;
	//cout<<"'"<<*tokenstring<<"'("<<tokenstring->size()<<") "<<previdx<<":"<<idx<<endl;
	string retval = tokenstring->substr(previdx, (idx - previdx-1));
	previdx = idx;
	return retval;
}
}//end namespace StringUtil
