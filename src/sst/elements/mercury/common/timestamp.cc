// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

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
//TimeDelta::tick_t TimeDelta::one_picosecond = TimeDelta::one_femtosecond * 1000;
TimeDelta::tick_t TimeDelta::one_picosecond = 1;
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
