#ifndef __mPower_h__
#define __mPower_h__

#include <signal.h>
#include "nvml.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <cuda.h>
#include <cupti.h>
#include <pthread.h>

typedef struct mPower_st {
  // NVML data
  nvmlDevice_t nvml_device;

  // CUPTI data
  CUcontext cupti_context;
  CUdevice cupti_device;

  // CUPTI occupancy
  CUpti_EventGroup occupancy_eventGroup;
  CUpti_EventID * occupancy_eventIds;

  // threaded
  pthread_t thread;
  volatile int thread_running;
  
} mPower_t;

mPower_t* mPower_create(int gpuIndex);

void mPower_destroy(mPower_t* mPower);

void* mPower_threadStart(void* arg);

unsigned long long mPower_currMicro();

void mPower_sample(mPower_t* mPower);

#endif
