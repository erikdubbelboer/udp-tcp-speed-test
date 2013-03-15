
#include <arpa/inet.h>
#include <sys/time.h>  // gettimeofday()
#include <string.h>    // memset()
#include <stdio.h>     // printf(), perror()
#include <stdlib.h>    // exit()
#include <errno.h>     // EAGAIN
#include <time.h>      // time_t, time()

#include "../../util.h"


#define PACKETSIZE 16


int main(int argc, const char* argv[]) {
  const char* ip = (argc > 1) ? argv[1] : "127.0.0.1";

  printf("pinging %s\n", ip);

  struct sockaddr_in me;
  memset(&me, 0, sizeof(me));

  me.sin_family      = AF_INET;
  me.sin_port        = htons(0);  // First free port.
  me.sin_addr.s_addr = htons(INADDR_ANY);

  int fd = socket(AF_INET, SOCK_DGRAM, 0);

  if (fd < 0) {
    perror("socket");
    exit(1);
  }  
  
  struct timeval tv = {0, 100000};  // 0.1 seconds.

  if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))) {
    perror("setsockopt");
    exit(1);
  }

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


  time_t next = time(NULL) + 1;

  uint32_t sent         = 0;
  uint32_t received     = 0;
  uint64_t microseconds = 0;

  for (;;) {
    time_t now = time(NULL);

    if (now > next) {
      char rbuffer[32];
      char mbuffer[32];
      char sbuffer[32];

      printf(
        "%6u per second %10s (%6u missing %10s) (%6u sent %10s) (%6lu microseconds latency)\n",
        received,
        printsize(rbuffer, sizeof(rbuffer), received * PACKETSIZE),
        sent - received,
        printsize(mbuffer, sizeof(mbuffer), (sent - received) * PACKETSIZE),
        sent,
        printsize(sbuffer, sizeof(sbuffer), sent * PACKETSIZE),
        (received > 0) ? (microseconds / received) : 0
      );

      ++next;

      sent         = 0;
      received     = 0;
      microseconds = 0;
    }


    char buffer[PACKETSIZE];

    gettimeofday((struct timeval*)buffer, 0);

    str_repeat(buffer + sizeof(struct timeval), 't', (unsigned int)sizeof(buffer) - (unsigned int)sizeof(struct timeval));

    if (sendto(fd, buffer, PACKETSIZE, 0, (struct sockaddr*)&other, sizeof(other)) != PACKETSIZE) {
      perror("sendto");
      exit(1);
    }

    ++sent;

    ssize_t n = recv(fd, buffer, sizeof(buffer), 0);

    if ((n == -1) && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
      // Timeout.
      continue;
    }

    if (n != PACKETSIZE) {
      perror("recv");
      exit(1);
    }

    struct timeval  tvnow;
    struct timeval* tvsent = (struct timeval*)buffer;

    gettimeofday(&tvnow, 0);

    microseconds += (tvnow.tv_sec  - tvsent->tv_sec) * 1000000;
    microseconds +=  tvnow.tv_usec - tvsent->tv_usec;

    ++received;
  }
}

