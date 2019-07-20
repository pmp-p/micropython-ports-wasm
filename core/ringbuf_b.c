

/******************************************************************************

                  Copyright (c) 2018 EmbedJournal

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

    Author : Siddharth Chandrasekaran
    Email  : siddharth@embedjournal.com
    Date   : Sun Aug  5 09:42:31 IST 2018

******************************************************************************/

#include "ringbuf_b.h"

int rbb_append(rbb_t *c, uint8_t data)
{
    int next;

    next = c->head + 1;  // next is where head will point to after this write.
    if (next >= c->maxlen)
        next = 0;

    // if the head + 1 == tail, circular buffer is full. Notice that one slot
    // is always left empty to differentiate empty vs full condition
    if (next == c->tail)
        return -1;

    c->buffer[c->head] = data;  // Load data and then move
    c->head = next;             // head to next data offset.
    return 0;  // return success to indicate successful push.
}

int rbb_is_empty(rbb_t *c) {
    return (c->head == c->tail); // if the head == tail, we don't have any data
}


int rbb_pop(rbb_t *c, uint8_t *data)
{
    int next;

    if (c->head == c->tail)  // if the head == tail, we don't have any data
        return 0;

    next = c->tail + 1;  // next is where tail will point to after this read.
    if(next >= c->maxlen)
        next = 0;

    *data = c->buffer[c->tail];  // Read data and then move
    c->tail = next;              // tail to next offset.
    return 1;  // return success to indicate successful pop.
}

int rbb_free_space(rbb_t *c)
{
    int freeSpace;
    freeSpace = c->tail - c->head;
    if (freeSpace <= 0)
        freeSpace += c->maxlen;
    return freeSpace - 1; // -1 to account for the always-empty slot.
}

#ifdef C_UTILS_TESTING
/* To test this module,
 * $ gcc -Wall -DC_UTILS_TESTING rinbguf_b.c
 * $ ./a.out
*/

RBB_T(my_circ_buf, 32);

#include <stdio.h>

int main()
{
    uint8_t out_data=0, in_data = 0x55;

    if (rbb_append(&my_circ_buf, in_data)) {
        printf("Out of space in CB\n");
        return -1;
    }

    if (rbb_pop(&my_circ_buf, &out_data)) {
        printf("CB is empty\n");
        return -1;
    }

    printf("Push: 0x%x\n", in_data);
    printf("Pop:  0x%x\n", out_data);
    return 0;
}

#endif
