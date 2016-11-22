#include "framework.h"

int helper_pmfInit(nvmlDevice_t* device) {
  nvmlReturn_t result;
  
  result = nvmlInit();

  if (result != NVML_SUCCESS) {
    printf("nvmlInit failed");
    return 0;
  }
  
  nvmlDeviceGetHandleByIndex(0, device);

  return 1;
}

void helper_pmfShutdown() {
  nvmlShutdown();
}

// END OF HELPER METHODS




pmf_t* pmfStart(nvmlDevice_t device) {
  pmf_t* pmf = (pmf_t*) malloc(sizeof(pmf_t));
  
  pmf->device = device;

  pmf->measurementCapacity = 1024*1024;
  pmf->measurementCount = 0;
  pmf->measurements = (pmfTimedData_t*) malloc(sizeof(pmfTimedData_t) * pmf->measurementCapacity);
  pmf->lastMeasurement = currMicro() - 100000; // -100000 needed, suspected unsynchronized cpu and gpu clock
  pmf->startTime = currMicro();
  
  // init mutex and start thread
  pmf->alive = 1;
  pthread_mutex_init(&pmf->aliveLock, NULL);
  
  pthread_create(&pmf->thread, NULL, p_startThread, pmf);

  return pmf;
}

void pmfLabel(pmf_t* pmf, char* name) {
  pmfTimedData_t nLabel;
  nLabel.data = name;
  //nLabel.timeMilli = currMilli();

  pmf->labels[pmf->labelCount] = nLabel;
  pmf->labelCount++;
}

void pmfEnd(pmf_t* pmf) {
  //pmf->endTime = currMilli();

  // stop measurement thread
  pthread_mutex_lock(&pmf->aliveLock);
  pmf->alive = 0;
  pthread_mutex_unlock(&pmf->aliveLock);
  pthread_join(pmf->thread, NULL);
  pthread_mutex_destroy(&pmf->aliveLock);
  pmf->endTime = currMicro();
}

void pmfExport(pmf_t* pmf, char* filename) {
  int i = 0;
  unsigned long long start = pmf->measurements[0].timeMilli;

  /*FILE* fp = fopen(filename, "w+");
  
  for (i = 0;i < pmf->measurementCount; i++) {

    pmfMeasurement_t* measurement = (pmfMeasurement_t*) pmf->measurements[i].data;
    
    fprintf(fp, "%lu\t%u\n",
	    pmf->measurements[i].timeMilli - start,
	    measurement->power);
  }

  fclose(fp);*/

  for (i = 0;i < pmf->measurementCount; i++) {
    pmfMeasurement_t* measurement = (pmfMeasurement_t*) pmf->measurements[i].data;
    
    printf("%llu\t%u\n",
	   pmf->measurements[i].timeMilli,
	    measurement->power);
  }
}



// PRIVATE FUNCTIONS

unsigned long long currMicro() {
  struct timespec currTime;
  clock_gettime(CLOCK_REALTIME, &currTime);
  return currTime.tv_nsec / 1000 + currTime.tv_sec * 1000000;
}

// DEAD FUNCTION
void measureValues(nvmlDevice_t device, pmfMeasurement_t* measurement) {
  // measure temperature
  unsigned int temp;
  nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temp);
  measurement->temperature = temp;

  // measure utilization
  nvmlUtilization_t utilization;
  nvmlDeviceGetUtilizationRates(device, &utilization);
  measurement->utilization = utilization.gpu;

  // measure power
  unsigned int power;
  nvmlDeviceGetPowerUsage(device, &power);
  measurement->power = power;
}

void insertMeasurement(pmf_t* pmf, pmfMeasurement_t* measurement, unsigned long long timestamp) {
  // pmf->measurementCount = next available position in vector
  // pmf->measurementCapacity = total available positions in vector
  if (pmf->measurementCount == pmf->measurementCapacity) {
    // grow vector
    pmf->measurementCapacity *= 2;
    pmfTimedData_t* grown = (pmfTimedData_t*) malloc(sizeof(pmfTimedData_t) * pmf->measurementCapacity);

    // copy over elements
    for (int i = 0;i < pmf->measurementCount;i++) {
      grown[i] = pmf->measurements[i];
    }

    free(pmf->measurements);
    pmf->measurements = grown;
  }

  pmf->measurements[pmf->measurementCount].timeMilli = timestamp;
  pmf->measurements[pmf->measurementCount].data = measurement;

  pmf->measurementCount++;
}

void* p_startThread(void* arg) {
  pmf_t* pmf = (pmf_t*) arg;

  int first = 1;
  
  while (1) {
    // see if we should stop measuring
    pthread_mutex_lock(&pmf->aliveLock);
    if (!pmf->alive) {
      pthread_mutex_unlock(&pmf->aliveLock);
      break;
    }
    pthread_mutex_unlock(&pmf->aliveLock);

    // ### READ FREQUENCY
    unsigned int clock;
    
    nvmlDeviceGetClockInfo(pmf->device, NVML_CLOCK_GRAPHICS, &clock);

    printf("C:%lu:%u\n", currMicro(), clock);
    
    // ### READ POWER
    
    // get sample count
    nvmlValueType_t sampleType;
    unsigned int sampleCount;
  
    nvmlDeviceGetSamples(pmf->device, NVML_TOTAL_POWER_SAMPLES, pmf->lastMeasurement, &sampleType, &sampleCount, NULL);
    nvmlSample_t* samples = (nvmlSample_t*) malloc(sampleCount * sizeof(nvmlSample_t));

    // read samples
    nvmlDeviceGetSamples(pmf->device, NVML_TOTAL_POWER_SAMPLES, pmf->lastMeasurement, &sampleType, &sampleCount, samples);

    if (first) {
      pmf->lastMeasurement = samples[sampleCount - 1].timeStamp;
      first = 0;
      continue;
    }
    
    // record them back to measurements array
    for (int i = 0;i < sampleCount;i++) {
      if (samples[i].timeStamp == 0 || samples[i].timeStamp == pmf->lastMeasurement) {
	// big wtf, but it happends
	continue;
      }
      
      //pmfMeasurement_t* measurement = (pmfMeasurement_t*) malloc(sizeof(pmfMeasurement_t));
      //measurement->power = samples[i].sampleValue.uiVal;
      //measurement->temperature = 0;
      //measurement->utilization = 0;
      //insertMeasurement(pmf, measurement, samples[i].timeStamp);
      unsigned long long timeStamp = samples[i].timeStamp;
      unsigned int power = samples[i].sampleValue.uiVal;
      
      printf("W:%lu:%u\n", timeStamp, power);
      
      pmf->lastMeasurement = timeStamp;
    }

    free(samples);
    
    // sleep for 10 ms
    usleep(10000);

  }
}
