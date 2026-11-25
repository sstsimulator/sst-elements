// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// There's a reasonable chance that we'll want the globals/TLS support eventually,
// so I'm leaving that sst-macro code here for now -- JPK

#pragma once

#define SSTPP_QUOTE(name) #name
#define SSTPP_STR(name) SSTPP_QUOTE(name)

#if SST_HG_EXTERNAL
#define SST_APP_NAME_QUOTED __FILE__ __DATE__
#ifdef __cplusplus
#define SST_DEFINE_EXE_NAME extern "C" const char exe_main_name[] = SST_APP_NAME_QUOTED;
#else
#define SST_DEFINE_EXE_NAME extern const char exe_main_name[] = SST_APP_NAME_QUOTED;
#endif
#else
#define SST_APP_NAME_QUOTED SSTPP_STR(ssthg_app_name)
#define SST_DEFINE_EXE_NAME
#endif

#define ELI_NAME(app) app##_eli
typedef int (*main_fxn)(int,char**);
typedef int (*empty_main_fxn)();

#ifndef __cplusplus
#include <stdbool.h>
#else
/* hate that I have to do this for cmake */
#if __cplusplus < 201103L
#define char16_t char16type
#define char32_t char32type
#include <sstream>
#undef char16_t
#undef char32_t
#endif

#include <mercury/operating_system/process/cppglobal.h>

#if __cplusplus >= 201103L
namespace SST {
class Params;
}
#endif
#endif /* __cplusplus */

#define define_var_name_pass_through(x) ssthg_dont_ignore_this##x
#define define_var_name(x) define_var_name_pass_through(x)

#ifdef __cplusplus
/* Main refactoring uses a static initializer with a function call, which is not
 * valid in C (compile-time constant required). Only apply for C++. */
#ifndef SSTHG_NO_REFACTOR_MAIN
#undef main
#define main USER_MAIN
#define USER_MAIN(...) \
 fxn_that_nobody_ever_uses_to_make_magic_happen(); \
 typedef int (*this_file_main_fxn)(__VA_ARGS__); \
 int userSkeletonMainInitFxn(const char* name, this_file_main_fxn fxn); \
 static int userSkeletonMain(__VA_ARGS__); \
 SST_DEFINE_EXE_NAME \
 static int dont_ignore_this = \
  userSkeletonMainInitFxn(SST_APP_NAME_QUOTED, userSkeletonMain); \
 static int userSkeletonMain(__VA_ARGS__)
#else
#define main ssthg_ignore_for_app_name(); int main
#endif
#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif
extern int ssthg_global_stacksize;
extern char* static_init_glbls_segment;
extern char* static_init_tls_segment;
void sst_hg_init_global_space(void* ptr, int size, int offset, bool tls);
void sst_hg_advance_time(const char* param_name);
void sst_hg_blocking_call(int condition, double timeout, const char* api);
/* Alias for hgcc-generated pragma: ssthg_blocking_call -> sst_hg_blocking_call */
#define ssthg_blocking_call sst_hg_blocking_call
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace SST {
namespace Hg {

unsigned int ssthg_sleep(unsigned int secs);
unsigned int ssthg_usleep(unsigned int usecs);
unsigned int ssthg_nanosleep(unsigned int nsecs);

} // namespace Hg
} // namespace SST

#endif /* __cplusplus */

#include <mercury/common/skeleton_tls.h>
#include <mercury/common/null_buffer.h>
