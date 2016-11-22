#!/usr/bin/env python

import sys
import os
import math

# ./multiGather.py <inputDirectory>

def main():
    measurements, labelTitles, labels = multiGather(sys.argv[1])
    popedLabelsCount = 0

    print labels
    
    for time, m, mMax, mMin in measurements:
        labelStr = ""

        if len(labels) > 0 and labels[0][0] < time:
            head = labels.pop(0)
            popedLabelsCount += 1
            labelStr = '\t'*(popedLabelsCount) + str(m)
        
        print str(int(time)) + "\t" + str(m) + "\t" + str(mMax) + "\t" + str(mMin) + labelStr
    

def multiGather(inDir):
    # compile measurements
    inFiles = os.listdir(inDir)

    data = []
    occupancies = []
    times = None

    for inFile in inFiles:
        if not "m" in inFile:
            continue
    
        fileContent = open(inDir + "/" + inFile).readlines()
        splitContent = map(lambda s: s.split('\t'), fileContent[2:-1])
        numberContent = map(lambda s: map(float, s[1:]), splitContent)

        occupancy = float(fileContent[0].split(":")[1])
        occupancies.append(occupancy)
        
        # extract time values for the first measurement file,
        # time should be the same for all measurements
        if times == None:
            times = map(lambda s: float(s[0]), splitContent)
        
        data.append(numberContent)
        
    maxLen = max(map(lambda d: len(d), data))

    for d in data:
        while len(d) < maxLen:
            d.append((26650.0, 342.0))

    # transpose the data list
    data = zip(*data)

    # compile the labels
    labels = {}
    
    for inFile in inFiles:
        if not "l" in inFile:
            continue

        fileContent = open(inDir + "/" + inFile).readlines()
        #splitContent = map(lambda s: s.split('\t'), fileContent[1:])

        for labelStr in fileContent[1:]:
            labelSplit = labelStr.split('\t')
            labelText = labelSplit[1][:-1]
            labelTime = float(labelSplit[0])

            if not labelText in labels:
                labels[labelText] = []

            labels[labelText].append(labelTime)

    # average the time for the labels
    labelsMean = []
    
    for key in labels.keys():
        lMean = mean(labels[key])
        if lMean < 0 or key == "START":
            continue
        #labelsMean[key] = int(lMean)
        labelsMean.append((lMean, key))

    labelsMean.sort(key=lambda l: l[0])
    labels = labelsMean
    
    # print compiled data and labels
    labelTitles = ""
    for label in labels:
        labelTitles += '\t' + label[1]
    
    popedLabelsCount = 0
    measurements = []
    
    for i in range(len(times)):

        time = times[i]
        singleTimestep = zip(*(data[i]))
        
        # singleTimestep[0] is power
        # singleTimestep[1] is frequency

        averageData = []
        averageData.append(int(time))
        
        for d in singleTimestep:
            averageData.append(int(mean(d)))
        
        # split power and frequency data
        #d = list(zip(*data[i])[0])
        #d.sort()

        #low = int(len(d) * 0.25)
        #high = int(len(d) * 0.75)

        #m = int(mean(d[low:high]))
        #mMax = int(mean(d[:low]))
        #mMin = int(mean(d[high:]))
        
        #measurements.append((time, m, mMax, mMin))
        measurements.append(averageData)

    return (measurements, labelTitles, labels, occupancies)
    
def mean(L):
    return 0 if len(L) == 0 else sum(L)/len(L)

def stdev(L):
    m = mean(L)
    mRemoved = map(lambda l: l - m, L)
    squared = map(lambda l: l*l, mRemoved)
    variance = sum(squared)/len(L)
    return math.sqrt(variance)

if __name__ == '__main__':
    main()
