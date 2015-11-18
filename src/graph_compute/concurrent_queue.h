#ifndef CONCURRENT_QUEUE_H_
#define CONCURRENT_QUEUE_H_

#include <iostream>
#include "../libgraphio/libgraphio.h"

using namespace std;

struct mrmw_queue_t {
  volatile vid_t * const data;
  const vid_t sentinel;
  const size_t mask;
  volatile size_t head;
  volatile size_t tail;
  volatile int lock;
  mrmw_queue_t(volatile vid_t * const _data, size_t _numBits,
               const size_t _sentinel = static_cast<size_t>(-1)) :
               data(_data), sentinel(_sentinel), mask((1 << _numBits) - 1),
               head(0), tail(0), lock(0) { }
  // assumes that it is not possible to overflow
  void push(const vid_t value);  //  So, push always succeeds
  vid_t pop();  //  pop can return sentinel, if it is full or contended
};
typedef struct mrmw_queue_t mrmw_queue_t;

inline void mrmw_queue_t::push(const vid_t value) {
  while (__sync_lock_test_and_set(&lock, 1) == 1) {
    while (lock == 1) {}
  }
  data[tail & mask] = value;
  tail++;
  __sync_lock_release(&lock);
}

inline vid_t mrmw_queue_t::pop() {
  if ((lock == 1) || (tail == head)) {
    return sentinel;
  }
  while (__sync_lock_test_and_set(&lock, 1) == 1) {
    return sentinel;
  }
  if (tail == head) {
    __sync_lock_release(&lock);
    return sentinel;
  } else {
    vid_t value = data[head & mask];
    head++;
    __sync_lock_release(&lock);
    return value;
  }
}

#endif  // CONCURRENT_QUEUE_H_
