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

#define _ISOC99_SOURCE // llabs was added in C99
#include <common/timestamp.h>
#include <common/errors.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <limits>
#include <stdio.h>
#include <math.h>
#include <common/util.h>
#include <common/output.h>

namespace SST {
namespace Hg {

//
// Static variables.
//
TimeDelta::tick_t TimeDelta::ASEC_PER_TICK = 100;
TimeDelta::tick_t TimeDelta::one_femtosecond = 1000/TimeDelta::ASEC_PER_TICK;
TimeDelta::tick_t TimeDelta::one_picosecond = TimeDelta::one_femtosecond * 1000;
TimeDelta::tick_t TimeDelta::one_nanosecond = TimeDelta::one_picosecond * 1000;
TimeDelta::tick_t TimeDelta::one_microsecond = 1000 * TimeDelta::one_nanosecond;
TimeDelta::tick_t TimeDelta::one_millisecond = 1000 * TimeDelta::one_microsecond;
TimeDelta::tick_t TimeDelta::one_second = 1000 * TimeDelta::one_millisecond; //default is 1 tick per ps
TimeDelta::tick_t TimeDelta::one_minute = TimeDelta::one_second * 60;
double TimeDelta::s_per_tick  = 1.0/TimeDelta::one_second;
double TimeDelta::ms_per_tick = 1.0/TimeDelta::one_millisecond;
double TimeDelta::us_per_tick = 1.0/TimeDelta::one_microsecond;
double TimeDelta::ns_per_tick = 1.0/TimeDelta::one_nanosecond;
double TimeDelta::ps_per_tick = 1.0/TimeDelta::one_picosecond;
double TimeDelta::fs_per_tick = 1.0/TimeDelta::one_femtosecond;
double TimeDelta::max_time_;

static std::string _tick_spacing_string_("100as");


void TimeDelta::initStamps(tick_t tick_spacing)
{
  static bool inited_ = false;
  if (inited_) return;

  //psec_tick_spacing_ = new tick_t(tick_spacing);
  ASEC_PER_TICK = tick_spacing;
  std::stringstream ss;
  ss << tick_spacing << " as";
  _tick_spacing_string_ = ss.str();
  one_femtosecond = 1000/ASEC_PER_TICK;
  one_picosecond = 1000*one_femtosecond;
  one_nanosecond = 1000 * one_picosecond;
  one_microsecond = 1000 * one_nanosecond;
  one_millisecond = 1000 * one_microsecond;
  one_second = 1000 * one_millisecond;
  one_minute = 60 * one_second;
  s_per_tick = 1.0/one_second;
  ms_per_tick = 1.0/one_millisecond;
  us_per_tick = 1.0/one_microsecond;
  ns_per_tick = 1.0/one_nanosecond;
  ps_per_tick = 1.0/one_picosecond;
  fs_per_tick = 1.0/one_femtosecond;
  max_time_ = std::numeric_limits<tick_t>::max() / one_second;
}

//
// Return the current time in seconds.
//
double TimeDelta::sec() const
{
  return ticks_ * s_per_tick;
}

//
// Return the current time in milliseconds.
//
double TimeDelta::msec() const
{
  return ticks_ * ms_per_tick;
}

//
// Return the current time in microseconds.
//
double TimeDelta::usec() const
{
  return ticks_ * us_per_tick;
}

//
// Return the current time in nanoseconds.
//
double TimeDelta::nsec() const
{
  return ticks_ * ns_per_tick;
}

//
// Return the current time in picoseconds.
//
double TimeDelta::psec() const
{
  return ticks_ * ps_per_tick;
}

Timestamp& Timestamp::operator+=(const TimeDelta& t)
{
  uint64_t sum = time.ticks() + t.ticks();
  uint64_t carry = (sum & Timestamp::carry_bits_mask) << Timestamp::carry_bits_shift;
  uint64_t rem = sum & Timestamp::remainder_bits_mask;
  epochs += carry;
  time.ticks_ = rem;
  return *this;
}

//
// static:  Get the tick interval.
//
TimeDelta::tick_t TimeDelta::tickInterval()
{
  return ASEC_PER_TICK;
}

//
// Get the tick interval in std::string form (for example, "1ps").
//
const std::string& TimeDelta::tickIntervalString()
{
  return _tick_spacing_string_;
}

//
// Add.
//
TimeDelta& TimeDelta::operator+=(const TimeDelta &other)
{
  ticks_ += other.ticks_;
  return *this;
}

//
// Subtract.
//
TimeDelta& TimeDelta::operator-=(const TimeDelta &other)
{
  ticks_ -= other.ticks_;
  return *this;
}

//
// Multiply.
//
TimeDelta& TimeDelta::operator*=(double scale)
{
  ticks_ *= scale;
  return *this;
}

//
// Divide.
//
TimeDelta& TimeDelta::operator/=(double scale)
{
  ticks_ /= scale;
  return *this;
}

TimeDelta operator+(const TimeDelta &a, const TimeDelta &b)
{
  TimeDelta rv(a);
  rv += b;
  return rv;
}

TimeDelta operator-(const TimeDelta &a, const TimeDelta &b)
{
  TimeDelta rv(a);
  rv -= b;
  return rv;
}

TimeDelta operator*(const TimeDelta &t, double scaling)
{
  TimeDelta rv(t);
  rv *= scaling;
  return rv;
}

TimeDelta operator*(double scaling, const TimeDelta &t)
{
  return (t * scaling);
}

TimeDelta operator/(const TimeDelta &t, double scaling)
{
  TimeDelta rv(t);
  rv /= scaling;
  return rv;
}

std::ostream& operator<<(std::ostream &os, const TimeDelta &t)
{
  /*  timestamp::tick_t psec = t.ticks() / t.tick_interval();
    timestamp::tick_t frac = psec % timestamp::tick_t(1e12);
    timestamp::tick_t secs = psec / timestamp::tick_t(1e12);
    os << "timestamp(" << secs << "."
       << std::setw(12) << std::setfill('0') << frac << " sec)";*/

  os << "timestamp(" << t.sec() << " secs)";
  return os;
}

std::string
to_printf_type(TimeDelta t)
{
  return SST::Hg::sprintf("%8.4e msec", t.msec());
}

} // end namespace Hg
} // end namespace SST
