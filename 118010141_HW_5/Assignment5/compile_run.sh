#!/bin/bash
# author: Jingyu Li

mkdir build
cd build
cmake ..
make -j4
./meshedit ../dae/teapot.dae