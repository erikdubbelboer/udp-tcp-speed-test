
#include <arpa/inet.h>
#include <string.h>  // memset()
#include <stdio.h>   // printf(), perror()
#include <stdlib.h>  // exit()
#include <errno.h>   // EAGAIN
#include <time.h>    // time_t, time()


#undef USE_SELECT


int main() {
  struct sockaddr_in me;

  memset(&me, 0, sizeof(me));
  
  me.sin_family = AF_INET;
  me.sin_port   = htons(0);  // First free port.

  if (inet_aton("127.0.0.1", &me.sin_addr) == 0) {
    perror("inet_aton");
    exit(1);
  }


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
  other.sin_port = htons(9991);

  if (inet_aton("127.0.0.1", &other.sin_addr) == 0) {
    perror("inet_aton");
    exit(1);
  }


  time_t next = time(NULL) + 1;

  uint32_t sent     = 0;
  uint32_t received = 0;

  for (;;) {
    time_t now = time(NULL);

    if (now > next) {
      printf("%6u per second (%6u missing)\n", received, (sent - received));

      ++next;

      sent     = 0;
      received = 0;
    }


    char buffer[65507];  // Max UDP packet size ((2^16 - 1) - (8 byte UDP header) - (20 byte IP header))

    int s = snprintf(buffer, sizeof(buffer), "test");

    if (s > sizeof(buffer)) {
      s = sizeof(buffer);
    }


    if (sendto(fd, buffer, s, 0, (struct sockaddr*)&other, sizeof(other)) != s) {
      perror("sendto");
      exit(1);
    }

    ++sent;


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

    ++received;
  }
}

