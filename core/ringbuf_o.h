
/*
  rbo_t.h - Library for implementing a simple Ring Buffer on Arduino boards.
  Created by D. Aaron Wisner (daw268@cornell.edu)
  January 17, 2015.
  Released into the public domain.
*/
#ifndef RINGBUF_O_H
#define RINGBUF_O_H

#ifndef __EMSCRIPTEN__
#include "Arduino.h"
#endif

#ifndef __cplusplus
#ifndef bool
#define bool uint8_t
#endif
#endif

#if defined(ARDUINO_ARCH_AVR)
    #include <util/atomic.h>
    #define RB_ATOMIC_START ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    #define RB_ATOMIC_END }


#elif defined(ARDUINO_ARCH_ESP8266)
    #ifndef __STRINGIFY
    #define __STRINGIFY(a) #a
    #endif

    #ifndef xt_rsil
        #define xt_rsil(level) (__extension__({uint32_t state; __asm__ __volatile__("rsil %0," __STRINGIFY(level) : "=a" (state)); state;}))
    #endif

    #ifndef xt_wsr_ps
        #define xt_wsr_ps(state)  __asm__ __volatile__("wsr %0,ps; isync" :: "a" (state) : "memory")
    #endif

    #define RB_ATOMIC_START do { uint32_t _savedIS = xt_rsil(15) ;
    #define RB_ATOMIC_END xt_wsr_ps(_savedIS) ;} while(0);
#else

    #if __EMSCRIPTEN__
        #define RB_ATOMIC_START
        #define RB_ATOMIC_END
    #else
    #error ("This library only supports AVR and ESP8266 Boards.")
    #endif

#endif


typedef struct rbo_t rbo_t;



#ifdef __cplusplus
extern "C" {
#endif

rbo_t *rbo_t_new(int size, int len);

int rbo_init(rbo_t *self, int size, int len);
int rbo_delete(rbo_t *self);

int rbo_next_end_index(rbo_t *self);
int rbo_incr_end(rbo_t *self);
int rbo_incr_start(rbo_t *self);
int rbo_append(rbo_t *self, const void *object);
void *rbo_peek(rbo_t *self, unsigned int num);
void *rbo_pop(rbo_t *self, void *object);
bool rbo_is_full(rbo_t *self);
bool rbo_is_empty(rbo_t *self);
unsigned int rbo_count(rbo_t *self);

#ifdef __cplusplus
}
#endif


// For those of you who cant live without pretty C++ objects....
#ifdef __cplusplus
class rbo_tC
{

public:
    rbo_tC(int size, int len) { buf = rbo_t_new(size, len); }
    ~rbo_tC() { rbo_delete(buf); }

    bool isFull() { return rbo_is_full(buf); }
    bool isEmpty() { return rbo_is_empty(buf); }
    unsigned int numElements() { return rbo_count(buf); }

    unsigned int add(const void *object) { return rbo_append(buf, object); }
    void *peek(unsigned int num) { return rbo_peek(buf, num); }
    void *pull(void *object) { return rbo_pop(buf, object); }

    // Use this to check if memory allocation failed
    bool allocFailed() { return !buf; }

private:
    rbo_t *buf;
};
#endif


#endif // RINGBUF_O_H








