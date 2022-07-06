#!/bin/bash
# author: Jingyu Li

mkdir build
cd build
cmake ..
make -j4
./draw ../svg/basic/test4.svg