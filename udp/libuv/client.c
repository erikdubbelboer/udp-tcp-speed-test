
// For more info see: http://nikhilm.github.com/uvbook/networking.html#udp

#include <sys/time.h>  // gettimeofday()
#include <stdio.h>     // printf()
#include <stdlib.h>    // exit()

#include "uv.h"

#include "../../util.h"


#define PACKETSIZE 16


#define CHECK(x) \
  do { \
    if (x != 0) { \
      printf("%s\n", uv_strerror(uv_last_error(loop))); \
      exit(1); \
    } \
  } while (0)


typedef struct socket_s {
  char       buffer[PACKETSIZE];
  uv_udp_t   udp;
  uv_timer_t timer;
} socket_t;


static const char* ip           = "127.0.0.1";
static int         count        = 3;
static uint32_t    sent         = 0;
static uint32_t    received     = 0;
uint64_t           microseconds = 0;
static uv_loop_t*  loop;
static socket_t*   sockets;


static void timeout_cb(uv_timer_t* handle, int status);


static void send_cb(uv_udp_send_t* req, int status) {
  CHECK(status);

  free(req);
}


static void send_packet(socket_t* socket) {
  struct sockaddr_in addr = uv_ip4_addr(ip, 9991);
  uv_udp_send_t*     req  = malloc(sizeof(uv_udp_send_t));
  uv_buf_t           buf;

  buf.base = socket->buffer;
  buf.len  = sizeof(socket->buffer);

  gettimeofday((struct timeval*)socket->buffer, 0);

  str_repeat(socket->buffer + sizeof(struct timeval), 't', (unsigned int)sizeof(socket->buffer) - (unsigned int)sizeof(struct timeval));

  req->data = socket;
  
  CHECK(uv_udp_send(req, &socket->udp, &buf, 1, addr, send_cb));
  
  CHECK(uv_timer_stop(&socket->timer));
  
  CHECK(uv_timer_start(&socket->timer, timeout_cb, 100, 0));

  ++sent;
}


static void timeout_cb(uv_timer_t* handle, int status) {
  CHECK(status);

  send_packet((socket_t*)handle->data);
}


static uv_buf_t recv_alloc(uv_handle_t* handle, size_t suggested_size) {
  (void)handle;

  char* buf = (char*)malloc(suggested_size);
  return uv_buf_init(buf, suggested_size);
}


void recv_read(uv_udp_t *req, ssize_t nread, uv_buf_t buf, struct sockaddr *addr, unsigned flags) {
  (void)addr;
  (void)flags;

  if (nread == -1) {
    printf("%s\n", uv_strerror(uv_last_error(loop)));
    exit(1);
  }

  if (nread == PACKETSIZE) {
    struct timeval  tvnow;
    struct timeval* tvsent = (struct timeval*)buf.base;

    gettimeofday(&tvnow, 0);

    microseconds += (tvnow.tv_sec  - tvsent->tv_sec) * 1000000;
    microseconds +=  tvnow.tv_usec - tvsent->tv_usec;

    ++received;

    send_packet((socket_t*)req->data);
  }

  free(buf.base);
}


static void second_cb(uv_timer_t* handle, int status) {
  (void)handle;
  
  CHECK(status);

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
}


int main(int argc, char* argv[]) {
  if (argc > 1) {
    ip = argv[1];

    if (argc > 2) {
      count = atoi(argv[2]);
    }
  }

  printf("pinging %s using %d sockets\n", ip, count);

  loop    = uv_default_loop();
  sockets = malloc(sizeof(socket_t) * count);

  for (int i = 0; i < count; ++i ) {
    CHECK(uv_timer_init(loop, &sockets[i].timer));

    CHECK(uv_udp_init(loop, &sockets[i].udp));
    CHECK(uv_udp_bind(&sockets[i].udp, uv_ip4_addr("0.0.0.0", 0), 0));
    
    sockets[i].timer.data = &sockets[i];
    sockets[i].udp.data   = &sockets[i];

    CHECK(uv_udp_recv_start(&sockets[i].udp, recv_alloc, recv_read));
    
    send_packet(&sockets[i]);
  }

  uv_timer_t second;

  CHECK(uv_timer_init(loop, &second));
  CHECK(uv_timer_start(&second, second_cb, 1000, 1000));

  uv_run(loop, UV_RUN_DEFAULT);

  return 0;
}

