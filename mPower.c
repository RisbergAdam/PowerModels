#ifndef __mPower_c__
#define __mPower_c__

#include "mPower.h"

mPower_t* mPower_create(int gpuIndex) {
  mPower_t* mPower = (mPower_t*) malloc(sizeof(mPower_t));

  // create CUPTI  
  cuInit(0);
  cuDeviceGet(&mPower->cupti_device, 0);
  cuCtxCreate(&mPower->cupti_context, 0, mPower->cupti_device);
  cuptiSetEventCollectionMode(mPower->cupti_context, CUPTI_EVENT_COLLECTION_MODE_CONTINUOUS);

  // create eventGroup for CUPTI occupancy
  // get metric id from metric name
  CUpti_MetricID occupancyId;
  CUpti_EventGroupSets* passData; // not used

  //cuptiMetricGetIdFromName(mPower->cupti_device, "achieved_occupancy", &occupancyId);
  cuptiMetricGetIdFromName(mPower->cupti_device, "achieved_occupancy", &occupancyId);
  cuptiMetricCreateEventGroupSets(mPower->cupti_context, sizeof(occupancyId), &occupancyId, &passData);
  
  // since we are reading the achieved_occupancy metric
  // there will always only be 1 EventGroupSet
  CUpti_EventGroupSet occupancy_eventGroupSet = passData->sets[0]; 
  mPower->occupancy_eventGroup = occupancy_eventGroupSet.eventGroups[0];
  cuptiEventGroupEnable(mPower->occupancy_eventGroup);

  // get the event ids from the eventGroup
  size_t eventIdsSize = sizeof(CUpti_EventID) * 2;
  mPower->occupancy_eventIds = (CUpti_EventID*) malloc(eventIdsSize);
  cuptiEventGroupGetAttribute(mPower->occupancy_eventGroup,
			      CUPTI_EVENT_GROUP_ATTR_EVENTS,
			      &eventIdsSize,
			      mPower->occupancy_eventIds);
  // eventIds[0] = active_warps, eventIds[1] = active_cycles
  
  // create NVML
  int result = nvmlInit();
  
  if (result != NVML_SUCCESS) {
    printf("nvmlInit failed");
    return NULL;
  }

  nvmlDeviceGetHandleByIndex(gpuIndex, &mPower->nvml_device);

  // start thread
  mPower->thread_running = 1;
  pthread_create(&mPower->thread, NULL, mPower_threadStart, mPower);

  return mPower;
}

void mPower_destroy(mPower_t* mPower) {
  mPower->thread_running = 0;
  pthread_join(mPower->thread, NULL);
  nvmlShutdown();
}

void* mPower_threadStart(void* arg) {
  mPower_t* mPower = (mPower_t*) arg;
  
  while (mPower->thread_running) {
    mPower_sample(mPower);
    // sleep for 1 ms
    usleep(1000);
  }

  return NULL;
}

unsigned long long mPower_currMicro() {
  struct timespec currTime;
  clock_gettime(CLOCK_REALTIME, &currTime);
  return currTime.tv_nsec / 1000 + currTime.tv_sec * 1000000;
}

void mPower_sample(mPower_t* mPower) {
  // sample NVML
  // read frequency
  unsigned int clock;
  nvmlDeviceGetClockInfo(mPower->nvml_device, NVML_CLOCK_GRAPHICS, &clock);
  printf("#MP:M:C:%lu:%u\n", mPower_currMicro(), clock);

  // read power
  unsigned int power;
  nvmlDeviceGetPowerUsage(mPower->nvml_device, &power);
  printf("#MP:M:W:%lu:%u\n", mPower_currMicro(), power);
  
  // sample CUPTI
  const int eventBuffSize = 256;
  size_t bytesRead = sizeof(uint64_t) * eventBuffSize;
  uint64_t eventBuffer[eventBuffSize];

  char * counterLabels[2];
  counterLabels[0] = "AW";
  counterLabels[1] = "AC";

  unsigned long long currMicro = mPower_currMicro();
  
  // only two event types
  for (int i = 0;i < 2;i++) {
    CUptiResult cuResult = cuptiEventGroupReadEvent(mPower->occupancy_eventGroup,
			     CUPTI_EVENT_READ_FLAG_NONE,
			     mPower->occupancy_eventIds[i],
			     &bytesRead,
			     eventBuffer);

    if (cuResult == CUPTI_EVENT_OVERFLOW) {
      printf("#MP:ERROR:EVENT_OVERFLOW");
    }
    
    for (int j = 0;j < bytesRead / sizeof(uint64_t);j++) {
      printf("#MP:M:%s:%lu:%lu\n", counterLabels[i], currMicro, eventBuffer[j]);
    }

    // reset read bytes
    bytesRead = sizeof(uint64_t) * eventBuffSize;
  }
 
}

void mPower_label(char* text) {
  struct timespec currTime;
  clock_gettime(CLOCK_REALTIME, &currTime);
  unsigned long long time = currTime.tv_nsec / 1000 + currTime.tv_sec * 1000000;
  printf("#MP:L:%lu:%s\n", time, text);
}

#endif
