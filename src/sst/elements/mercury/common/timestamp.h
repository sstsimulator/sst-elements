// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
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

#include <sst/core/sst_types.h>
#include <mercury/common/serializable.h>
#include <mercury/common/errors.h>
#include <iosfwd>
#include <stdint.h>
#include <iostream>
#include <limits>

namespace SST {
namespace Hg {

/**
 * A basic container for time (subject to future transplant).
 * Stores time as an integral number of picoseconds (tentatively).
 * With 1 psec resolution, a 64 bit int can hold roughly +/- 106 days.
 *
 * Intended to be reasonably compatible with ns3::HighPrecision time.
 */
class TimeDelta
{
 public:
  friend struct Timestamp;

  /// The type that holds a timestamp.
  typedef uint64_t tick_t;

  static tick_t ASEC_PER_TICK;
  static const tick_t zero = 0;
  static tick_t one_femtosecond;
  static tick_t one_picosecond;
  static tick_t one_nanosecond;
  static tick_t one_microsecond;
  static tick_t one_millisecond;
  static tick_t one_second;
  static tick_t one_minute;

  static double s_per_tick;
  static double ms_per_tick;
  static double us_per_tick;
  static double ns_per_tick;
  static double ps_per_tick;
  static double fs_per_tick;

 private:
  tick_t ticks_;
  static double max_time_;

 public:
  static void initStamps(tick_t tick_spacing);

  typedef enum { exact } timestamp_param_type_t;

  explicit TimeDelta(double t_seconds){
    ticks_ = uint64_t(t_seconds * one_second);
    if (t_seconds > maxTime()) {
      sst_hg_abort_printf("TimeDelta(): Time value %e out of bounds 0...%e",
                        t_seconds, maxTime());
    }
  }

  explicit TimeDelta(tick_t ticks, timestamp_param_type_t  /*ty*/) : ticks_(ticks) {}

  explicit TimeDelta(uint64_t num_units, tick_t ticks_per_unit) : ticks_(num_units*ticks_per_unit) {}

  explicit TimeDelta() : ticks_(0) {}

  static uint32_t divideUp(TimeDelta num, TimeDelta denom){
    //optimize for architectures that generate remainder bit
    //this should optimize
    uint32_t x = num.ticks();
    uint32_t y = denom.ticks();
    uint32_t xy = x/y;
    uint32_t res = (x % y) ? xy + 1 : xy;
    return res;
  }

  explicit operator SST::SimTime_t() const {
    return ticks_;
  }

  double sec() const;

  double msec() const;

  double usec() const;

  double nsec() const;

  double psec() const;

 public:
  inline tick_t ticks() const {
    return ticks_;
  }

  static tick_t tickInterval();

  static const std::string & tickIntervalString();

  static double maxTime(){
    return max_time_;
  }

  inline bool operator==(const TimeDelta &other) const {
    return (ticks_ == other.ticks_);
  }

  inline bool operator!=(const TimeDelta &other) const {
    return (ticks_ != other.ticks_);
  }

  inline bool operator<(const TimeDelta &other) const {
    return (ticks_ < other.ticks_);
  }

  inline bool operator<=(const TimeDelta &other) const {
    return (ticks_ <= other.ticks_);
  }

  inline bool operator>(const TimeDelta &other) const {
    return (ticks_ > other.ticks_);
  }

  inline bool operator>=(const TimeDelta &other) const {
    return (ticks_ >= other.ticks_);
  }

  TimeDelta& operator+=(const TimeDelta &other);
  TimeDelta& operator-=(const TimeDelta &other);
  TimeDelta& operator*=(double scale);
  TimeDelta& operator/=(double scale);
};

struct Timestamp
{
  friend class TimeDelta;

  explicit Timestamp() : epochs(0), time()
  {
  }

  explicit Timestamp(uint64_t eps, TimeDelta tcks) :
    epochs(eps), time(tcks)
  {
  }

  explicit Timestamp(uint64_t eps, uint64_t subticks) :
    epochs(eps), time(subticks, TimeDelta::exact)
  {
  }

  static Timestamp max(){
    return Timestamp(0, std::numeric_limits<uint64_t>::max());
  }

  bool empty() const {
    return time.ticks() == 0;
  }

