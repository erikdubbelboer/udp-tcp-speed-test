
#include <sys/time.h>  // gettimeofday()
#include <stdio.h>     // snprintf()
#include <stdlib.h>    // rand()
#include <math.h>      // log(), pow()

#include "util.h"


char* printsize(char* buffer, unsigned int length, uint64_t size) {
  // floor(log(pow(2, 64)) / log(1024)) = 6
  const char* unit[] = {
    "B", "KB", "MB", "GB", "TB", "PB", "EB"
  };
   
  if (size == 0) {
    snprintf(buffer, length, "0 %s", unit[0]);
  } else {
    unsigned int base = floor(log(size) / log(1024));
     
    snprintf(buffer, length, "%.2f %s", size / pow(1024, base), unit[base]);
  }
   
  return buffer;
}


char* str_repeat(char* buffer, char c, unsigned int count) {
  char* p = buffer;

  for (unsigned int i = 0; i < count; ++i) {
    *p++ = c;
  }

  return buffer;
}


uint32_t rand32() {
  uint32_t r = 0;

  for (int i = 0; i < 4; ++i) {
    r |= (rand() & 0xFF) << (i * 8);
  }

  return r;
}

uint64_t rand64() {
  uint64_t r = 0;

  for (int i = 0; i < 8; ++i) {
    r |= (rand() & 0xFF) << (i * 8);
  }

  return r;
}


int64_t ustime() {
  struct timeval tv;
  int64_t        ust;

  gettimeofday(&tv, NULL);

  ust = ((int64_t)tv.tv_sec) * 1000000;
  ust += tv.tv_usec;

  return ust;
}

