#!/bin/bash
# author: Jingyu Li

mkdir build
cd build
cmake ..
make -j4
./pathtracer -t 4 -s 1024 -l 128 -m 5  ../dae/sky/CBspheres.dae
