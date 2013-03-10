#!/bin/sh

taskset 2 ./client &
P1=$!

taskset 3 ./client &
P2=$!

taskset 4 ./client &
P3=$!

taskset 5 ./client &
P4=$!

taskset 6 ./client &
P5=$!

trap "kill $P1 $P2 $P3 $P4 $P5; exit 0" INT

while true; do
  sleep 1
  echo
done

