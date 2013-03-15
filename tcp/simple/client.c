
#include <arpa/inet.h>
#include <netinet/tcp.h>  // TCP_NODELAY
#include <string.h>       // memset()
#include <stdio.h>        // printf(), perror()
#include <stdlib.h>       // exit()
#include <time.h>         // time_t, time()

#include "../util.h"


#define PACKETSIZE 4
#define NODELAY


int main(int argc, const char* argv[]) {
  const char* ip = (argc > 1) ? argv[1] : "127.0.0.1";

  printf("pinging %s\n", ip);

  struct sockaddr_in me;
  memset(&me, 0, sizeof(me));
  me.sin_family      = AF_INET;
  me.sin_port        = htons(0);  // First free port.
  me.sin_addr.s_addr = htons(INADDR_ANY);

  int fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd < 0) {
    perror("socket");
    exit(1);
  }

#ifdef NODELAY
  int on = 1;
  if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) != 0) {
    perror("setsockopt");
    exit(1);
  }
#endif

  struct sockaddr_in other;
  memset(&other, 0, sizeof(other));
  other.sin_family = AF_INET;
  other.sin_port = htons(9991);

  if (inet_aton(ip, &other.sin_addr) == 0) {
    perror("inet_aton");
    exit(1);
  }

  if (connect(fd, (struct sockaddr*)&other, sizeof(other)) != 0) {
    perror("connect");
    exit(1);
  }


  uint32_t       received = 0;
  time_t         next = time(NULL) + 1;
  struct timeval tv;

  tv.tv_sec  = 0;
  tv.tv_usec = 1000;

  while (1) {
    char buffer[PACKETSIZE];

    str_repeat(buffer, 't', sizeof(buffer));

    if (send(fd, buffer, PACKETSIZE, 0) != PACKETSIZE) {
      perror("send");
      exit(1);
    }

    while (1) {
      time_t now = time(NULL);

      if (now > next) {
        printf("%6u per second\n", received);

        ++next;

        received = 0;
      }

      fd_set rfds;
      FD_ZERO(&rfds);
      FD_SET(fd, &rfds);

      int ready = select(fd + 1, &rfds, NULL, NULL, &tv);

      if (ready == -1) {
        perror("select");
        exit(1);
      }

      if (ready == 0) {
        // Timeout.
        continue;
      }

      if (FD_ISSET(fd, &rfds)) {
        break;
      }

      // Will we ever reach this?
    }


    ssize_t n = recv(fd, buffer, sizeof(buffer), MSG_WAITALL);

    if (n != PACKETSIZE) {
      perror("recv");
      exit(1);
    }

    ++received;
  }
}

