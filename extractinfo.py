import string
import sys
import os
import os.path
import re
import math
from math import sqrt
import statistics
import matplotlib.pyplot as plt 
import operator
from collections import OrderedDict
#nodes = [10, 16, 30, 50, 100, 150, 200, 250, 300, 350, 400, 450, 500];
           
width = ["100"];#0, 25, 50, 75, 100, 125, 150, 175, 200];
nodes = ["100"];

NUE= 104

#os.path.join('/my/root/directory', 'in', 'here')
start_path = os.getcwd()

print start_path

def stddev(lst):
    """returns the standard deviation of lst"""
    mn = statistics.mean(lst)
    variance = sum([(e-mn)**2 for e in lst]) / len(lst)
    return sqrt(variance)


########## PDCP STATS#######################
rnti=[]
txbytes=[]
rxbytes=[]
delay=[]
pdr =[]
for i in range(NUE):
    txbytes.append(0)
    rxbytes.append(0)
    pdr.append(0)
    delay.append(0.0)
line_counter = 0
line_error = 0
nCounter = 0

final_path = os.path.join(start_path,"UlPdcpStats.txt")
print final_path
        
try:
    fp = open(final_path)
    while line_counter != 10000:
        line = fp.readline()
        if not line:
            break
        line_counter += 1
        words = re.split('; |, |s | |\* |% |\t|\n', line)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
        if line_counter != 1:
            try:
                ue =(int(words[4]))
                rnti.append(ue)
                txbytes[ue]= int(words[6])+txbytes[ue]
                rxbytes[ue]= int(words[8])+rxbytes[ue]
                de =float(words[10])
                if(delay[ue]!=0 and (de)!=0):
                    delay[ue] = ((float(words[10])) + delay[ue])/2.0
                elif (delay[ue]==0 and (de)!=0) :
                    delay[ue] = (float(words[10]))

            except ValueError:
                line_error += 1
        else:
            continue


 
    print "Txbytes"
    print txbytes
    print "Rxbytes"
    print rxbytes
    pdr = [(x*1.0)/y if y!= 0 else 0 for x,y in zip(rxbytes, txbytes)]


    print "PDR"
    print pdr




    #zipped2=zip(rnti,delay)
    #delay_new= sorted(zipped2, key=operator.itemgetter(0))
    print "DELAY"
    print delay
    line_counter = 0

except IOError:
            print "Could not open file! Please close Excel!"
            pass

nCounter += 1

########## MAC STATS#######################

mcs=[]
repetition = []
tone =[]
rnti1 =[]
for i in range(NUE):
    mcs.append(0)
    tone.append(0)
    repetition.append(0)


line_counter = 0
line_error = 0
nCounter = 0

final_path = os.path.join(start_path,"UlMacStats.txt")
print final_path
        
try:
    fp = open(final_path)
    while line_counter != 10000:
        line = fp.readline()
        if not line:
            break
        line_counter += 1
        words = re.split('; |, |s | |\* |% |\t|\n', line)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
        if line_counter != 1:
            try:
                ue=(int(words[5]))
                rnti1.append(ue)
                mcs[ue] = (int(words[6]))
                repetition[ue] = (int(words[8]))
                toneid = ((int(words[9]))) #+ tone[ue])/2.0   
                if toneid <= 11:
                    tone[ue]=1
                elif toneid > 11 and toneid < 16:
                    tone[ue]=3
                elif toneid >= 16 and toneid <18:
                    tone[ue]=6
                elif toneid ==18:
                    tone[ue]=12
                elif toneid > 18:
                    tone[ue]=0


              
            except ValueError:
                line_error += 1
        else:
            continue
    #ravg = statistics.mean(r)
    #stddev = stddev(r)
    print "mcs"
    print mcs
    print "repetition"
    print repetition
    print "tone"
    print tone
    
    line_counter = 0
  

except IOError:
            print "Could not open file! Please close Excel!"
            pass

nCounter += 1

########## POSITION OR DISTANCE FROM ENODEB#######################

distance =[]
for i in range(NUE):
    distance.append(0)
keep_phrases = ["EpcUeNas:DoRecvData", "yyyyyyyyyyyy", "xxxxx"]
line_counter = 0
line_error = 0
nCounter = 0

final_path = os.path.join(start_path,"cmd.txt")
print final_path
        
try:
    fp = open(final_path)
    while line_counter != 10000:
        line = fp.readline()
        if not line:
            break
        line_counter += 1
        words = re.split('; |, |s | |\* |% |\t|\n', line)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
        if "rnti" in words:
            ue=(int(words[1]))
            distance[ue]=( sqrt (pow(float(words[3]),2)+pow(float(words[4]),2)))
        else:
            continue
 
    print distance
    print "\n\n\n"


except IOError:
            print "Could not open file! Please close Excel!"
            pass

nCounter += 1

zipall = zip (distance,repetition,mcs, tone,pdr,delay)
out= sorted(zipall, key=operator.itemgetter(0))

for a,b,c,d,e,f in out:
    print (str(a)+ ' '+ str(b)+ ' '+  str(c)+ ' '+  str(d)+ ' '+  str(e)+ ' '+  str(f)+ ' ')

with open('result.txt', 'a') as the_file:
    the_file.write (str('D')+ ' '+ str('R')+ ' '+  str('M')+ ' '+  str('T')+ ' '+  str('P')+ ' '+  str('D')+ ' '+'\n')
    for a,b,c,d,e,f in out:
        the_file.write (str(a)+ ' '+ str(b)+ ' '+  str(c)+ ' '+  str(d)+ ' '+  str(e)+ ' '+  str(f)+ ' '+'\n')
      