
#include <arpa/inet.h>
#include <string.h>   // memset()
#include <stdio.h>    // printf(), perror()
#include <stdlib.h>   // exit()
#include <errno.h>    // EAGAIN
#include <time.h>     // time_t, time()
#include <unistd.h>   // sleep()
#include <pthread.h>


#define USE_SPINLOCK
#define USE_SELECT


#ifdef USE_SPINLOCK
#include "spinlock.h"

spinlock_t lock;
#endif

  
static const char* ip = "127.0.0.1";
static uint32_t    sent;
static uint32_t    received;


void* printer(void* arg) {
  for (;;) {
    sleep(1);

#ifdef USE_SPINLOCK
    spinlock_lock(&lock, 0);
#endif

    printf("%6u per second (%6u missing)\n", received, (sent - received));

    sent     -= received;
    received = 0;

#ifdef USE_SPINLOCK
    spinlock_unlock(&lock);
#endif
  }

  return NULL;
}


void* worker(void* arg) {
  struct sockaddr_in me;

  memset(&me, 0, sizeof(me));
  
  me.sin_family      = AF_INET;
  me.sin_port        = htons(0);  // First free port.
  me.sin_addr.s_addr = htonl(INADDR_ANY);


  int fd = socket(PF_INET, SOCK_DGRAM, 0);

  if (fd < 0) {
    perror("socket");
    exit(1);
  }
  
  
#ifndef USE_SELECT
  struct timeval tv = {0, 100000};  // 0.1 seconds.

  if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))) {
    perror("setsockopt");
    exit(1);
  }
#endif
  

  if (bind(fd, (struct sockaddr*)&me, sizeof(me)) != 0) {
    perror("bind");
    exit(1);
  }


  struct sockaddr_in other;

  memset(&other, 0, sizeof(other));

  other.sin_family = AF_INET;
  other.sin_port   = htons(9991);

  if (inet_aton(ip, &other.sin_addr) == 0) {
    perror("inet_aton");
    exit(1);
  }


  for (;;) {
    char buffer[65507];  // Max UDP packet size ((2^16 - 1) - (8 byte UDP header) - (20 byte IP header)).

    int s = snprintf(buffer, sizeof(buffer), "test");

    if (s > sizeof(buffer)) {
      s = sizeof(buffer);
    }


    if (sendto(fd, buffer, s, 0, (struct sockaddr*)&other, sizeof(other)) != s) {
      perror("sendto");
      exit(1);
    }

#ifdef USE_SPINLOCK
    spinlock_lock(&lock, 0);
#endif
    ++sent;
#ifdef USE_SPINLOCK
    spinlock_unlock(&lock);
#endif


#ifdef USE_SELECT
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = 100000;

    int ready = select(fd + 1, &fds, NULL, NULL, &tv);

    if (ready == -1) {
      perror("select");
      exit(1);
    }

    if (ready == 0) {
      continue;
    }
#endif


    ssize_t n = recv(fd, buffer, sizeof(buffer), 0);

    if ((n == -1) && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
      continue;
    }

    if (n != s) {
      perror("recv");
      exit(1);
    }

#ifdef USE_SPINLOCK
    spinlock_lock(&lock, 0);
#endif
    ++received;
#ifdef USE_SPINLOCK
    spinlock_unlock(&lock);
#endif
  }

  return NULL;
}


int main(int argc, char* argv[]) {
  int count = 3;

  if (argc > 1) {
    ip = argv[1];

    if (argc > 2) {
      count = atoi(argv[2]);
    }
  }


  printf("pinging %s using %d threads\n", ip, count);


#ifdef USE_SPINLOCK
  spinlock_init(&lock);
#endif


  pthread_t printert;

  pthread_create(&printert, 0, printer, NULL);


  pthread_t threads[count];

  for (int i = 0; i < count; ++i) {
    pthread_create(&threads[i], 0, worker, NULL);
  }

  for (int i = 0; i < count; ++i) {
    pthread_join(threads[i], NULL);
  }
}

