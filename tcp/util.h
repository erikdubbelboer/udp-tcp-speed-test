
#ifndef SRC_UTIL_H_
#define SRC_UTIL_H_


#include <stdint.h>  // uint64_t


#ifdef __cplusplus
extern "C" {
#endif


char* printsize(char* buffer, unsigned int length, uint64_t size);
char* str_repeat(char* buffer, char c, unsigned int count);

uint32_t rand32();
uint64_t rand64();


inline uint8_t _rotr8(uint8_t b, uint8_t n) {
  uint8_t r;
  asm ("rorb %b[n], %b[r]" : [r] "=rm" (r) : "[r]" (b), [n] "Nc" (n));
  return r;
}


#define breakpoint() __asm__("int $3")


#ifdef __cplusplus
}
#endif


#endif // __SRC_UTIL_H_

