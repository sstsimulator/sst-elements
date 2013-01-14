#include <qsim.h>
#include <iostream>
#include <vector>

#include "zesto-qsim.h"

// TODO: Migrate the stuff below to a header file/library

void Queue::inst_cb
  (int c, uint64_t va, uint64_t pa, uint8_t l, const uint8_t *b)
{
  if (cpu == c) this->push(Qsim::QueueItem(va, pa, l, b));
}

void Queue::mem_cb
(int c, uint64_t va, uint64_t pa, uint8_t s, int t)
{
  if (cpu == c) this->push(Qsim::QueueItem(va, pa, s, t));
}

int Queue::int_cb(int c, uint8_t v) {
  if (cpu == c) this->push(Qsim::QueueItem(v));
  return 0;
}
