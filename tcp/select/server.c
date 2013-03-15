
#include <arpa/inet.h>
#include <netinet/tcp.h>  // TCP_NODELAY
#include <fcntl.h>        // fcntl()
#include <unistd.h>       // close()
#include <string.h>       // memset()
#include <stdio.h>        // perror()
#include <stdlib.h>       // exit()
#include <errno.h>        // errno, EWOULDBLOCK


#define PACKETSIZE 4
#define NODELAY


int main() {
  struct sockaddr_in me;
  memset(&me, 0, sizeof(me));
  me.sin_family      = AF_INET;
  me.sin_port        = htons(9991);
  me.sin_addr.s_addr = htons(INADDR_ANY);

  /*if (inet_aton("127.0.0.1", &me.sin_addr) == 0) {
    perror("inet_aton");
    exit(1);
  }*/

  int fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd < 0) {
    perror("socket");
    exit(1);
  }

  // Set SO_REUSEADDR so we can quicly restart the server without the socket still being in use.
  int on = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0) {
    perror("setsockopt");
    exit(1);
  }

#ifdef NODELAY
  on = 1;
  if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) != 0) {
    perror("setsockopt");
    exit(1);
  }
#endif

  int flags = fcntl(fd, F_GETFL, 0);

  if (flags == -1) {
    perror("fcntl");
    exit(1);
  }

  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
    perror("fcntl");
    exit(1);
  }

  if (bind(fd, (struct sockaddr*)&me, sizeof(me)) != 0) {
    perror("bind");
    exit(1);
  }

  if (listen(fd, 127) != 0) {
    perror("listen");
    exit(1);
  }


  fd_set         fds;
  int            maxfd = fd;
  struct timeval tv;

  FD_ZERO(&fds);
  FD_SET(fd, &fds);

  tv.tv_sec  = 1;
  tv.tv_usec = 0;

  while (1) {
    fd_set rfds;

    memcpy(&rfds, &fds, sizeof(fds));

    int ready = select(maxfd + 1, &rfds, 0, 0, &tv);

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
        if (i == fd) { // Is this our listening socket?
          while (1) {
            int nfd = accept(fd, 0, 0);

            if (nfd == -1) {
              if (errno != EWOULDBLOCK) {
                perror("accept");
                exit(1);
              }

              break;
            }

            FD_SET(nfd, &fds);

            if (nfd > maxfd) {
              maxfd = nfd;
            }
          }
        } else {
          char buffer[PACKETSIZE];
          int  n = recv(i, buffer, sizeof(buffer), 0);

          if (n == -1) {
            if (errno == ECONNRESET) {
              // Close the socket.
              n = 0;
            } else {
              perror("recv");
              exit(1);
            }
          }

          if (n > 0) {
            // Respond.
            int s = send(i, buffer, n, 0);

            if (s != n) {
              if (errno == ECONNRESET) {
                // Close the socket.
                n = 0;
              } else {
                perror("send");
                exit(1);
              }
            }
          }

          // Does this socket need to be closed?
          if (n == 0) {
            close(i);

            FD_CLR(i, &fds);

            if (i == maxfd) {
              while (!FD_ISSET(maxfd, &fds)) {
                --maxfd;
              }
            }
          }
        }
      }
    }
  }
}

