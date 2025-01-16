// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#pragma once

#include <set>
#include <vector>
#include <list>
#include <map>
#include <sstream>

template <class Container>
std::string oneDimString(const Container& c, const char* open, const char* close){
  std::stringstream sstr;
  sstr << open;
  for (auto& item : c){
    sstr << " " << item;
  }
  sstr << " " << close;
  return sstr.str();
}

template <class T>
std::string stlString(const std::set<T>& t){
  return oneDimString(t, "{", "}");
}

template <class T>
std::string stlString(const std::vector<T>& t){
  return oneDimString(t, "[", "]");
}

template <class T>
std::string stlString(const std::list<T>& t){
  return oneDimString(t, "<", ">");
}
