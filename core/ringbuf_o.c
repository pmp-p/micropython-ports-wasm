/*
  ringbuf_o.c - Library for implementing a simple Ring Buffer on Arduino boards.
  Created by D. Aaron Wisner (daw268@cornell.edu)
  January 17, 2015.
  Released into the public domain.
*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ringbuf_o.h"



typedef struct rbo_t {
  // Invariant: end and start is always in bounds
  unsigned char *buf;
  unsigned int len, size, start, end, elements;

  // Private:
  int (*next_end_index) (rbo_t*);
  int (*incr_end_index) (rbo_t*);

  int (*incr_start_index) (rbo_t*);

  //public:
  // Returns true if full
  bool (*isFull) (rbo_t*);
  // Returns true if empty
  bool (*isEmpty) (rbo_t*);
  // Returns number of elemnts in buffer
  unsigned int (*numElements)(rbo_t*);
  // Add Event, Returns index where added in buffer, -1 on full buffer
  int (*add) (rbo_t*, const void*);
  // Returns pointer to nth element, NULL when nth element is empty
  void *(*peek) (rbo_t*, unsigned int);
  // Removes element and copies it to location pointed to by void *
  // Returns pointer passed in, NULL on empty buffer
  void *(*pull) (rbo_t*, void *);

} rbo_t;

/////// Constructor //////////
rbo_t*
rbo_t_new(int size, int len) {
  rbo_t *self = (rbo_t *)malloc(sizeof(rbo_t));
  if (!self) return NULL;
  memset(self, 0, sizeof(rbo_t));
  if (rbo_init(self, size, len) < 0)
  {
    free(self);
    return NULL;
  }
  return self;
}

int
rbo_init(rbo_t *self, int size, int len) {
  self->buf = (unsigned char *)malloc(size*len);
  if (!self->buf) return -1;
  memset(self->buf, 0, size*len);

  self->size = size;
  self->len = len;
  self->start = 0;
  self->end = 0;
  self->elements = 0;

  self->next_end_index = &rbo_next_end_index;
  self->incr_end_index = &rbo_incr_end;
  self->incr_start_index = &rbo_incr_start;
  self->isFull = &rbo_is_full;
  self->isEmpty = &rbo_is_empty;
  self->add = &rbo_append;
  self->numElements = &rbo_count;
  self->peek = &rbo_peek;
  self->pull = &rbo_pop;
  return 0;
}
/////// Deconstructor //////////
int
rbo_delete(rbo_t *self) {
  free(self->buf);
  free(self);
  return 0;
}

/////// PRIVATE METHODS //////////

// get next empty index
int
rbo_next_end_index(rbo_t *self) {
  //buffer is full
  if (self->isFull(self)) return -1;
  //if empty dont incriment
  return (self->end+(unsigned int)!self->isEmpty(self))%self->len;
}

// incriment index of rbo_t struct, only call if safe to do so
int
rbo_incr_end(rbo_t *self) {
  self->end = (self->end+1)%self->len;
  return self->end;
}


// incriment index of rbo_t struct, only call if safe to do so
int
rbo_incr_start(rbo_t *self) {
  self->start = (self->start+1)%self->len;
  return self->start;
}

/////// PUBLIC METHODS //////////

// Add an object struct to rbo_t
int
rbo_append(rbo_t *self, const void *object) {
  int index;
  // Perform all atomic opertaions
  RB_ATOMIC_START
  {
    index = self->next_end_index(self);
    //if not full
    if (index >= 0)
    {
      memcpy(self->buf + index*self->size, object, self->size);
      if (!self->isEmpty(self)) self->incr_end_index(self);
      self->elements++;
    }
  }
  RB_ATOMIC_END

  return index;
}

// Return pointer to num element, return null on empty or num out of bounds
void*
rbo_peek(rbo_t *self, unsigned int num) {
  void *ret = NULL;
  // Perform all atomic opertaions
  RB_ATOMIC_START
  {
    //empty or out of bounds
    if (self->isEmpty(self) || num > self->elements - 1) ret = NULL;
    else ret = &self->buf[((self->start + num)%self->len)*self->size];
  }
  RB_ATOMIC_END

  return ret;
}

// Returns and removes first buffer element
void*
rbo_pop(rbo_t *self, void *object) {
  void *ret = NULL;
  // Perform all atomic opertaions
  RB_ATOMIC_START
  {
    if (self->isEmpty(self)) ret = NULL;
    // Else copy Object
    else
    {
      memcpy(object, self->buf+self->start*self->size, self->size);
      self->elements--;
      // don't increment start if removing last element
      if (!self->isEmpty(self)) self->incr_start_index(self);
      ret = object;
    }
  }
  RB_ATOMIC_END

  return ret;
}

// Returns number of elemnts in buffer
unsigned int
rbo_count(rbo_t *self) {
  unsigned int elements;

  // Perform all atomic opertaions
  RB_ATOMIC_START
  {
    elements = self->elements;
  }
  RB_ATOMIC_END

  return elements;
}

// Returns true if buffer is full
bool
rbo_is_full(rbo_t *self) {
  bool ret;

  // Perform all atomic opertaions
  RB_ATOMIC_START
  {
    ret = self->elements == self->len;
  }
  RB_ATOMIC_END

  return ret;
}

// Returns true if buffer is empty
bool
rbo_is_empty(rbo_t *self) {
  bool ret;

  // Perform all atomic opertaions
  RB_ATOMIC_START
  {
    ret = !self->elements;
  }
  RB_ATOMIC_END

  return ret;
}
