#!/bin/bash
# author: Jingyu Li

mkdir build
cd build
cmake ..
make -j4
./pathtracer -t 4 -s 16 -l 4 -m 5  ../dae/sky/CBdragon_microfacet_au.dae