  explicit Timestamp(double t) :
    epochs(0), time(t)
  {
  }

  double sec() const {
    return time.sec();
  }

  double usec() const {
    return time.usec();
  }

  double nsec() const {
    return time.nsec();
  }

  uint64_t usecRounded() const {
    return time.ticks() / time.one_microsecond;
  }

  uint64_t nsecRounded() const {
    return time.ticks() / time.one_nanosecond;
  }

  Timestamp& operator+=(const TimeDelta& t);

  uint64_t epochs;
  TimeDelta time;

  static constexpr uint64_t carry_bits_mask = 0;
  static constexpr uint64_t remainder_bits_mask = ~uint64_t(0);
  static constexpr uint64_t carry_bits_shift = 0;
};

TimeDelta operator+(const TimeDelta &a, const TimeDelta &b);
TimeDelta operator-(const TimeDelta &a, const TimeDelta &b);
TimeDelta operator*(const TimeDelta &t, double scaling);
TimeDelta operator*(double scaling, const TimeDelta &t);
TimeDelta operator/(const TimeDelta &t, double scaling);
static inline uint64_t operator/(const TimeDelta& a, const TimeDelta& b){
  return a.ticks() / b.ticks();
}

static inline Timestamp operator+(const Timestamp& a, const TimeDelta& b)
{
  uint64_t sum = a.time.ticks() + b.ticks();
  uint64_t carry = (sum & Timestamp::carry_bits_mask) << Timestamp::carry_bits_shift;
  uint64_t rem = sum & Timestamp::remainder_bits_mask;
  return Timestamp(carry + a.epochs, TimeDelta(rem, TimeDelta::exact));
}

static inline TimeDelta operator-(const Timestamp& a, const Timestamp& b){
  if (a.epochs == b.epochs){
    return a.time - b.time;
  } else {
    ::abort(); //TODO
  }
}

static inline Timestamp operator+(const TimeDelta& a, const Timestamp& b){
  return b + a;
}

static inline Timestamp operator+(const Timestamp& a, const Timestamp& b){
  Timestamp tmp = a.time + b;
  tmp.epochs += a.epochs;
  return tmp;
}

static inline Timestamp operator-(const Timestamp& a, const TimeDelta b){
  if (a.time > b){
    Timestamp tmp = a;
    tmp.time -= b;
    return tmp;
  } else {
    uint64_t ticks = Timestamp::remainder_bits_mask - b.ticks();
    Timestamp gt(a.epochs-1, TimeDelta(ticks, TimeDelta::exact));
    return gt;
  }
}

static inline bool operator>=(const Timestamp& a, const Timestamp& b){
  if (a.epochs == b.epochs){
    return a.time >= b.time;
  } else {
    return a.epochs >= b.epochs;
  }
}

static inline bool operator!=(const Timestamp& a, const Timestamp& b){
  if (a.epochs == b.epochs){
    return a.time != b.time;
  } else {
    return true;
  }
}

static inline bool operator<(const Timestamp& a, const Timestamp& b){
  if (a.epochs == b.epochs){
    return a.time < b.time;
  } else {
    return a.epochs < b.epochs;
  }
}

static inline bool operator==(const Timestamp& a, const Timestamp& b){
  return a.epochs == b.epochs && a.time == b.time;
}

static inline bool operator>(const Timestamp& a, const Timestamp& b){
  return b < a;
}

static inline bool operator<=(const Timestamp& a, const Timestamp& b){
  if (a.epochs == b.epochs){
    return a.time <= b.time;
  } else {
    return a.time < b.time;
  }
}

std::ostream& operator<<(std::ostream &os, const TimeDelta &t);

std::string to_printf_type(TimeDelta t);


} // end namespace Hg
} // end namespace SST

START_SERIALIZATION_NAMESPACE
template <> class serialize<SST::Hg::TimeDelta>
{
 public:
  void operator()(SST::Hg::TimeDelta& t, serializer& ser){
    ser.primitive(t);
  }
};

template <> class serialize<SST::Hg::Timestamp>
{
 public:
  void operator()(SST::Hg::Timestamp& t, serializer& ser){
    ser.primitive(t);
  }
};
END_SERIALIZATION_NAMESPACE
