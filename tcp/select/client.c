
#include <arpa/inet.h>
#include <netinet/tcp.h>  // TCP_NODELAY]
#include <sys/time.h>     // gettimeofday()
#include <string.h>       // memset()
#include <stdio.h>        // printf(), perror()
#include <stdlib.h>       // exit()
#include <time.h>         // time_t, time()

#include "../../util.h"


#define PACKETSIZE 16
#define NODELAY 1
  

int create_socket(const char* ip) {
  struct sockaddr_in me;
  memset(&me, 0, sizeof(me));
  me.sin_family      = AF_INET;
  me.sin_port        = htons(9991);
  me.sin_addr.s_addr = htons(INADDR_ANY);

  int fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd < 0) {
    perror("socket");
    exit(1);
  }

  int on = NODELAY;
  if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) != 0) {
    perror("setsockopt");
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

  if (connect(fd, (struct sockaddr*)&other, sizeof(other)) != 0) {
    perror("connect");
    exit(1);
  }

  return fd;
}


void send_packet(int fd) {
  char buffer[PACKETSIZE];

  gettimeofday((struct timeval*)buffer, 0);

  str_repeat(buffer + sizeof(struct timeval), 't', (unsigned int)sizeof(buffer) - (unsigned int)sizeof(struct timeval));

  if (send(fd, buffer, PACKETSIZE, 0) != PACKETSIZE) {
    perror("send");
    exit(1);
  }
}


int main(int argc, const char* argv[]) {
  int         count = 3;
  const char* ip    = "127.0.0.1";

  if (argc > 1) {
    ip = argv[1];

    if (argc > 2) {
      count = atoi(argv[2]);
    }
  }

  printf("pinging %s using %d sockets\n", ip, count);

  uint32_t       received     = 0;
  uint64_t       microseconds = 0;
  time_t         next         = time(NULL) + 1;
  int            maxfd        = 0;
  fd_set         fds;
  struct timeval tv;

  FD_ZERO(&fds);

  for (int i = 0; i < count; ++i) {
    int fd = create_socket(ip);

    send_packet(fd);

    FD_SET(fd, &fds);

    if (fd > maxfd) {
      maxfd = fd;
    }
  }

  while (1) {
    char   rbuffer[32];
    time_t now = time(NULL);

    if (now > next) {
      printf(
        "%6u per second %10s (%6lu microseconds latency)\n",
        received,
        printsize(rbuffer, sizeof(rbuffer), received * PACKETSIZE),
        (received > 0) ? (microseconds / received) : 0
      );

      ++next;
      
      received     = 0;
      microseconds = 0;
    }


    fd_set rfds;
    memcpy(&rfds, &fds, sizeof(fds));
  
    tv.tv_sec  = 0;
    tv.tv_usec = 1000;

    int ready = select(maxfd + 1, &rfds, 0, 0, &tv);

    if (ready == -1) {
      perror("select");
      exit(1);
    }

    if (ready == 0) {
      // Timeout.
      continue;
    }

    // Don't check stdin, stdout and stderr.
    for (int i = 3; i <= maxfd; ++i) {
      if (FD_ISSET(i, &rfds)) {
        char buffer[PACKETSIZE];
        int  n = recv(i, buffer, sizeof(buffer), MSG_WAITALL);

        if (n != PACKETSIZE) {
          perror("recv");
          exit(1);
        }

        ++received;

        struct timeval  tvnow;
        struct timeval* tvsent = (struct timeval*)buffer;

        gettimeofday(&tvnow, 0);

        microseconds += (tvnow.tv_sec  - tvsent->tv_sec) * 1000000;
        microseconds +=  tvnow.tv_usec - tvsent->tv_usec;

        send_packet(i);
      }
    }
  }
}

