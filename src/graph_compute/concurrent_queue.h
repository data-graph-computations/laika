#ifndef CONCURRENT_QUEUE_H_
#define CONCURRENT_QUEUE_H_

typedef struct mrmw_queue_t {
  vid_t * data;
  vid_t sentinel;
  size_t mask;
  size_t head;
  size_t tail;
  mrmw_queue_t(vid_t * _data, size_t _numBits, vid_t _sentinel = -1) {
    mask = (1 << _numBits) - 1;
    tail = 0;
    head = 0;
    sentinel = _sentinel;
    for (size_t i = 0; i <= mask; i++)
      data[i] = sentinel;
  }
  // assumes that it is not possible to overflow
  void push(vid_t value);
  vid_t pop();
};

void push(vid_t value) {
  size_t position = __sync_fetch_and_add(&tail, 1);
  while (data[position & mask] != sentinel) {}
  data[position & mask] = value;
}

vid_t pop() {
  while (tail != head) {
    size_t position = head;
    if (__sync_bool_compare_and_swap(&head, position, position+1)) {
      vid_t value = data[position & mask];
      data[position & mask] = sentinel;
      return value;
    }
  }
  return sentinel;
}
#endif  // CONCURRENT_QUEUE_H_
