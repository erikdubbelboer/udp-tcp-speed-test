
// For more info see: http://nikhilm.github.com/uvbook/networking.html#udp

#include <stdio.h>   // printf()
#include <stdlib.h>  // exit()

#include "uv.h"


#define CHECK(x) \
  do { \
    if (x != 0) { \
      printf("%s\n", uv_strerror(uv_last_error(loop))); \
      exit(1); \
    } \
  } while (0)


typedef struct socket_s {
  uv_udp_t   udp;
  uv_timer_t timer;
} socket_t;


static int         count = 3;
static uv_loop_t*  loop;
static socket_t*   sockets;
static uint32_t    sent;
static uint32_t    received;


static void timeout_cb(uv_timer_t* handle, int status);


static void send_cb(uv_udp_send_t* req, int status) {
  CHECK(status);

  free(req);
}


static void send_packet(socket_t* socket) {
  struct sockaddr_in addr = uv_ip4_addr("127.0.0.1", 9991);
  uv_udp_send_t*     req  = malloc(sizeof(uv_udp_send_t));
  uv_buf_t           buf;

  char buffer[65507];  // Max UDP packet size ((2^16 - 1) - (8 byte UDP header) - (20 byte IP header))
  buf.base = buffer;
  buf.len  = snprintf(buf.base, sizeof(buffer), "test");

  if (buf.len > sizeof(buffer)) {
    buf.len = sizeof(buffer);
  }

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
  (void)buf;
  (void)addr;
  (void)flags;

  if (nread == -1) {
    printf("%s\n", uv_strerror(uv_last_error(loop)));
    exit(1);
  }

  if (nread > 0) {
    ++received;

    send_packet((socket_t*)req->data);
  }

  free(buf.base);
}


static void second_cb(uv_timer_t* handle, int status) {
  (void)handle;
  
  CHECK(status);

  printf("%6u per second (%6u missing)\n", received, (sent - received));

  sent     = 0;
  received = 0;
}


int main(int argc, char* argv[]) {
  if (argc > 1) {
    count = atoi(argv[1]);
  }

  printf("using %d sockets\n", count);

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

  sent     = 0;
  received = 0;

  uv_timer_t second;

  CHECK(uv_timer_init(loop, &second));
  CHECK(uv_timer_start(&second, second_cb, 1000, 1000));

  uv_run(loop, UV_RUN_DEFAULT);

  return 0;
}

