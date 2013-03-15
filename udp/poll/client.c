
#include <arpa/inet.h>
#include <sys/time.h>  // gettimeofday()
#include <string.h>    // memset()
#include <stdio.h>     // printf(), perror()
#include <stdlib.h>    // exit()
#include <poll.h>      // poll()


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
  int         count = 3;
  const char* ip    = "127.0.0.1";

  if (argc > 1) {
    ip = argv[1];

    if (argc > 2) {
      count = atoi(argv[2]);
    }
  }

  printf("pinging %s using %d sockets\n", ip, count);


  struct sockaddr_in other;

  memset(&other, 0, sizeof(other));

  other.sin_family = AF_INET;
  other.sin_port = htons(9991);

  if (inet_aton(ip, &other.sin_addr) == 0) {
    perror("inet_aton");
    exit(1);
  }


  struct timeval timers[count];
  struct timeval printer;
  struct pollfd  pfds[count];

  memset(pfds  , 0, sizeof(pfds));
  memset(timers, 0, sizeof(timers));


  // Create the sockets.
  for (int i = 0; i < count; ++i) {
    pfds[i].fd     = create_socket();
    pfds[i].events = POLLIN;

    send_packet(pfds[i].fd, &other);
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
        send_packet(pfds[i].fd, &other);

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


    int timeout = (mtv.tv_sec * 1000) + (mtv.tv_usec / 1000);

    int ready = poll(pfds, count, timeout);

    if (ready == -1) {
      perror("poll");
      exit(1);
    }

    if (ready == 0) {
      continue;
    }


    for (int i = 0; i < count; ++i) {
      if (pfds[i].revents & POLLIN) {
        char    buffer[65507];  // Max UDP packet size ((2^16 - 1) - (8 byte UDP header) - (20 byte IP header)).
        ssize_t n = recv(pfds[i].fd, buffer, sizeof(buffer), 0);

        if (n < 0) {
          perror("recv");
          exit(1);
        }

        send_packet(pfds[i].fd, &other);

        timers[i].tv_sec  = tv.tv_sec;
        timers[i].tv_usec = tv.tv_usec + 100000;

        ++sent;
        ++received;
      }
    }
  }
}

