
UDP speed test
==============

These are several implementations of a simple UDP ping pong program. I am using these to see which can perform the most ping-pong's per second.


*server*

This is a simple single threaded server that echos each request it receives.

*simple*

This is the most simple version. It is single threaded and only uses one socket. It can either use SO\_RCVTIMEO or select().

*event*

This version is also single threaded but uses multiple sockets and select().

*threaded*

This version uses multiple threads which all use one socket.

*libuv*

This version uses multiple sockets and [libuv](https://github.com/joyent/libuv).

