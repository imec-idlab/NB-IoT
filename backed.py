
line_counter = 0
line_error = 0
nCounter = 0
for i in range(0,NRUNS+1):
	os.chdir("RESULTS100/RESULTS100"+str(i))	        
	try:
	    fp = open("result.txt")
	    while line_counter != 10000:
	        line = fp.readline()
	        if not line:
	            break
	        line_counter += 1
	        words = re.split('; |, |s | |\* |% |\t|\n', line)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
	        if line_counter != 1:
	            try:
	
	                #print d1               
	                r = (int(words[1]))
	                m = (int(words[2]))
	                t = (int(words[3]))    
	                p = (int(words[4]))
	                d1=(float(words[0]))
	                d1 = round(d1,0)
	                d1 = int (d1) 
	                print d1

	                if(d1 > 0):
	                	print d1
	                	print d[0]
	                	if(d[0]==0):
	                		d[0]=d1
	                	else:
	                		d[0]=(d[0]+d1)/2                
	                elif(d1<3000):
	                	if(d[1]==0):
	                		d[1]=d1
	                	else:
	                		d[1]=(d[1]+d1)/2
	                elif(d1<4000):
	                	if(d[2]==0):
	                		d[2]=d1
	                	else:
	                		d[2]=(d[2]+d1)/2
	                elif(d1<5000):
	                	if(d[3]==0):
	                		d[3]=d1
	                	else:
	                		d[3]=(d[3]+d1)/2
	                elif(d1<6000):
	                	if(d[4]==0):
	                		d[4]=d1
	                	else:
	                		d[4]=(d[4]+d1)/2	           
	                elif(d1<7000):
	                	if(d[5]==0):
	                		d[5]=d1
	                	else:
	                		d[5]=(d[5]+d1)/2
	                elif(d1<8000):
	                	if(d[6]==0):
	                		d[6]=d1
	                	else:
	                		d[6]=(d[6]+d1)/2
	                elif(d1<9000):
	                	if(d[7]==0):
	                		d[7]=d1
	                	else:
	                		d[7]=(d[7]+d1)/2
	                elif(d1<10000):
	                	if(d[8]==0):
	                		d[8]=d1
	                	else:
	                		d[8]=(d[8]+d1)/2 
	                elif(d1>10000):
	                	if(d[9]==0):
	                		d[9]=d1
	                	else:
	                		d[9]=(d[9]+d1)/2
	                

	            except ValueError:
	                line_error += 1
	        else:
	            continue  
	    print d
    #os.chdir(mycwd)
	except IOError:
	            print "Could not open file! Please close Excel!"
	            pass

	nCounter += 1
	os.chdir(mycwd)


