#!/bin/bash

for N in {1..500}; do
    echo "server.cpp" | ./client phoenix 8081 &
    echo $N
done
