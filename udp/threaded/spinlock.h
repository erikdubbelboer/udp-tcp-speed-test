
#ifndef SRC_SPINLOCK_H_
#define SRC_SPINLOCK_H_


#include <sched.h>


typedef volatile int spinlock_t;


inline void spinlock_init(spinlock_t* spinlock) {
  *spinlock = 0;
}


inline void spinlock_lock(spinlock_t* spinlock, int yield) {
  for (int tries = 0; 1; ++tries) {
    if (__sync_bool_compare_and_swap(spinlock, 0, 1)) {
      return;
    } else if ((tries == 100) && yield) {
      tries = 0;
      sched_yield();
    }
  }
}


inline int spinlock_try_lock(spinlock_t* spinlock) {
  if (__sync_bool_compare_and_swap(spinlock, 0, 1)) {
    return 1;
  }

  return 0;
}


inline int spinlock_locked(spinlock_t* spinlock) {
  return *spinlock;
}


inline void spinlock_unlock(spinlock_t* spinlock) {
  __sync_lock_release(spinlock);
}


#endif // SRC_SPINLOCK_H_

