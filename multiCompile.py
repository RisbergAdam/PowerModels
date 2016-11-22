#!/usr/bin/env python

import os
import multiMeasure
import multiGather
import subprocess

#blockSizes = [2, 4, 8, 12, 16]
#mTimes = [14, 10, 10, 8, 8]
#blockSizes = [2, 4, 6, 8, 10, 12, 14, 16]
#mTimes = [None] * len(blockSizes)
#outputDirectory = "gaussianMeasurements2"
#runBenchmark = ["./gaussian.sh"]
#recompileBenchmark = ["./gaussianRecompile.sh"]

#blockSizes = [8, 16, 24, 32]
#mTimes = [17, 10, 10, 10]
blockSizes = [8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32]
outputDirectory = "hotspotMeasurements"
runBenchmark = ["./hotspot.sh"]
recompileBenchmark = ["./hotspotRecompile.sh"]

mCount = 15

measure = False
gather = True

if measure:
    for block in blockSizes:
        devnull = open(os.devnull, "w")
    
        # recompile benchmark to set block size
        print("recompiling...")
        subprocess.call(recompileBenchmark + [str(block)], stdout=devnull, stderr=devnull)

        # run multiMeasure
        print("multiMeasure...")
        blockOutputDir = outputDirectory + "/block" + str(block)
        if not os.path.exists(blockOutputDir):
            os.makedirs(blockOutputDir)
        multiMeasure.multiMeasure(mCount, blockOutputDir, runBenchmark)

if gather:
    for block in blockSizes:
        blockOutputDir = outputDirectory + "/block" + str(block)
        finalOutputFile = outputDirectory + "/final_block" + str(block) + ".txt"
        
        # run multiGather
        # print("multiGather...")
        measurements, labelTitles, labels, occupancies = multiGather.multiGather(blockOutputDir)

        kernelStart = None
        kernelEnd = None
        
        for labelTime, labelText in labels:
            if labelText == "kernelStart":
                kernelStart = long(labelTime)
            elif labelText == "kernelEnd":
                kernelEnd = long(labelTime)

        kernelMeasurements = []
        
        for time, power, frequency in measurements:
            if kernelStart < long(time) and long(time) < kernelEnd:
                kernelMeasurements.append((power - 26650.0) / 1000.0 * 0.01)

        avrOcc = sum(occupancies) / len(occupancies)
                
        print("energy for blocksize: " + str(block) + " is " + str(sum(kernelMeasurements)) + " E, occupancy is " + str(avrOcc))
                
        # write final measurement
        #finalFile = open(finalOutputFile, "w")
        
        #popedLabelsCount = 0

        #firstTime = int(measurements[0][0])
    
        #for time, power, frequency in measurements:
        #    labelStr = ""

        #    if len(labels) > 0 and labels[0][0] < time:
        #        head = labels.pop(0)
        #        popedLabelsCount += 1
        #        labelStr = '\t'*(popedLabelsCount) + str(power)
        
        #    finalFile.write(str(int(time) - firstTime) + "\t" + str(power) + "\t" + str(frequency) + labelStr + "\n")

        #finalFile.close()
