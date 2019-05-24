import numpy as np
import matplotlib.pyplot as plt
#import seaborn as sns

#sns.set_style("white")
from matplotlib import rc
from numpy import double
rc('mathtext', default='regular')

base_path = "/Volumes/DriveD/workspace/NB-IoT"

path_result = base_path + "/result"


def findthroughput(filename):
    
    data = {}
    with open(base_path + filename) as f:
        line = f.readline()
        rntiindex = line.split('\t').index("RNTI")
        rxbytesindex = line.split('\t').index("RxBytes")
        txbytesindex = line.split('\t').index("TxBytes")
        endtimeindex = line.split('\t').index("end")
        
        #print rntiindex, rxbytesindex, txbytesindex, endtimeindex
        for line in f: 
            if line.split('\t')[rntiindex] in data:
                data[(line.split('\t')[rntiindex])][0] += int(line.split('\t')[rxbytesindex])
                data[(line.split('\t')[rntiindex])][1] = float(line.split('\t')[endtimeindex])
                data[(line.split('\t')[rntiindex])][2] += int(line.split('\t')[txbytesindex])
            else:
                data.setdefault((line.split('\t')[rntiindex]),[])
                data[(line.split('\t')[rntiindex])].append(int(line.split('\t')[rxbytesindex]))
                data[(line.split('\t')[rntiindex])].append(float(line.split('\t')[endtimeindex]))   
                data[(line.split('\t')[rntiindex])].append(int(line.split('\t')[txbytesindex]))         
                
    #print data.values()[0]
    print "Throughtput Rx = " + str(float(data.values()[0][0]/float(1000*data.values()[0][1]))) + " kbps"
              
    print "Throughtput Tx = " + str(float(data.values()[0][2]/float(1000*data.values()[0][1]))) + " kbps"


def findoverhead():
    
    data = {}
    
    filename = "/UlRlcStats.txt"
    with open(base_path + filename) as f:
        line = f.readline()
        rntiindex = line.split('\t').index("RNTI")
        rlc_rxbytesindex = line.split('\t').index("RxBytes")
        rlc_txbytesindex = line.split('\t').index("TxBytes")
        endtimeindex = line.split('\t').index("end")
        
        #print rntiindex, rxbytesindex, txbytesindex, endtimeindex
        for line in f: 
            if line.split('\t')[rntiindex] in data:
                data[(line.split('\t')[rntiindex])][0] = float(line.split('\t')[endtimeindex])
                data[(line.split('\t')[rntiindex])][1] += int(line.split('\t')[rlc_rxbytesindex])
                data[(line.split('\t')[rntiindex])][2] += int(line.split('\t')[rlc_txbytesindex])
            else:
                data.setdefault((line.split('\t')[rntiindex]),[])
                data[(line.split('\t')[rntiindex])].append(float(line.split('\t')[endtimeindex]))
                data[(line.split('\t')[rntiindex])].append(int(line.split('\t')[rlc_rxbytesindex]))
                data[(line.split('\t')[rntiindex])].append(int(line.split('\t')[rlc_txbytesindex]))      
                data[(line.split('\t')[rntiindex])].append(0)
                data[(line.split('\t')[rntiindex])].append(0)
                data[(line.split('\t')[rntiindex])].append(0)
                data[(line.split('\t')[rntiindex])].append(0)
                data[(line.split('\t')[rntiindex])].append(0)
                
                
    filename = "/UlPdcpStats.txt"
    with open(base_path + filename) as f:
        line = f.readline()
        rntiindex = line.split('\t').index("RNTI")
        pdcp_rxbytesindex = line.split('\t').index("RxBytes")
        pdcp_txbytesindex = line.split('\t').index("TxBytes")
        
        #print rntiindex, rxbytesindex, txbytesindex, endtimeindex
        for line in f: 
            if line.split('\t')[rntiindex] in data:
                data[(line.split('\t')[rntiindex])][3] += int(line.split('\t')[pdcp_rxbytesindex])
                data[(line.split('\t')[rntiindex])][4] += int(line.split('\t')[pdcp_txbytesindex])

    
    filename = "/UlMacStats.txt"
    with open(base_path + filename) as f:
        line = f.readline()
        rntiindex = line.split('\t').index("RNTI")
        mac_bytesindex = line.split('\t').index("size")
        
        #print rntiindex, rxbytesindex, txbytesindex, endtimeindex
        for line in f: 
            if line.split('\t')[rntiindex] in data:
                data[(line.split('\t')[rntiindex])][5] += int(line.split('\t')[mac_bytesindex])



    filename = "/UlRxPhyStats.txt"
    with open(base_path + filename) as f:
        line = f.readline()
        rntiindex = line.split('\t').index("RNTI")
        phy_rxbytesindex = line.split('\t').index("size")
        
        #print rntiindex, rxbytesindex, txbytesindex, endtimeindex
        for line in f: 
            if line.split('\t')[rntiindex] in data:
                data[(line.split('\t')[rntiindex])][6] += int(line.split('\t')[phy_rxbytesindex])

                
    filename = "/UlTxPhyStats.txt"
    with open(base_path + filename) as f:
        line = f.readline()
        rntiindex = line.split('\t').index("RNTI")
        phy_txbytesindex = line.split('\t').index("size")
        
        #print rntiindex, rxbytesindex, txbytesindex, endtimeindex
        for line in f: 
            if line.split('\t')[rntiindex] in data:
                data[(line.split('\t')[rntiindex])][7] += int(line.split('\t')[phy_txbytesindex])
               
    
    return data
    
    
