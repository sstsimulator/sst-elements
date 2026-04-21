// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// C-linkage wrappers for hgcc replacement headers (sleep, usleep, gettimeofday,
// clock_gettime, nanosleep, getenv). These bridge to mercury's simulated time
// and environment when running under SST.
// Do not include skeleton.h here (it redefines main and breaks SST core headers).

#include <mercury/common/timestamp.h>
#include <mercury/operating_system/process/thread.h>

#include <sys/time.h>
#include <time.h>
#include <cstdint>

namespace SST::Hg {
extern unsigned int ssthg_sleep(unsigned int secs);
extern unsigned int ssthg_usleep(unsigned int usecs);
extern unsigned int ssthg_nanosleep(unsigned int nsecs);
}

extern "C" char* ssthg_getenv(const char* name);

extern "C" {

unsigned int hgcc_sleep(unsigned int secs)
{
  return SST::Hg::ssthg_sleep(secs);
}

int hgcc_usleep(unsigned int usecs)
{
  SST::Hg::ssthg_usleep(usecs);
  return 0;
}

char* hgcc_getenv(const char* name)
{
  return ssthg_getenv(name);
}

int SSTMAC_gettimeofday(struct timeval* tv, struct timezone* tz)
{
  if (!tv) return 0;
  SST::Hg::Thread* t = SST::Hg::Thread::current();
  if (!t) return -1;
  SST::Hg::Timestamp now = t->now();
  uint64_t usec = now.usecRounded();
  tv->tv_sec = static_cast<time_t>(usec / 1000000ULL);
  tv->tv_usec = static_cast<suseconds_t>(usec % 1000000ULL);
  if (tz) {
    tz->tz_minuteswest = 0;
    tz->tz_dsttime = 0;
  }
  return 0;
}

int HGCC_clock_gettime(clockid_t /*id*/, struct timespec* ts)
{
  if (!ts) return 0;
  SST::Hg::Thread* t = SST::Hg::Thread::current();
  if (!t) return -1;
  SST::Hg::Timestamp now = t->now();
  uint64_t nsec = now.nsecRounded();
  ts->tv_sec = static_cast<time_t>(nsec / 1000000000ULL);
  ts->tv_nsec = static_cast<long>(nsec % 1000000000ULL);
  return 0;
}

int hgcc_ts_nanosleep(const struct timespec* req, struct timespec* rem)
{
  if (!req) return -1;
  uint64_t total_nsec = static_cast<uint64_t>(req->tv_sec) * 1000000000ULL
    + static_cast<uint64_t>(req->tv_nsec);
  const unsigned int max_nsec = static_cast<unsigned int>(-1);
  unsigned int nsecs = (total_nsec > max_nsec) ? max_nsec : static_cast<unsigned int>(total_nsec);
  SST::Hg::ssthg_nanosleep(nsecs);
  if (rem) {
    rem->tv_sec = 0;
    rem->tv_nsec = 0;
  }
  return 0;
}

} // extern "C"
