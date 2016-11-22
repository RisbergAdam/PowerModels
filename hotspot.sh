#!/bin/bash

cd ../rodinia_3.1/cuda/hotspot
#/usr/local/cuda/bin/nvcc hotspot.cu -o hotspot -I/usr/local/cuda/include -L/usr/local/cuda/lib64 -D RD_WG_SIZE=$1
./hotspot 512 2 60000 ../../data/hotspot/temp_1024 ../../data/hotspot/power_1024 output.out
