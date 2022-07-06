#!/bin/bash
# author: Jingyu Li

mkdir build
cd build
cmake ..
make -j4
./asg2 -r 30
# ./asg2 -r