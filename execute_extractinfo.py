import os,errno
import shutil
import fileinput
import sys
import re
import glob
import statistics
import math
working_directory= os.path.join(os.getcwd()+"/RESULTS10/RESULTS100/")

def stddev(lst):
    """returns the standard deviation of lst"""
    mn = statistics.mean(lst)
    variance = sum([(e-mn)**2 for e in lst]) / len(lst)
    return math.sqrt(variance)

#print working_directory
folder = "RESULTS10"
NRUNS=4
START=0
command= "python extractinfo.py"
mycwd = os.getcwd()
for i in range(START,NRUNS+1):	
	os.chdir(folder+"/RESULTS10"+str(i))
	try:
		os.remove("result.txt")
	except OSError:
		pass
	os.system(command)
	os.chdir(mycwd)




mcs=[]
repetition = []
tone =[]
d =[]
delay=[]
pdr=[]
A=0
D=1000
l=[5000,10000,20000]

w=len(l)
h=NRUNS
PDRMatrix = [[0 for x in range(w)] for y in range(h)] 
delayMatrix = [[0 for x in range(w)] for y in range(h)] 
for i in range(0,len(l)):
    mcs.append(0)
    tone.append(0)
    repetition.append(0)
    d.append(0)
    delay.append(0)
    pdr.append(0.0)

for k in range(START,NRUNS+1):
	if(k>START):
		for j in range(0,len(l)):
			PDRMatrix[k-1][j]=pdr[j]
			delayMatrix[k-1][j]=delay[j]


	os.chdir(mycwd)
	os.chdir(folder+"/RESULTS10"+str(k))

	line_counter=0

	final_path = "result.txt"
	#print final_path
	        
	try:
	    fp = open(final_path)
	    while line_counter != 100:
	        line = fp.readline()
	        if not line:
	            break
	        line_counter += 1
	        words = re.split('; |, |s | |\* |% |\t|\n', line)    
	        if(line_counter>1):                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
		        d1=(float(words[0]))
		        d1 = round(d1,0)
		        d1 = int (d1)
		        r = (int(words[1]))
		        m = (int(words[2]))
		        t = (int(words[3]))
		        p = (float(words[4]))
		        de=(float(words[5]))
		        
		        if(d1 > A and d1<D):
		        	
		        	if(d[0]==0):
		        		d[0]=d1
		        		pdr[0]=p
		        		repetition[0]=r
		        		delay[0]=de
		        		mcs[0] =m
		        		tone[0]=t

		        	else:
		        		d[0]=(d[0]+d1)/2
		        		pdr[0]=(pdr[0]+p)/2.0
		        		repetition[0]=(repetition[0]+r)/2
		        		delay[0]=(delay[0]+de)/2.0	 
		        		mcs[0]=(mcs[0]+m)/2   
		        		tone[0]=(tone[0]+t)/2    		
		        for i in range (1,len(l)):
		        	if(d1>l[i-1] and d1<l[i]):
		        		if(d[i]==0):
		        			d[i]=d1
		        			pdr[i]=p
		        			delay[i]=de
		        			repetition[i]=r
		        			tone[i]=t
		        			mcs[i]=m
		        		else:
		        			d[i]=(d[i]+d1)/2
		        			pdr[i]=(pdr[i]*float(k)+p)/float(k+1)
		        			#delay[i]=(delay[i]+de)/2.0
		        			delay[i]=(delay[i]*float(k)+de)/float(k+1)
		        			repetition[i]=(repetition[i]+r)/2
		        			mcs[i]=(mcs[i]+m)/2
		        			tone[i]=(tone[i]+t)/2

	except IOError:
	            print "Could not open file! Please close Excel!"
	            pass

print "DISTANCE"
print d
print "mcs"
print mcs
print "tone"
print tone
print "repetition"
print repetition
print "pdr"
print pdr
print "delay"
print delay

print "pdr stddev"
for i in range(0,w):
	print (stddev([row[i] for row in PDRMatrix]))

print "delay stddev"
for i in range(0,w):
	print (stddev([row[i] for row in delayMatrix]))