#ifndef __powermodels_framework_h__
#define __powermodels_framework_h__

#include "nvml.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

// the helper methods simply encapsulate calls
// to the nvml library if the user does not want
// to do these themselves

// initializes nvml and returns the first device
int helper_pmfInit(nvmlDevice_t* device);

// shuts down nvml
void helper_pmfShutdown();

typedef struct pmfMeasurement_st {
  unsigned int temperature;
  unsigned int utilization;
  unsigned int power;
} pmfMeasurement_t;

typedef struct pmfTimedData_st {
  void* data;
  unsigned long long timeMilli;
} pmfTimedData_t;

typedef struct pmf_st {
  nvmlDevice_t device;
  
  long startTime;
  long endTime;

  pthread_t thread;
  int alive;
  pthread_mutex_t aliveLock;
  
  pmfTimedData_t labels[32];
  int labelCount;

  // dynamically growing array
  pmfTimedData_t* measurements;
  int measurementCapacity;
  
  int measurementCount;
  unsigned long long lastMeasurement;
  
} pmf_t;

// starts measurements on a specific device
// by spawning a thread that does continuous
// measurements
pmf_t* pmfStart(nvmlDevice_t device);

void pmfLabel(pmf_t* pmf, char* name);

// stops the measurements by stopping the
// measuring thread
void pmfEnd(pmf_t* pmf);

// writes the measurements to a file
void pmfExport(pmf_t* pmf, char* filename);


// PRIVATE FUNCTIONS BELOW

void insertMeasurement(pmf_t* pmf, pmfMeasurement_t* measurement, unsigned long long timestamp);

unsigned long long currMicro();

void* p_startThread(void*);

#endif
