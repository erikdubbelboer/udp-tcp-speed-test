
#include <arpa/inet.h>
#include <sys/time.h>  // gettimeofday()
#include <string.h>    // memset()
#include <stdio.h>     // printf(), perror()
#include <stdlib.h>    // exit()


int create_socket() {
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
  
  if (bind(fd, (struct sockaddr*)&me, sizeof(me)) != 0) {
    perror("bind");
    exit(1);
  }

  return fd;
}


void send_packet(int fd, struct sockaddr_in* other) {
  char buffer[65507];  // Max UDP packet size ((2^16 - 1) - (8 byte UDP header) - (20 byte IP header)).

  int s = snprintf(buffer, sizeof(buffer), "test");

  if (s > sizeof(buffer)) {
    s = sizeof(buffer);
  }

  if (sendto(fd, buffer, s, 0, (struct sockaddr*)other, sizeof(*other)) != s) {
    perror("sendto");
    exit(1);
  }
}


int main(int argc, char* argv[]) {
  int count = 3;

  if (argc > 1) {
    count = atoi(argv[1]);
  }


  printf("using %d sockets\n", count);


  struct sockaddr_in other;

  memset(&other, 0, sizeof(other));

  other.sin_family = AF_INET;
  other.sin_port = htons(9991);

  if (inet_aton("127.0.0.1", &other.sin_addr) == 0) {
    perror("inet_aton");
    exit(1);
  }


  int            fds[count];
  struct timeval timers[count];
  struct timeval printer;
  fd_set         rfds;
  int            maxfd = -1;

  FD_ZERO(&rfds);

  memset(&timers, 0, sizeof(timers));


  // Create the sockets.
  for (int i = 0; i < count; ++i) {
    fds[i] = create_socket();

    FD_SET(fds[i], &rfds);

    if (fds[i] > maxfd) {
      maxfd = fds[i];
    }

    send_packet(fds[i], &other);
  }


  if (gettimeofday(&printer, 0) == -1) {
    perror("gettimeofday");
    exit(1);
  }

  printer.tv_sec += 1;


  uint32_t sent     = 0;
  uint32_t received = 0;


  for (;;) {
    struct timeval tv;   // The current time.
    struct timeval mtv;  // The lowest timeout.

    if (gettimeofday(&tv, 0) == -1) {
      perror("gettimeofday");
      exit(1);
    }

    mtv = tv;

    // Check for timeouts.
    for (int i = 0; i < count; ++i) {
      // If the timer has expired.
      if ((timers[i].tv_sec < tv.tv_sec) ||
          ((timers[i].tv_sec == tv.tv_sec) && (timers[i].tv_usec < tv.tv_usec))) {
        send_packet(fds[i], &other);

        timers[i].tv_sec  = tv.tv_sec;
        timers[i].tv_usec = tv.tv_usec + 100000;

        ++sent;
      }

      // Is this the lowest timer?
      if ((timers[i].tv_sec < mtv.tv_sec) ||
          ((timers[i].tv_sec == mtv.tv_sec) && (timers[i].tv_usec < mtv.tv_usec))) {
        mtv = timers[i];
      }
    }


    if ((printer.tv_sec < tv.tv_sec) ||
        ((printer.tv_sec == tv.tv_sec) && (printer.tv_usec < tv.tv_usec))) {
      printf("%6u per second (%6u missing)\n", received, (sent - received));

      sent     = 0;
      received = 0;

      printer.tv_sec += 1;
    }

    // Is the printer timer the lowest timer?
    if ((printer.tv_sec < mtv.tv_sec) ||
        ((printer.tv_sec == mtv.tv_sec) && (printer.tv_usec < mtv.tv_usec))) {
      mtv = printer;
    }


    // Make it an interval, not an exact time.
    mtv.tv_sec  -= tv.tv_sec;
    mtv.tv_usec -= tv.tv_usec;


    fd_set tfds;

    memcpy(&tfds, &rfds, sizeof(fd_set));

    int ready = select(maxfd + 1, &tfds, NULL, NULL, &mtv);

    if (ready == -1) {
      perror("select");
      exit(1);
    }

    if (ready == 0) {
      continue;
    }


    for (int i = 0; i < count; ++i) {
      if (FD_ISSET(fds[i], &tfds)) {
        char    buffer[65507];  // Max UDP packet size ((2^16 - 1) - (8 byte UDP header) - (20 byte IP header)).
        ssize_t n = recv(fds[i], buffer, sizeof(buffer), 0);

        if (n < 0) {
          perror("recv");
          exit(1);
        }

        send_packet(fds[i], &other);

        timers[i].tv_sec  = tv.tv_sec;
        timers[i].tv_usec = tv.tv_usec + 100000;

        ++sent;
        ++received;
      }
    }
  }
}

