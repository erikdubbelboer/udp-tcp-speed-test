
#include <arpa/inet.h>
#include <sys/time.h>  // gettimeofday()
#include <string.h>    // memset()
#include <stdio.h>     // printf(), perror()
#include <stdlib.h>    // exit()
#include <errno.h>     // EAGAIN
#include <time.h>      // time_t, time()
#include <unistd.h>    // sleep()
#include <pthread.h>

#include "spinlock.h"
#include "../../util.h"


#define PACKETSIZE 16


static spinlock_t  lock;
static const char* ip           = "127.0.0.1";
static uint32_t    sent         = 0;
static uint32_t    received     = 0;
static uint64_t    microseconds = 0;
static int         fd;


void* printer(void* arg) {
  for (;;) {
    sleep(1);
    
    spinlock_lock(&lock);

    uint32_t s = sent;
    uint32_t r = received;
    uint64_t m = microseconds;
   
    sent         = 0; 
    received     = 0;
    microseconds = 0;

    spinlock_unlock(&lock);

    if (s < r) {
      s = r;
    }

    char rbuffer[32];
    char mbuffer[32];
    char sbuffer[32];

    printf(
      "%6u per second %10s (%6u missing %10s) (%6u sent %10s) (%6lu microseconds latency)\n",
      r,
      printsize(rbuffer, sizeof(rbuffer), r * PACKETSIZE),
      s - r,
      printsize(mbuffer, sizeof(mbuffer), (s - r) * PACKETSIZE),
      s,
      printsize(sbuffer, sizeof(sbuffer), s * PACKETSIZE),
      (r > 0) ? (m / r) : 0
    );
  }

  return 0;
}


void* sender(void* arg) {
  struct sockaddr_in other;
  memset(&other, 0, sizeof(other));
  other.sin_family = AF_INET;
  other.sin_port   = htons(9991);

  if (inet_aton(ip, &other.sin_addr) == 0) {
    perror("inet_aton");
    exit(1);
  }

  for (;;) {
    char buffer[PACKETSIZE];

    gettimeofday((struct timeval*)buffer, 0);

    str_repeat(buffer + sizeof(struct timeval), 't', (unsigned int)sizeof(buffer) - (unsigned int)sizeof(struct timeval));

    if (sendto(fd, buffer, PACKETSIZE, 0, (struct sockaddr*)&other, sizeof(other)) != PACKETSIZE) {
      perror("sendto");
      exit(1);
    }

    spinlock_lock(&lock);
    ++sent;
    spinlock_unlock(&lock);
  }

  return 0;
}


int main(int argc, char* argv[]) {
  if (argc > 1) {
    ip = argv[1];
  }

  printf("pinging %s\n", ip);

  spinlock_init(&lock);

  pthread_t printert;

  pthread_create(&printert, 0, printer, NULL);

  struct sockaddr_in me;
  memset(&me, 0, sizeof(me));
  me.sin_family      = AF_INET;
  me.sin_port        = htons(0);  // First free port.
  me.sin_addr.s_addr = htonl(INADDR_ANY);

  fd = socket(AF_INET, SOCK_DGRAM, 0);

  if (fd < 0) {
    perror("socket");
    exit(1);
  }  
  
  if (bind(fd, (struct sockaddr*)&me, sizeof(me)) != 0) {
    perror("bind");
    exit(1);
  }

  pthread_t sender_thread;

  pthread_create(&sender_thread, 0, sender, NULL);

  for (;;) {
    char buffer[PACKETSIZE];
    
    if (recv(fd, buffer, sizeof(buffer), 0) != PACKETSIZE) {
      perror("recv");
      exit(1);
    }

    struct timeval  tvnow;
    struct timeval* tvsent = (struct timeval*)buffer;

    gettimeofday(&tvnow, 0);

    spinlock_lock(&lock);
    ++received;
    microseconds += (tvnow.tv_sec  - tvsent->tv_sec) * 1000000;
    microseconds +=  tvnow.tv_usec - tvsent->tv_usec;
    spinlock_unlock(&lock);
  }

  return 0;
}

