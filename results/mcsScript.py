import string
import sys
import os
import os.path
import re
import math
from math import sqrt
import statistics

#nodes = [10, 16, 30, 50, 100, 150, 200, 250, 300, 350, 400, 450, 500];
width = [25, 50, 75, 100, 125, 150, 175, 200];
nodes = [100, 200, 300, 400, 500, 600];


#os.path.join('/my/root/directory', 'in', 'here')
start_path = os.getcwd()

print start_path

def stddev(lst):
    """returns the standard deviation of lst"""
    mn = statistics.mean(lst)
    variance = sum([(e-mn)**2 for e in lst]) / len(lst)
    return sqrt(variance)


minDelay = []
tbSize = []
avgDelay = []
keep_phrases = ["EpcUeNas:DoRecvData", "yyyyyyyyyyyy", "xxxxx"]
line_counter = 0
line_error = 0

for n in nodes:
    for w in width:
        folder = "W" + str(w) + " N" + str(n) + "/UlPdcpStats.txt"
        folder2 = "W" + str(w) + " N" + str(n)
        final_path = os.path.join(start_path, folder)
        print final_path
        #print folder
        try:
            fp = open(final_path)
            while 1:
                line = fp.readline()
                if not line:
                    break
                line_counter += 1
                #for phrase in keep_phrases:
                #if phrase in line:
                words = re.split('; |, |s | |\* |% |\t|\n', line)
                if line_counter != 1:
                    try:
                        min_delay_value = float(words[12])
                        if min_delay_value != 0 and min_delay_value < 1.0:
                            minDelay.append((min_delay_value))
                        #if min_delay_value > 1.0:
                        #    print min_delay_value
                    except ValueError:
                        line_error += 1
                        #print "Could not sinr.append(10*math.log10(float(words[5])))", words[5]
                    #print float(words[5]), 10*math.log10(float(words[5]))
                else:
                    continue

                if line_counter == 30000:
                    break

            #avgDelay.append (statistics.mean(minDelay))
            #print("Len minDelay:", len(minDelay), " avgDelay: ", avgDelay)
            #print("Len minDelay:", len(minDelay), "minDelay:", minDelay)
            average1 = statistics.mean(minDelay)
            stddev1 = stddev(minDelay)
            print folder2, average1, stddev1, len(minDelay)
            #print("Len tbSize:", len(tbSize), " avgTbSize: ", avgTbSize)
            del minDelay[:]
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
    #exit()
