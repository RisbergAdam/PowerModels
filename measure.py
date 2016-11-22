#!/usr/bin/env python

# utility script that executes two processes: one measurement process
# and one computation process. The measurement process prints sampled
# values from the gpu labeles with GPU time, together with start and
# end CPU time, while the computation process optionally prints
# labels/milestones in CPU time

# ./measure.py <measureTime> <commands...>

import sys
import subprocess
import time


def main():
    mCommands = sys.argv[1:]
    occs = []

    for i in range(0, 30):
        print i
        labels, measurements, clockMeasurements, occupancy = measure(None, mCommands)
        occs.append(occupancy)
    
    print sum(occs)/len(occs)
    
# runs a measurement process for mTime seconds as measureProc,
# while executing mCommands as computeProc (which hopefully runs some cuda program)
# after that, the measurements from measureProc are resampled to a regular time interval,
# while labels printed from computeProc are time-normalized to times from measureProc
# returns (labels, measurements) where labels: [(time ms, text)]and measurements: [(time ms, power mw)]
def measure(mCommands):
    computeProc = subprocess.Popen(mCommands, stdout=subprocess.PIPE)

    computeOutput, _ = computeProc.communicate()
    computeOutput = computeOutput.split('\n')

    measureOutput = computeOutput
    
    minTime, maxTime = extractTimes(measureOutput)
    interval = (minTime, maxTime)

    for line in computeOutput:
        if line.startswith("#MP:ERROR"):
            print "wtf"
    
    # extract power measurements
    #powerMeasurements = extractPowerMeasurements(measureOutput, interval)
    powerMeasurements = extractByName(measureOutput, interval, "#MP:M:W:")

    # extract clock measurements
    clockMeasurements = extractByName(measureOutput, interval, "#MP:M:C:")

    # extract performance counters
    activeCycles = extractByName(measureOutput, None, "#MP:M:AC:")
    activeWarps = extractByName(measureOutput, None, "#MP:M:AW:")
    occupancy = calculateOccupancy(activeCycles, activeWarps)
    
    # extract labels from computeOutput
    labels = extractLabels(computeOutput)

    return (labels, powerMeasurements, clockMeasurements, occupancy)

def calculateOccupancy(activeCycles, activeWarps):
    activeCycles = map(lambda c: c[1], activeCycles)
    activeWarps = map(lambda w: w[1], activeWarps)

    for i in range(1, len(activeCycles)):
        activeCycles[i] += activeCycles[i-1]
        activeWarps[i] += activeWarps[i-1]

    return sum(activeWarps)/sum(activeCycles)/48.0 * 100.0 * 0.8689 - 6.152


def interpolate(wattOverTime1, wattOverTime2, time):
    f = 0
    if not wattOverTime2[0] - wattOverTime1[0] == 0:
        f = float(time - wattOverTime1[0]) / float(wattOverTime2[0] - wattOverTime1[0])
    # clamp f to 0 < f < 1
    f = min(max(f, 0.0), 1.0)
    return wattOverTime1[1] + (wattOverTime2[1] - wattOverTime1[1])*f


def interpolateLists(wattOverTime, times):
    wIndex = 0
    interpolated = []
    
    for time in times:
        
        while wIndex+2 < len(wattOverTime) and wattOverTime[wIndex+1][0] < time:
            wIndex += 1
        
        interp = interpolate(wattOverTime[wIndex], wattOverTime[wIndex+1], time)
        interpolated.append((time, interp))

    return interpolated


def extractTimes(measureOutput):
    #minTime = long(measureOutput[0].split(":")[1])
    #maxTime = minTime
    minTime = None
    maxTime = None
    
    for line in measureOutput[:-1]:
        if not line.startswith("#MP:M:"):
            continue
        sLine = line.split(":")
        time = long(sLine[3])
        
        if maxTime == None or time > maxTime:
            maxTime = time

        if minTime == None or time < minTime:
            minTime = time

    return long(minTime/1000.0), long(maxTime/1000.0)


def extractByName(measureOutput, interval, name):
    measurements = []
    
    for line in measureOutput[:-1]:
        if not line.startswith(name):
            continue
        sLine = line.split(":")
        timeMicro = long(sLine[3])
        timeMilli = long(timeMicro/1000.0)
        wattMilli = long(sLine[4])
        measurements.append((timeMilli, wattMilli))

    if interval == None:
        return measurements
        
    interpMeasurements = interpolateLists(measurements, range(interval[0], interval[1], 10))
    return interpMeasurements


def extractLabels(computeOutput):
    labels = []
    
    for line in computeOutput:
        if not line.startswith("#MP:L:"):
            continue
        sLine = line.split(':')
        timeMicro = long(sLine[2])
        timeMilli = long(timeMicro/1000.0)
        text = sLine[3]
        labels.append((timeMilli, text))

    return labels

if __name__ == '__main__':
    main()
