import string
import sys
import os
import os.path
import re
import math
from math import sqrt
import statistics

#nodes = [10, 16, 30, 50, 100, 150, 200, 250, 300, 350, 400, 450, 500];
           
width = ["100"];#0, 25, 50, 75, 100, 125, 150, 175, 200];
nodes = ["100"];



#os.path.join('/my/root/directory', 'in', 'here')
start_path = os.getcwd()

print start_path

def stddev(lst):
    """returns the standard deviation of lst"""
    mn = statistics.mean(lst)
    variance = sum([(e-mn)**2 for e in lst]) / len(lst)
    return sqrt(variance)


pdr = [0]*101
tbSize = []
avgDelay = []
keep_phrases = ["EpcUeNas:DoRecvData", "yyyyyyyyyyyy", "xxxxx"]
line_counter = 0
line_error = 0
appStartTime = [1, 5, 5, 6, 20, 100]
maxPacketInterval = [20, 20, 20, 20, 20, 40]
minPacketInterval = [10, 10, 10, 10, 10, 30]
nCounter = 0


for w in range(0,len(width)):
    for n in range(0,len(nodes)):
        folder = "W" + width[w] + "N" + nodes[n]+ "/UlPdcpStats.txt"
        
        final_path = os.path.join(start_path, folder)
        #print final_path
        #print folder
        try:
            fp = open(final_path)
            while line_counter != 100:
                line = fp.readline()
                if not line:
                    break
                line_counter += 1
                #for phrase in keep_phrases:
                #if phrase in line:
                words = re.split('; |, |s | |\* |% |\t|\n', line)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
                if line_counter != 1:
                    try:
                        txbytes = float(words[6])
                        rxbytes = float(words[8])
                        startTime = float(words[0])
                        #pdr.append(rxbytes/txbytes)
                        index= int(words[4])
                        pdr [index]=rxbytes/txbytes
                        #if min_delay_value > 1.0:
                        #    print min_delay_value
                    except ValueError:
                        line_error += 1
                        #print "Could not sinr.append(10*math.log10(float(words[5])))", words[5]
                    #print float(words[5]), 10*math.log10(float(words[5]))
                else:
                    continue

                #if line_counter == 100:
                   # break

            #avgDelay.append (statistics.mean(minDelay))
            #print("Len minDelay:", len(minDelay), " avgDelay: ", avgDelay)
            #print("Len minDelay:", len(minDelay), "minDelay:", minDelay)
            average1 = statistics.mean(pdr)
            stddev1 = stddev(pdr)
            print folder, average1, stddev1
            #print("Len tbSize:", len(tbSize), " avgTbSize: ", avgTbSize)
            del pdr[:]
            line_counter = 0
        except IOError:
            print "Could not open file! Please close Excel!", folder
            pass
        #print("avgSinr[", item, "]: ", avgSinr)
        #average1 = float(sum(avgSinr)) / max(len(avgSinr), 1)
        #if len(avgDelay) != 0:
        #    average1 = statistics.mean(avgDelay)
        #    stddev1 = stddev(avgDelay)
        #    print average1, stddev1
        del avgDelay[:]
    nCounter += 1
    #exit()
