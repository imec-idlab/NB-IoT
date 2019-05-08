import os
import shutil
import fileinput
import sys
import re
import glob
import time

### Parameters for simulation
NTIMES = 1# Run sim for this many times
SIMTIME= "40.0"
buildingWidth="100"
#Run for different UE distances and save in different folders
d1= ["50"] #"6","12","18","24","30","36","42",
UE= ["1"]
param= UE
for d in range (0,len(param)):
	os.system('rm -rf RESULTS'+param[d])
	os.system('mkdir RESULTS'+param[d])
	for i in range(0,NTIMES):


		print("""./waf --run lena-simple-epc --command='%s  --RngRun="""+str(i)+""" --buildingWidth="""+buildingWidth+""" --numberOfUeNodes="""+UE[d]+""" --ueDistance="""+d1[0]+""" --simTime="""+SIMTIME+"""  '""")
		start=time.time()
		os.system("""./waf --run lena-simple-epc --command='%s  --RngRun="""+str(i)+""" --buildingWidth="""+buildingWidth+""" --numberOfUeNodes="""+UE[d]+""" --ueDistance="""+d1[0]+""" --simTime="""+SIMTIME+"""  '"""+"> cmd.txt")
		end = time.time()
		print (end-start)
		os.system('mkdir RESULTS'+param[d]+"/RESULTS"+param[d]+str(i))
		for file in glob.glob("*.txt"):
			shutil.move(file,"RESULTS"+param[d]+"/RESULTS"+param[d]+str(i))
		for file in glob.glob("extractinfo.py"):
			shutil.copy(file,"RESULTS"+param[d]+"/RESULTS"+param[d]+str(i))



