
all: client server

client: client.c ../../util.c
	gcc -Wall -pedantic -O2 -DNDEBUG -D_GNU_SOURCE -std=gnu99 $^ -lm -o $@

server: server.c
	gcc -Wall -pedantic -O2 -DNDEBUG -D_GNU_SOURCE -std=gnu99 $^ -o $@

