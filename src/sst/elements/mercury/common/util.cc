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

#include <mercury/common/util.h>
#include <mercury/components/operating_system.h>
#include <mercury/operating_system/process/app.h>

#include <cstring>

extern template class  SST::Hg::HgBase<SST::Component>;
extern template class  SST::Hg::HgBase<SST::SubComponent>;

typedef int (*main_fxn)(int,char**);
typedef int (*empty_main_fxn)();

extern "C" void ssthg_exit(int code)
{
  SST::Hg::OperatingSystem::currentThread()->kill(code);
}

extern "C" unsigned int ssthg_alarm(unsigned int  /*delay*/)
{
  //for now, do nothing
  //seriously, why are you using the alarm function?
  return 0;
}

extern "C" int ssthg_on_exit(void(* /*fxn*/)(int,void*), void*  /*arg*/)
{
  //for now, just ignore any atexit functions
  return 0;
}

extern "C" int ssthg_atexit(void (* /*fxn*/)(void))
{
  //for now, just ignore any atexit functions
  return 0;
}

#include <unordered_map>

int
userSkeletonMainInitFxn(const char* name, main_fxn fxn)
{
  SST::Hg::UserAppCxxFullMain::registerMainFxn(name, fxn);
  return 42;
}

int
userSkeletonMainInitFxn(const char* name, empty_main_fxn fxn)
{
  SST::Hg::UserAppCxxEmptyMain::registerMainFxn(name, fxn);
  return 42;
}

extern "C"
char* ssthg_getenv(const char* name)
{
  SST::Hg::App* a = SST::Hg::OperatingSystem::currentThread()->parentApp();
  return a->getenv(name);
}

extern "C"
int ssthg_setenv(const char* name, const char* value, int overwrite)
{
  SST::Hg::App* a = SST::Hg::OperatingSystem::currentThread()->parentApp();
  return a->setenv(name, value, overwrite);
}

extern "C"
int ssthg_putenv(char* input)
{
  SST::Hg::App* a = SST::Hg::OperatingSystem::currentThread()->parentApp();
  return a->putenv(input);
}

