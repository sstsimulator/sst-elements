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


#ifndef STRINGUTIL_H
#define STRINGUTIL_H 
#include <vector>
using std::vector;
#include <string>
using std::string;
#include <sstream>
namespace StringUtil{
vector<string> &split_string(const string &s, char delim, vector<string> &elems);


vector<string> split_string(const string &s, char delim);

string stringToken(string *str,string aFormat);
}//end namespace std
#endif
