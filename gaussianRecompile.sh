#!/bin/bash

cd ../rodinia_3.1/cuda/gaussian
/usr/local/cuda/bin/nvcc  gaussian.cu -o gaussian -I/usr/local/cuda/include -L/usr/local/cuda/lib64 -D RD_WG_SIZE=$1
