#!/bin/bash

/opt/compiler/gcc-8.2/bin/g++ test_timer_task.cpp timer_task.cpp -latomic -lpthread -std=c++17

while true; do
    ./a.out
done
