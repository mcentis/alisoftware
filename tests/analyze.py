# program to launch the analysis of the binary files, the binary files must be in a directory containing no subdirectory and a .cfg file
# the run type is determined using the extension of the run file

import sys
import os

# check input
if len(sys.argv) != 2:
    sys.exit('\tUsage: python analyze.py dataFolder')

progPed = './executables/pedToRoot'
progDel = './executables/delToRoot'
progCal = './executables/calToRoot'
progData = './executables/dataToRoot'

dataPath = sys.argv[1]
print 'Process data in %s' % dataPath

confFiles = []
pedRuns = []
delRuns = []
calRuns = []
dataRuns = []

for dirPath, dirNames, fileNames in os.walk(dataPath):
    confFiles = []
    pedRuns = []
    delRuns = []
    calRuns = []
    dataRuns = []
    if len(dirNames) == 0:
        confFiles = [f for f in fileNames if f.endswith('.cfg')]
        pedRuns = [f for f in fileNames if f.endswith('.ped')]
        delRuns = [f for f in fileNames if f.endswith('.del')]
        calRuns = [f for f in fileNames if f.endswith('.cal')]
        dataRuns = [f for f in fileNames if f.endswith('.dat')]

        #pedestals
        for r in pedRuns:
            command = progPed + ' ' + dirPath + '/' + r + ' ' + dirPath + '/' + confFiles[0]
            os.system(command)
        #delay
        for r in delRuns:
            command = progDel + ' ' + dirPath + '/' + r + ' ' + dirPath + '/' + confFiles[0]
            os.system(command)
        #calibration
        for r in calRuns:
            command = progCal + ' ' + dirPath + '/' + r + ' ' + dirPath + '/' + confFiles[0]
            os.system(command)
        #data
        for r in dataRuns:
            command = progData + ' ' + dirPath + '/' + r + ' ' + dirPath + '/' + confFiles[0]
            os.system(command)

"""
    print dirPath + '================================================'
    print confFiles
    print pedRuns
    print delRuns
    print calRuns
    print dataRuns
"""
