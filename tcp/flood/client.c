
#include <arpa/inet.h>
#include <netinet/tcp.h>  // TCP_NODELAY
#include <fcntl.h>        // fcntl()
#include <sys/time.h>     // gettimeofday()
#include <string.h>       // memset()
#include <stdio.h>        // perror()
#include <stdlib.h>       // exit()
#include <time.h>         // time_t, time()
#include <errno.h>        // errno, EWOULDBLOCK

#include "../../util.h"


#define PACKETSIZE 16
#define NODELAY 1
#define NONBLOCK


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

#ifdef NONBLOCK
  int flags = fcntl(fd, F_GETFL, 0);

  if (flags == -1) {
    perror("fcntl");
    exit(1);
  }

  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
    perror("fcntl");
    exit(1);
  }
#endif

  struct sockaddr_in other;
  memset(&other, 0, sizeof(other));
  other.sin_family = AF_INET;
  other.sin_port   = htons(9991);

  if (inet_aton(ip, &other.sin_addr) == 0) {
    perror("inet_aton");
    exit(1);
  }

  int n = connect(fd, (struct sockaddr*)&other, sizeof(other));

  if (n != 0) {
    if (errno != EINPROGRESS) { // Non-blocking tcp sockets return EINPROGRESS on connect().
      perror("connect");
      exit(1);
    }

    errno = 0;  // Reset errno so calls to perror() are correct when errno isn't set.
  }

  return fd;
}


int main(int argc, const char* argv[]) {
  if (PACKETSIZE < sizeof(struct timeval)) {
    printf("PACKETSIZE should be at least %ld\n", sizeof(struct timeval));
    exit(1);
  }

  int         count = 3;
  const char* ip    = "127.0.0.1";

  if (argc > 1) {
    ip = argv[1];

    if (argc > 2) {
      count = atoi(argv[2]);
    }
  }

  printf("pinging %s using %d sockets\n", ip, count);

  uint32_t       sent         = 0;
  uint32_t       received     = 0;
  uint64_t       microseconds = 0;
  time_t         next         = time(NULL) + 1;
  int            maxfd        = 0;
  fd_set         fds;
  struct timeval tv;

  FD_ZERO(&fds);

  for (int i = 0; i < count; ++i) {
    int fd = create_socket(ip);

    FD_SET(fd, &fds);

    if (fd > maxfd) {
      maxfd = fd;
    }
  }

  while (1) {
    time_t now = time(NULL);

    if (now > next) {
      char rbuffer[32];
      char sbuffer[32];

      if (sent < received) {
        sent = received;
      }

      printf(
        "%6u per second %10s (%6u missing %10s) (%6lu microsecond latency, %6lu miliseconds)\n",
        received,
        printsize(rbuffer, sizeof(rbuffer), received * PACKETSIZE),
        sent - received,
        printsize(sbuffer, sizeof(sbuffer), (sent - received) / PACKETSIZE),
        (received > 0) ? (microseconds / received) : 0,
        (received > 0) ? ((microseconds / received) / 1000) : 0
      );

      ++next;
      
      sent         = 0;
      received     = 0;
      microseconds = 0;
    }


    fd_set rfds, wfds;

    memcpy(&rfds, &fds, sizeof(fds));
    memcpy(&wfds, &fds, sizeof(fds));
   
    tv.tv_sec  = 0;
    tv.tv_usec = 1000;

    int ready = select(maxfd + 1, &rfds, &wfds, 0, &tv);

    if (ready == -1) {
      perror("select");
      exit(1);
    }

    if (ready == 0) {
      // Timeout.
      continue;
    }

    // Skip stdin, stdout and stderr.
    for (int i = 3; i <= maxfd; ++i) {
      if (FD_ISSET(i, &rfds)) {
        char buffer[PACKETSIZE];
        int  n = recv(i, buffer, sizeof(buffer), MSG_WAITALL);

        if (n != PACKETSIZE) {
          if (errno != EWOULDBLOCK) {
            perror("recv");
            exit(1);
          }
        } else {
          struct timeval  tvnow;
          struct timeval* tvsent = (struct timeval*)buffer;

          gettimeofday(&tvnow, 0);

          microseconds += (tvnow.tv_sec  - tvsent->tv_sec) * 1000000;
          microseconds +=  tvnow.tv_usec - tvsent->tv_usec;

          ++received;
        }
      }
      if (FD_ISSET(i, &wfds)) {
        char buffer[PACKETSIZE];

        gettimeofday((struct timeval*)buffer, 0);

        str_repeat(buffer + sizeof(struct timeval), 't', (unsigned int)sizeof(buffer) - (unsigned int)sizeof(struct timeval));

        int n = send(i, buffer, PACKETSIZE, 0);
       
        if (n != PACKETSIZE) {
          if (errno != EWOULDBLOCK) {
            perror("send");
            exit(1);
          }
        } else {
          ++sent;
        }
      }
    }
  }
}

