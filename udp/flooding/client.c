
#include <arpa/inet.h>
#include <string.h>   // memset()
#include <stdio.h>    // printf(), perror()
#include <stdlib.h>   // exit()
#include <errno.h>    // EAGAIN
#include <time.h>     // time_t, time()
#include <unistd.h>   // sleep()
#include <pthread.h>


#include "spinlock.h"

spinlock_t lock;

  
static const char* ip = "127.0.0.1";
static uint32_t    sent;
static uint32_t    received;
static int         fd;


void* printer(void* arg) {
  for (;;) {
    sleep(1);
    
    spinlock_lock(&lock);

    uint32_t s = sent;
    uint32_t r = received;
    
    //sent     -= received;
    sent     = 0;
    received = 0;
    
    spinlock_unlock(&lock);

    printf("%6u per second (%6u missing) %6u sent\n", r, (s - r), s);
  }

  return NULL;
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
    char buffer[65507];  // Max UDP packet size ((2^16 - 1) - (8 byte UDP header) - (20 byte IP header)).

    int s = snprintf(buffer, sizeof(buffer), "test");

    if (s > sizeof(buffer)) {
      s = sizeof(buffer);
    }


    if (sendto(fd, buffer, s, 0, (struct sockaddr*)&other, sizeof(other)) != s) {
      perror("sendto");
      exit(1);
    }

    spinlock_lock(&lock);
    ++sent;
    spinlock_unlock(&lock);
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
    char buffer[65507];
    
    if (recv(fd, buffer, sizeof(buffer), 0) == -1) {
      perror("recv");
      exit(1);
    }

    spinlock_lock(&lock);
    ++received;
    spinlock_unlock(&lock);
  }

  return 0;
}

