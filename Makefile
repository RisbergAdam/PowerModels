all:
	gcc mPower.c -o mPower -lcupti -lnvidia-ml -lrt -lcuda -std=gnu99 -I/usr/local/cuda/include -I/usr/local/cuda/extras/CUPTI/include -L/usr/local/cuda/extras/CUPTI/lib64

old:
	gcc main.c framework.c -o mPower -lcupti -lnvidia-ml -lrt -lcuda -std=gnu99 -I/usr/local/cuda/include -I/usr/local/cuda/extras/CUPTI/include -L/usr/local/cuda/extras/CUPTI/lib64

obj:
	gcc inject.c framework.c -c -std=gnu99

port:
	gcc inject.c framework.c -o inject -lnvidia-ml -lrt -std=gnu99 -std=gnu99
