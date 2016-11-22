#!/usr/bin/env python

import subprocess
import sys
import time
import measure


# ./multiMeasure.py <measurementCount> <outputDirectory> <commands to run one measurement...>

def main():
    mCount = int(sys.argv[1])
    outDir = sys.argv[2]
    mCommands = sys.argv[3:]


def multiMeasure(mCount, outDir, mCommands):
    for i in range(0, mCount):
        print i
        labels, measurements, clockMeasurements, occupancy = measure.measure(mCommands)
        measureFile = open(outDir + "/m" + str(i) + ".txt", "w")
        labelFile = open(outDir + "/l" + str(i) + ".txt", "w")

        startTime = measurements[0][0]

        measureFile.write("occupancy:" + str(occupancy) + "\n")
        measureFile.write("time(ms)\tpower(mw)\n")
        labelFile.write("time(ms)\tlabel\n")
    
        for m, c in zip(measurements, clockMeasurements):
            measureFile.write(str(m[0] - startTime) + '\t' + str(m[1]) + "\t" + str(c[1]) + "\n")

        for l in labels:
            labelFile.write(str(l[0] - startTime) + '\t' + l[1] + "\n")

    measureFile.close()
    labelFile.close()

    
if __name__ == '__main__':
    main()
