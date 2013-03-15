
#include <arpa/inet.h>
#include <netinet/tcp.h>  // TCP_NODELAY
#include <fcntl.h>        // fcntl()
#include <unistd.h>       // close()
#include <string.h>       // memset()
#include <stdio.h>        // perror()
#include <stdlib.h>       // exit()

 
int main() {
  struct sockaddr_in me;
  memset(&me, 0, sizeof(me));
  me.sin_family      = AF_INET;
  me.sin_port        = htons(9991);
  me.sin_addr.s_addr = htons(INADDR_ANY);

  int lfd = socket(AF_INET, SOCK_STREAM, 0);

  if (lfd < 0) {
    perror("socket");
    exit(1);
  }

  // Set SO_REUSEADDR so we can quicly restart the server without the socket still being in use.
  int on = 1;
  if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0) {
    perror("setsockopt");
    exit(1);
  }

  on = 1;
  if (setsockopt(lfd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) != 0) {
    perror("setsockopt");
    exit(1);
  }

  int flags = fcntl(lfd, F_GETFL, 0);

  if (flags == -1) {
    perror("fcntl");
    exit(1);
  }

  if (fcntl(lfd, F_SETFL, flags | O_NONBLOCK) != 0) {
    perror("fcntl");
    exit(1);
  }

  if (bind(lfd, (struct sockaddr*)&me, sizeof(me)) != 0) {
    perror("bind");
    exit(1);
  }

  if (listen(lfd, 127) != 0) {
    perror("listen");
    exit(1);
  }
 
  //sleep(1);
  
  int cfd = socket(AF_INET, SOCK_STREAM, 0);

  if (cfd < 0) {
    perror("socket");
    exit(1);
  }
   
  if (connect(cfd, (struct sockaddr*)&me, sizeof(me)) != 0) {
    perror("connect");
    exit(1);
  }
   
  //sleep(1);
   
  int sfd = accept(lfd, 0, 0);

  if (sfd < 0) {
    perror("accept");
    exit(1);
  }

  socklen_t len;
  if (getsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, &on, &len) != 0) {
    perror("getsockopt");
    exit(1);
  }

  printf("TCP_NODELAY on inherited socket is %s.\n", on ? "enabled" : "disabled");
   
  flags = fcntl(sfd, F_GETFL, 0);

  printf("O_NONBLOCK on inherited socket is %s.\n", (flags & O_NONBLOCK) ? "enabled" : "disabled");

  close(sfd);
  close(cfd);
  close(lfd);
   
  return 0;
}

