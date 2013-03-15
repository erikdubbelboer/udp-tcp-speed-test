
#include <arpa/inet.h>
#include <sys/time.h>  // gettimeofday()
#include <string.h>    // memset()
#include <stdio.h>     // printf(), perror()
#include <stdlib.h>    // exit()

#include "../../util.h"


#define PACKETSIZE 16
#define TIMEOUT    10000  // Timeout in microseconds.
#define USE_SELECT
//#define USE_POLL
//#define USE_EPOLL


#ifdef USE_POLL
#include <poll.h>
#elif defined USE_EPOLL
#include <sys/epoll.h>
#endif


int create_socket() {
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
  
  if (bind(fd, (struct sockaddr*)&me, sizeof(me)) != 0) {
    perror("bind");
    exit(1);
  }

  return fd;
}


void send_packet(int fd, struct sockaddr_in* other) {
  char buffer[PACKETSIZE];

  gettimeofday((struct timeval*)buffer, 0);

  str_repeat(buffer + sizeof(struct timeval), 't', (unsigned int)sizeof(buffer) - (unsigned int)sizeof(struct timeval));

  if (sendto(fd, buffer, PACKETSIZE, 0, (struct sockaddr*)other, sizeof(*other)) != PACKETSIZE) {
    perror("sendto");
    exit(1);
  }
}


int main(int argc, char* argv[]) {
  int         count = 3;
  const char* ip    = "127.0.0.1";

  if (argc > 1) {
    ip = argv[1];

    if (argc > 2) {
      count = atoi(argv[2]);
    }
  }

  printf(
    "pinging %s using %d sockets (%s)\n",
    ip,
    count,
#ifdef USE_SELECT
    "select"
#elif defined USE_POLL
    "poll"
#elif defined USE_EPOLL
    "epoll"
#endif
  );

  struct sockaddr_in other;
  memset(&other, 0, sizeof(other));
  other.sin_family = AF_INET;
  other.sin_port   = htons(9991);

  if (inet_aton(ip, &other.sin_addr) == 0) {
    perror("inet_aton");
    exit(1);
  }

  int      fds[count];
  uint64_t timers[count];
  uint64_t printer;
#ifdef USE_SELECT
  fd_set   rfds;
  int      maxfd = -1;
  
  FD_ZERO(&rfds);
#elif defined USE_POLL
  struct pollfd  pfds[count];
#elif defined USE_EPOLL
  int epfd = epoll_create(count);
#endif

  // Create the sockets.
  for (int i = 0; i < count; ++i) {
    fds[i]    = create_socket();
    timers[i] = ustime() + TIMEOUT;

#ifdef USE_SELECT
    FD_SET(fds[i], &rfds);

    if (fds[i] > maxfd) {
      maxfd = fds[i];
    }
#elif defined USE_POLL
    pfds[i].fd     = fds[i];
    pfds[i].events = POLLIN;
#elif defined USE_EPOLL
    struct epoll_event ee;

    ee.events  = EPOLLIN;
    ee.data.fd = fds[i];

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, ee.data.fd, &ee) == -1) {
      perror("epoll_ctl");
      exit(1);
    }
#endif

    send_packet(fds[i], &other);
  }


  printer = ustime() + 1000000;


  uint32_t sent         = 0;
  uint32_t received     = 0;
  uint64_t microseconds = 0;


  for (;;) {
    uint64_t now = ustime();
    uint64_t min = now + 1000000;

    // Check for timeouts.
    for (int i = 0; i < count; ++i) {
      // If the timer has expired.
      if (timers[i] < now) {
        send_packet(fds[i], &other);

        timers[i] = now + TIMEOUT;

        ++sent;
      }

      // Is this the lowest timer?
      if (timers[i] < min) {
        min = timers[i];
      }
    }


    if (printer < now) {
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

      sent         = 0;
      received     = 0;
      microseconds = 0;

      printer += 1000000;
    }

    // Is the printer timer the lowest timer?
    if (printer < min) {
      min = printer;
    }


    // Make it an interval, not an exact time.
    min -= now;


#ifdef USE_SELECT
    struct timeval tv;
    fd_set         tfds;

    memcpy(&tfds, &rfds, sizeof(fd_set));
    
    tv.tv_sec  = min / 1000000;
    tv.tv_usec = min % 1000000;

    int ready = select(maxfd + 1, &tfds, NULL, NULL, &tv);
#elif defined USE_POLL
    int ready   = poll(pfds, count, min / 1000);
#elif defined USE_EPOLL
    struct epoll_event events[count];

    int ready = epoll_wait(epfd, events, count, min / 1000);
#endif

    if (ready == -1) {
#ifdef USE_SELECT
      perror("select");
#elif defined USE_POLL
      perror("poll");
#elif defined USE_EPOLL
      perror("epoll_wait");
#endif

      exit(1);
    }

    if (ready == 0) {
      // Timeout.
      continue;
    }

    struct timeval  tvnow;
    gettimeofday(&tvnow, 0);

    for (int i = 0; i < count; ++i) {
#ifdef USE_SELECT
      if (FD_ISSET(fds[i], &tfds)) {
#elif defined USE_POLL
      if (pfds[i].revents & POLLIN) {
#elif defined USE_EPOLL
      int hasdata = 0;

      for (int j = 0; j < ready; ++j) {
        if ((events[j].events & EPOLLIN) &&
            (events[i].data.fd = fds[i])) {
          hasdata = 1;
          break;
        }
      }

      if (hasdata) {
#endif
        char    buffer[PACKETSIZE];
        ssize_t n = recv(fds[i], buffer, sizeof(buffer), 0);

        if (n != PACKETSIZE) {
          perror("recv");
          exit(1);
        }

        send_packet(fds[i], &other);

        timers[i] = now + TIMEOUT;

        struct timeval* tvsent = (struct timeval*)buffer;

        microseconds += (tvnow.tv_sec  - tvsent->tv_sec) * 1000000;
        microseconds +=  tvnow.tv_usec - tvsent->tv_usec;

        ++sent;
        ++received;
      }
    }
  }
}

