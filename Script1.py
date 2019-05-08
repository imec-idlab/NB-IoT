import os,errno
import shutil
import fileinput
import sys
import re
import glob
import statistics
import math
working_directory= os.path.join(os.getcwd()+"/RESULTS200m/RESULTS2000/")

def stddev(lst):
    """returns the standard deviation of lst"""
    mn = statistics.mean(lst)
    variance = sum([(e-mn)**2 for e in lst]) / len(lst)
    return math.sqrt(variance)

#print working_directory
NRUNS=4
command= "python extractinfo.py"
mycwd = os.getcwd()
for i in range(0,NRUNS+1):	
	os.chdir("RESULTS10/RESULTS10"+str(i))
	try:
		os.remove("extractinfo.py")
	except OSError:
		pass
	#os.system(command)
	os.chdir(mycwd)
	shutil.copy("extractinfo.py","RESULTS10/RESULTS10"+str(i))
