import os
import shutil
import fileinput
import sys
import re
import glob

### Parameters for simulation
NTIMES = 1 # Run sim for this many times
SIMTIME= "2.0"

#Run for different UE distances and save in different folders

width = ["100"];#0, 25, 50, 75, 100, 125, 150, 175, 200];
nodes = ["100"];

for w in range (0,len(width)):	
	for n in range(0,len(nodes)):
		os.system('mkdir '+ "W"+ width[w]+"N"+nodes[n])
		os.system("""./waf --run lena-simple-epc --command='%s  --buildingWidth="""+width[w]+""" --numberOfUeNodes="""+nodes[n]+""" --ueDistance="""+"100"+""" --simTime="""+SIMTIME+"""  '"""+"> cmd.txt")		
		for file in glob.glob("*.txt"):
			shutil.move(file,"W" + width[w] + "N" + nodes[n])


