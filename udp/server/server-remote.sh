#!/bin/sh

python -m SimpleHTTPServer 9991 &
PY=$!

trap "kill $PY; exit 0" INT

ssh -t 192.168.0.2 rm -f /tmp/udp-test-server "&&" wget -q http://192.168.0.8:9991/server -O /tmp/udp-test-server "&&" echo "&&" chmod u+x /tmp/udp-test-server "&&" /tmp/udp-test-server

kill $PY

