#!/bin/bash
# author: Jingyu Li

mkdir build
cd build
cmake ..
make -j4
./pathtracer -t 4 -s 8 -l 4 -m 5 ../dae/sky/CBbunny.dae