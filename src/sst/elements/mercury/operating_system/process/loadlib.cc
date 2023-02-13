/**
Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government 
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly 
owned subsidiary of Honeywell International, Inc., for the U.S. Department of 
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2021, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#include <common/errors.h>

#include <dlfcn.h>
#include <vector>
#include <string>
#include <cstring>
#include <sys/stat.h>

namespace SST {
namespace Hg {

static std::vector<std::string> split_path(const std::string& searchPath)
{
  std::vector<std::string> paths;
  char * pathCopy = new char [searchPath.length() + 1];
  std::strcpy(pathCopy, searchPath.c_str());
  char *brkb = NULL;
  char *p = NULL;
  for ( p = strtok_r(pathCopy, ":", &brkb); p ; p = strtok_r(NULL, ":", &brkb) ) {
    paths.push_back(p);
  }

  delete [] pathCopy;
  return paths;
}

std::string loadExternPathStr(){
  const char* libpath_str = getenv("SST_LIB_PATH");
  if (libpath_str){
    return libpath_str;
  } else {
    return "";
  }
}

void* loadExternLibrary(const std::string& libname, const std::string& searchPath)
{
  struct stat sbuf;
  int ret = stat(libname.c_str(), &sbuf);
  std::string fullpath;
  if (ret != 0){
    std::vector<std::string> paths = split_path(searchPath);
    //always include current directory
    paths.push_back(".");

    for (auto&& path : paths) {
      fullpath = path + "/" + libname;
      ret = stat(fullpath.c_str(), &sbuf);
      if (ret == 0) break;
    }
  } else {
    fullpath = libname;
  }

  if (ret != 0){
    //didn't find it
    sst_hg_abort_printf("%s not found in current directory or in path=%s",
                      libname.c_str(), searchPath.c_str());
  }

  //std::cerr << "Loading external library " << fullpath << std::endl;

  // This is a little weird, but always try the last path - if we
  // didn't succeed in the stat, we'll get a file not found error
  // from dlopen, which is a useful error message for the user.
  void* handle = dlopen(fullpath.c_str(), RTLD_NOW|RTLD_GLOBAL);
  if (handle == NULL) {
    std::cerr << dlerror();
    sst_hg_abort_printf("Opening library %s failed", libname.c_str());
  }
  return handle;
}

void* loadExternLibrary(const std::string& libname)
{
  return loadExternLibrary(libname, loadExternPathStr());
}

void unloadExternLibrary(void* handle)
{
  // for sst-core parallel simulation, dlclosing causes segfault (GlobalVariableContext destructor)
  //dlclose(handle);
}

} // end namespace Hg
} // end namespace SST
