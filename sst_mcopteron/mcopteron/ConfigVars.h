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


#ifndef CONFIGVARS_H
#define CONFIGVARS_H

namespace McOpteron{ //Scoggin: Added a namespace to reduce possible conflicts as library
class ConfigVars
{
 public:
   ConfigVars(const char *configFilename=0);
   ~ConfigVars();
   int readConfigFile(const char *filename);
   int addDomain(const char *domain);
   int useDomain(const char *domain=0);
   int addVariable(const char *name, const char *value);
   const char* findVariable(const char *name);
   int removeVariable(const char *name);
 private:
   struct Variable {
      const char *name;
      const char *value;
      struct Variable *next;
   };
   struct Domain {
      const char *name;
      struct Variable *vars;
      struct Domain *next;
   };
   Variable* findVariableRec(const char *name);
   Domain *domains;
   Domain *domainInUse;
};
}//end namespace McOpteron
#endif