def main():
    target = open(path_result+"/parsed.log", 'w')
    
    x1 = []
    y1 = []
    
    x2 = []
    y2 = []
    
    c2 = []
    c3 = []
    
    z1 = []
    z2 = []
    
    with open(path_result + "/withPSM7.txt") as f:
        for line in f:
            if "m_total:" in line:
                z1.append(int(line.split(" ")[1]))
            if "Remaining energy in joules" in line:
                #print line.split(" ")
                y1.append(float(line.split(" ")[6] )) 
    
                x1.append(float(line.split(" ")[0] )) 
                c2.append(float(line.split(" ")[8]))
                
    with open(path_result + "/without7.txt") as f:
        for line in f:  
            if "m_total:" in line:
                z2.append(int(line.split(" ")[1]))
            if "Remaining energy in joules" in line:
                y2.append(float(line.split(" ")[6] )) 
                x2.append(float(line.split(" ")[0] ))
                c3.append(float(line.split(" ")[8]))
              
    #print x1, y1, x2, y2 
    diffenergy = y1[-1] - y2[-1]
    
    plt.plot(x1,y1,'r',x2,y2)
    plt.ylabel('Remaining Energy in joules')
    plt.xlabel('Time in Seconds')
    plt.title('Energy graph, PSM saves '+ str(diffenergy) + ' J in ' + str(x1[-1]) + ' seconds')
    plt.grid(True)
    
    plt.show()
    
    
    
    plt.plot(x1,c2,'r',x2,c3)
    plt.ylabel('Total Power')
    plt.xlabel('Time in Seconds')
    plt.title('Battery can support PSM enabled device for '+ str((float(float(x1[-1])/c2[-1])/3600)*37800)  + ' hours')
    plt.grid(True)
    
    #plt.show()
    
    plt.plot(x1,z1,'r',x2,z2)
    plt.ylabel('Total Power')
    plt.xlabel('Time in Seconds')
    plt.title('Energy graph, PSM saves '+ str(diffenergy) + ' J in ' + str(x1[-1]) + ' seconds')
    plt.grid(True)
    
    #plt.show()
    
    print "*******Uplink******"
    findthroughput("/UlPdcpStats.txt")
    print "*******Downlink*****"
    findthroughput("/DlPdcpStats.txt")
    
    
    print "*******Overhead (in bytes) *****"
    print findoverhead().values()[0]
    print "Rx diff RLC-PHY =" + str(findoverhead().values()[0][6] - findoverhead().values()[0][1])
    print "Tx diff RLC-PHY =" + str(findoverhead().values()[0][7] - findoverhead().values()[0][2])
    
    print "Rx diff RLC-PDCP =" + str(findoverhead().values()[0][3] - findoverhead().values()[0][1])
    print "Tx diff RLC-PDCP =" + str(findoverhead().values()[0][4] - findoverhead().values()[0][2])
    

    

if(__name__ == "__main__"):
    main()
