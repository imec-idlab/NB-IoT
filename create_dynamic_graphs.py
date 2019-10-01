import tkinter as tk                # python 3
from tkinter import font  as tkfont # python 3
#import Tkinter as tk     # python 2
#import tkFont as tkfont  # python 2
from tkinter import *
from tkinter import ttk
import subprocess
import threading
import os
import time
import ctypes
import multiprocessing
import os.path
#from PIL import Image, ImageTk


from matplotlib import style
import matplotlib
matplotlib.use ('TkAgg')
import math
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import matplotlib.animation as animation
from matplotlib import style
from matplotlib import pyplot as plt

xList1 = []
yList1 = []
zList1 = []
yList1.append(0)
xList1.append(0)
zList1.append(0)

xList2 = []
yList2 = []
zList2 = []
yList2.append(0)
xList2.append(0)
zList2.append(0)

xList3 = []
yList3 = []
zList3 = []
yList3.append(0)
xList3.append(0)
zList3.append(0)
processList = []

style.use("ggplot")

f = Figure()
a = f.add_subplot(111) # how many charts

f1 = Figure()
b = f1.add_subplot(111) # how many charts

f2 = Figure()
plt = f2.add_subplot(111) # how many charts

NORM_FONT = ("Verdana",10)
SMALL_FONT = ("Verdana",8)
LAST_I = 10

def stop_simulation():
    global processList
    for j in processList:
        print(j)
        print ('j.is_alive()', j.is_alive())

        j.terminate()

    
    '''cmd = "pkill -n waf"
    ps = subprocess.Popen(cmd,shell=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    output = ps.communicate()[0]
    print ("PS: " + str(output))'''

def latencygraph():
    exit()


def execute_p(t3412,t3324,ptw,dli,uli,nn,rrc,ps,rand,rt):
    global xList1
    global yList1
    global zList1
    global xList2
    global yList2
    global zList2
    global xList3
    global yList3
    global zList3
    
    print(t3412,t3324,ptw)
    
    cmd1 = "/Applications/MATLAB_R2018a.app/bin/matlab -nodisplay -nosplash -nodesktop -r"+" 'cal_delay(0,64,"+ str(rrc)+","+str(t3324)+","+str(t3412) +"," + str(ptw) +","+ str(uli)  +","+ str(dli) +","+ str(rt)+",100,1);exit(0);'"
    print (cmd1)
    os.system(cmd1)
    time.sleep(4)
    
 
    
    os.system("touch sampleData.txt")
    time.sleep(0.5)
    xList1 = [0]
    yList1= [0]
    zList1= [0]
    xList2= [0]
    yList2= [0]
    zList2= [0]
    xList3= [0]
    yList3= [0]
    zList3= [0]
    cmd = './waf --run "lena-simple-epc-1 -simTime=' + str(rt) +' -interPacketIntervaldl=' + str(dli) +' -interPacketIntervalul=' + str(uli) +' -numberOfNodes=' + str(nn) +' -t3324='+str(t3324) + ' -t3412='+str(t3412) + ' -rrc_release_timer=' + str(rrc) + ' -edrx_cycle=' + str(ptw) +' -packetsize=' + str(ps) +' -EnableRandom=' + str(rand) +'" > sampleData.txt'
    os.system(cmd)



def calculate_s(dataList):

    psmval      = {}
    count_state = {}
    ue_data = {}
    ue_data_end = {}
    psm_paging = {}
    latency_ul = {}
    rnti = 0
    last = ""
    for text in dataList:
        if "LteUeRrc::SwitchToState" in text and "--> CONNECTED_NORMALLY" in text:
            rnti = text.split(" ")[1]
            index = 4  # 1: PSM, 2: DRX, 3: PAging, 4: Connected, 5: DL Sent, 6 DL received, 7: UL Sent, 8: UL Received
            time = text.split(" ")[0]
            if rnti not in ue_data:
                ue_data.setdefault(rnti,{})
                ue_data[str(rnti)]['1'] = []
                ue_data[str(rnti)]['2'] = []
                ue_data[str(rnti)]['3'] = []
                ue_data[str(rnti)]['4'] = []
                ue_data[str(rnti)]['5'] = []
                ue_data[str(rnti)]['6'] = []
                ue_data[str(rnti)]['7'] = []
                ue_data[str(rnti)]['8'] = []
                ue_data[str(rnti)]['4'].append(float(time)) # Time of that state in seconds
            
            if rnti not in ue_data_end:
                ue_data_end.setdefault(rnti,{})
                ue_data_end[str(rnti)]['1'] = []
                ue_data_end[str(rnti)]['2'] = []
                ue_data_end[str(rnti)]['3'] = []
                ue_data_end[str(rnti)]['4'] = []
                ue_data_end[str(rnti)]['5'] = []
                ue_data_end[str(rnti)]['6'] = []
                ue_data_end[str(rnti)]['7'] = []
                ue_data_end[str(rnti)]['8'] = []
                    
                    
            else:
                ue_data[str(rnti)]['4'].append(float(time)) # Time of that state in seconds
            last = "CONNECTED"

        elif "LteUeRrc::SwitchToState from IDLE_PSM --> SUSPEND_PAGING" in text:
            rnti = text.split(" ")[1]
            index = 1  # 1: PSM, 2: DRX, 3: PAging, 4: Connected, 5: DL Sent, 6 DL received, 7: UL Sent, 8: UL Received
            time = text.split(" ")[0]
            if rnti not in psm_paging:
                psm_paging.setdefault(rnti,{})
                psm_paging[str(rnti)][str(index)] = []
                psm_paging[str(rnti)][str(index)].append(float(time))
            else:
                psm_paging[str(rnti)][str(index)].append(float(time))
            last = "PAGING"
            
        elif "LteUeRrc::SwitchToState" in text and "--> IDLE_SUSPEND" in text:
            rnti = text.split(" ")[1]
            index = 2  # 1: PSM, 2: DRX, 3: Paging, 4: Connected, 5: DL Sent, 6 DL received, 7: UL Sent, 8: UL Received
            time = text.split(" ")[0]
            ue_data[str(rnti)][str(index)].append(float(time)) # Time of that state in seconds
            last = "eDRX"

        elif "LteUeRrc::SwitchToState" in text and "--> SUSPEND_PAGING" in text:
            rnti = text.split(" ")[1]
            index = 3  # 1: PSM, 2: DRX, 3: Paging, 4: Connected, 5: DL Sent, 6 DL received, 7: UL Sent, 8: UL Received
            time = text.split(" ")[0]
            ue_data[str(rnti)][str(index)].append(float(time)) # Time of that state in seconds
            last = "PAGING"

        elif "LteUeRrc::SwitchToState" in text and "--> IDLE_PSM" in text:
            rnti = text.split(" ")[1]
            index = 1  # 1: PSM, 2: DRX, 3: Paging, 4: Connected, 5: DL Sent, 6 DL received, 7: UL Sent, 8: UL Received
            time = text.split(" ")[0]
            ue_data[str(rnti)][str(index)].append(float(time)) # Time of that state in seconds
            last = "PSM"
        
        elif "LteEnbNetDevice::Send" in text:
            rnti = text.split(" ")[1]
            index = 5  # 1: PSM, 2: DRX, 3: Paging, 4: Connected, 5: DL Sent, 6 DL received, 7: UL Sent, 8: UL Received
            time = text.split(" ")[0]
            ue_data[str(rnti)][str(index)].append(float(time)) # Time of that state in seconds
            
        elif "LteUeRrc::DoReceivePdcpSdu" in text:
            rnti = text.split(" ")[1]
            index = 6  # 1: PSM, 2: DRX, 3: Paging, 4: Connected, 5: DL Sent, 6 DL received, 7: UL Sent, 8: UL Received
            time = text.split(" ")[0]
            ue_data[str(rnti)][str(index)].append(float(time)) # Time of that state in seconds
    
        elif "LteUeRrc::DoSendData" in text:
            rnti = text.split(" ")[1]
            index = 7  # 1: PSM, 2: DRX, 3: Paging, 4: Connected, 5: DL Sent, 6 DL received, 7: UL Sent, 8: UL Received
            time = text.split(" ")[0]
            ue_data[str(rnti)][str(index)].append(float(time)) # Time of that state in seconds
            
        elif "eNb_UeManager::DoReceivePdcpSdu" in text:
            rnti = text.split(" ")[1]
            index = 8  # 1: PSM, 2: DRX, 3: Paging, 4: Connected, 5: DL Sent, 6 DL received, 7: UL Sent, 8: UL Received
            time = text.split(" ")[0]
            ue_data[str(rnti)][str(index)].append(float(time)) # Time of that state in seconds

        elif "m_total" in text:
            runtime = float(text.split(" ")[6])/1000000000
                    

        if "LteUeRrc::SwitchToState" in text and "IDLE_PSM -->" in text:
            rnti = text.split(" ")[1]
            index = 1  # 1: PSM, 2: DRX, 3: Paging, 4: Connected, 5: DL Sent, 6 DL received, 7: UL Sent, 8: UL Received
            time = text.split(" ")[0]
            ue_data_end[str(rnti)][str(index)].append(float(time)) # Time of that state in seconds

        if "LteUeRrc::SwitchToState" in text and "IDLE_SUSPEND -->" in text:
            rnti = text.split(" ")[1]
            index = 2  # 1: PSM, 2: DRX, 3: Paging, 4: Connected, 5: DL Sent, 6 DL received, 7: UL Sent, 8: UL Received
            time = text.split(" ")[0]
            ue_data_end[str(rnti)][str(index)].append(float(time)) # Time of that state in seconds

        if "LteUeRrc::SwitchToState" in text and "CONNECTED_NORMALLY -->" in text:
            rnti = text.split(" ")[1]
            index = 4  # 1: PSM, 2: DRX, 3: PAging, 4: Connected, 5: DL Sent, 6 DL received, 7: UL Sent, 8: UL Received
            time = text.split(" ")[0]
            ue_data_end[str(rnti)][str(index)].append(float(time)) # Time of that state in seconds
#print (rnti,ue_data,ue_data_end,psm_paging)
    return (ue_data,ue_data_end,psm_paging,last)

def calculate_l(dataList):
    puid = {}
    latency_ul = {}
    latency = {}
    edrx_cycle = 0
    lastpsm = 0
    lastsuspend = 0
    lastsuspend_d = 0
    lastpsm_d = 0
    t3412 = 0
    t3324 = 0
    time = 0
    for text in dataList:
        if "--> IDLE_PSM" in text:
            lastpsm_d = float(text.split(" ")[0]) + t3412
            lastpsm = float(text.split(" ")[0])
                #print lastpsm
        if "--> IDLE_SUSPEND" in text:
            lastsuspend_d = float(text.split(" ")[0]) + edrx_cycle
            lastsuspend = float(text.split(" ")[0])
        if "EnergyConsumed" in text:
            time = text.split(" ")[0] #Not used
        if "LteEnbNetDevice::Send" in text:
            #print text
            dltime = float(text.split(" ")[0])
            rnti   = text.split(" ")[1]
            dldata = float(text.split(" ")[3])
            dluid  = text.split(" ")[5]
            if lastsuspend < lastpsm:
                decr = lastpsm_d
            else:
                decr = lastsuspend_d
            
            if rnti not in latency:
                latency.setdefault(rnti,{})
                puid.setdefault(rnti,{})
                puid[str(rnti)]['uid'] = []
            
            
            if dluid not in latency[str(rnti)]:
                if dltime < decr:
                    latency[str(rnti)][str(dluid)] = [dltime,dldata,1,decr,0,0]
                else:
                    latency[str(rnti)][str(dluid)] = [dltime,dldata,1,0,0,0]
                #sendinT,senddata,sendcounter,reciT,recd,recc
                puid[str(rnti)]['uid'].append(dluid)
            #print dltime,t3412,decr,decr
            
            else:
                latency[str(rnti)][str(dluid)][0] = dltime
                latency[str(rnti)][str(dluid)][1] = dldata
                latency[str(rnti)][str(dluid)][2] = latency[str(rnti)][str(dluid)][2] + 1
                #print dltime,t3412,decr,decr
                latency[str(rnti)][str(dluid)][3] = decr
                puid[str(rnti)]['uid'].append(dluid)
                
                
                    
        elif "LteUeRrc::DoReceivePdcpSdu" in text:
            #print text
            dlrtime = float(text.split(" ")[0])
            dlrrnti   = text.split(" ")[1]
            dlrdata = float(text.split(" ")[4])
            dlruid  = text.split(" ")[6]
            latency[str(dlrrnti)][str(dlruid)][3] = dlrtime
            latency[str(dlrrnti)][str(dlruid)][4] = dlrdata
            latency[str(dlrrnti)][str(dlruid)][5] = latency[str(dlrrnti)][str(dlruid)][5] + 1
            
            '''print (puid[str(dlrrnti)]['uid'])
                print int(dlruid)
                print "BELOW"'''
            i = 0
            while i < len(puid[str(dlrrnti)]['uid']):
                #print puid[str(dlrrnti)]['uid'][i]
                #print dlruid
                if int(puid[str(dlrrnti)]['uid'][i]) < int(dlruid):
                    #print "IF"
                    latency[str(dlrrnti)][puid[str(dlrrnti)]['uid'][i]][3] = 0
                    puid[str(dlrrnti)]['uid'].remove(puid[str(dlrrnti)]['uid'][i])
                if int(puid[str(dlrrnti)]['uid'][i]) == int(dlruid):
                    #print "ELIF"
                    puid[str(dlrrnti)]['uid'].remove(puid[str(dlrrnti)]['uid'][i])
                i = i + 1
                                                    
                                                    
        elif "LteUeRrc::DoSendData" in text:
            ultime = float(text.split(" ")[0])
            ulrnti   = text.split(" ")[1]
            uldata = float(text.split(" ")[4])
            uluid  = text.split(" ")[6]
            if ulrnti not in latency_ul:
                latency_ul.setdefault(ulrnti,{})
            if uluid not in latency_ul[str(ulrnti)]:
                latency_ul[str(ulrnti)][str(uluid)] = [ultime,uldata,1,0,0,0]
                    
            else:
                latency_ul[str(ulrnti)][str(uluid)][0] = ultime
                latency_ul[str(ulrnti)][str(uluid)][1] = uldata
                latency_ul[str(ulrnti)][str(uluid)][2] = latency_ul[str(ulrnti)][str(uluid)][2] + 1
                                                                                
        elif "eNb_UeManager::DoReceivePdcpSdu" in text:
            ulrtime = float(text.split(" ")[0])
            ulrrnti   = text.split(" ")[1]
            ulrdata = float(text.split(" ")[4])
            ulruid  = text.split(" ")[6]
            
            latency_ul[str(ulrrnti)][str(ulruid)][3] = ulrtime
            latency_ul[str(ulrrnti)][str(ulruid)][4] = ulrdata
            latency_ul[str(ulrrnti)][str(ulruid)][5] = latency_ul[str(ulrrnti)][str(ulruid)][5] + 1

    count = 0
    dlcounter = -1 #To ignore the first packet which is always received in 0.086 seconds
    dllatency = 0
    dlmissed_s = 0
    dlmissed_r = 0
    dldata_s  = 0
    dldata_r  = 0
    for i in latency.keys():
        dlcounter = -1 #To ignore the first packet which is always received in 0.086 seconds
        dllatency = 0
        dlmissed_s = 0
        dlmissed_r = 0
        dldata_s  = 0
        dldata_r  = 0
        for j in latency[str(i)].keys():
            if float(latency[str(i)][j][2]) !=0 and float(latency[str(i)][j][3]) ==0:
                dlmissed_s = dlmissed_s + 1
            if float(latency[str(i)][j][5]) !=0 and float(latency[str(i)][j][0]) ==0:
                dlmissed_r = dlmissed_r + 1
            if float(latency[str(i)][j][2]) !=0 and float(latency[str(i)][j][3]) !=0 and float(latency[str(i)][j][0]) !=0:
                dlcounter = dlcounter + 1
                dllatency = dllatency + float(latency[str(i)][j][3]) - float(latency[str(i)][j][0])
                #print j, float(latency[str(i)][j][3]) - float(latency[str(i)][j][0]), dlcounter
                dldata_s = dldata_s + float(latency[str(i)][j][1])
                dldata_r = dldata_r + float(latency[str(i)][j][4])
        count = count + 1


    count1 = 0
    ulcounter = 0
    ullatency = 0
    ulmissed_s = 0
    ulmissed_r = 0
    uldata_s  = 0
    uldata_r  = 0
    for i in latency_ul.keys():
        ulcounter = 0
        ullatency = 0
        ulmissed_s = 0
        ulmissed_r = 0
        uldata_s  = 0
        uldata_r  = 0
        #print i
        for j in latency_ul[str(i)].keys():
            if float(latency_ul[str(i)][j][2]) !=0 and float(latency_ul[str(i)][j][3]) ==0:
                ulmissed_s = ulmissed_s + 1
            if float(latency_ul[str(i)][j][5]) !=0 and float(latency_ul[str(i)][j][0]) ==0:
                ulmissed_r = ulmissed_r + 1
            if float(latency_ul[str(i)][j][2]) !=0 and float(latency_ul[str(i)][j][3]) !=0 and float(latency_ul[str(i)][j][0]) !=0:
                ulcounter = ulcounter + 1
                ullatency = ullatency + float(latency_ul[str(i)][j][3]) - float(latency_ul[str(i)][j][0])
                uldata_s = uldata_s + float(latency_ul[str(i)][j][1])
                uldata_r = uldata_r + float(latency_ul[str(i)][j][4])
    count1 = count1 + 1
    if ulcounter == 0:
        ull = 0
    else:
        ull= float(ullatency/ulcounter)

    if dlcounter == 0:
        dll = 0
    else:
        dll= float(dllatency/dlcounter)

#print (ull,dll,time)
    return  (round(ull,3),round(dll,3),round(float(time),2))



def calculate_e(dataList):
    count_state = {}
    ue_data = {}
    latency = {}
    latency_ul = {}
    psmval      = {}
    t3412 = 0
    edrx_cycle = 0
    t3324 = 0
    time = 0
    tenergy = 0
    for text in dataList:
        if len(text)  > 1:
            if "m_total" in text:
                
                rnti = text.split(" ")[1]
                index = text.split(" ")[3]
                runtime = float(text.split(" ")[0])
                '''energy_value[index] = text.split(" ")[4]'''
                
                if rnti not in ue_data:
                    ue_data.setdefault(rnti,{})
                    count_state.setdefault(rnti,{})
                    
                    if rnti not in psmval:
                        psmval.setdefault(rnti,{})
                        psmval[str(rnti)]['totalenergy'] = []
                        psmval[str(rnti)]['latency_dl'] = []
                        psmval[str(rnti)]['latency_ul'] = []
                    
                    ue_data[str(rnti)][str(index)] = float(text.split(" ")[4])/1000000000 # Time of that state in seconds
                    count_state[str(rnti)][str(index)] = 1    #Counter of that state
                else:
                    if index not in ue_data[(rnti)]:
                        ue_data[(rnti)][str(index)] = float(text.split(" ")[4])/1000000000
                        count_state[str(rnti)][str(index)] = 1
                    else:
                        ue_data[(rnti)][str(index)] = float(ue_data[(rnti)][str(index)]) + (float(text.split(" ")[4])/1000000000)
                        count_state[str(rnti)][str(index)] = int(count_state[str(rnti)][str(index)]) + 1
            if "EnergyConsumed" in text:
                time = text.split(" ")[0] #Not used
#print (time,ue_data)

    ''' Fill the empty entries'''
    i = 1
    while i <= len(ue_data.keys()):  #Assume RNTI assigned is always continous
        j = 1
        if 7 != len(ue_data[str(i)].keys()):   # All state count = 7
            while j <=7:
                if str(j) not in ue_data[str(i)].keys():
                    ue_data[str(i)][str(j)] = 0
                    count_state[str(i)][str(j)] = 0
                    
                j = j + 1
        i = i + 1
    
    voltage = 3.6
    assumed_transfer_time = 0.000928570
    energy_psm = 0.000000003*voltage      #1
    energy_drx = 0.006*voltage      #2
    energy_idle = 0.0088*voltage     #3
    
    energy_ul_tx = 0.022*assumed_transfer_time*voltage    #4
    energy_dl_tx = 0.046*assumed_transfer_time*voltage   #5
    energy_ul_dl_tx = energy_dl_tx #6
    energy_paging = 0*voltage       #7
    
    #print count_state
    #print count_state[str(1)][str(7)]
    
    i = 1
    while i <= len(ue_data.keys()) :   # Number of UEs
        tenergy = energy_psm*ue_data[str(i)][str(1)] + \
            energy_drx*ue_data[str(i)][str(2)] + \
            energy_idle*ue_data[str(i)][str(3)] + \
            energy_idle*ue_data[str(i)][str(4)] + \
            energy_idle*ue_data[str(i)][str(5)] + \
            energy_idle*ue_data[str(i)][str(6)] + \
            energy_idle*ue_data[str(i)][str(7)] + \
            energy_ul_tx*count_state[str(i)][str(4)] + \
            energy_dl_tx*count_state[str(i)][str(5)] + \
            energy_ul_dl_tx*count_state[str(i)][str(6)] + \
            energy_paging*count_state[str(i)][str(7)]
        i = i + 1
    #print (tenergy, time)
    return (round(float(time),2), round(float(tenergy),2))




def popupmsg(msg):
    popup = tk.Tk()
    def f1():
        popup.destroy()
    popup.wm_title("!!")
    label = ttk.Label(popup,text=msg,font=NORM_FONT)
    label.pack(side="top",fill="x",pady=30)
    b1 = ttk.Button(popup,text="Okay",command= f1)
    b1.pack()
    
    popup.mainloop()





def animate3(i):
    if os.path.isfile("sampleData.txt") == False:
        os.system("touch sampleData.txt")
    
    
    pullData = open("sampleData.txt","r").read()
    dataList = pullData.split('\n')
    global xList3
    global yList3
    global zList3
    
    ue_data,ue_data_end,psm_paging,last = calculate_s(dataList)

    plt.clear()
    #print ("psm_paging: ")
    #print ( len(psm_paging), len(ue_data) , len(ue_data_end) )
    #print ("psm_paging ends ")
    for rnti in ue_data.keys():
        if len(psm_paging) > 0:
            while len(psm_paging[str(rnti)]['1']) > LAST_I*2:
                psm_paging[str(rnti)]['1'].pop(0)
            #print (psm_paging[str(rnti)],len(psm_paging[str(rnti)]),len(psm_paging[str(rnti)]['1']),(ue_data[str(rnti)]['1'][0] - psm_paging[str(rnti)]['1'][0]))
            if len(psm_paging[str(rnti)]['1']) > 0 and len(ue_data[str(rnti)]['1']) > 0:
                while ( (ue_data[str(rnti)]['1'][0] - psm_paging[str(rnti)]['1'][0]) < 0) and (len(psm_paging[str(rnti)]['1'])>0):
                    psm_paging[str(rnti)]['1'].pop(0)
                    if len(psm_paging[str(rnti)]['1']) == 0:
                        break;
            #print (psm_paging[str(rnti)])

        for j in range(1,9):
            #print ("b" + str(len(ue_data_end[str(rnti)][str(j)])) + " " + str(len(ue_data[str(rnti)][str(j)])))

            while len(ue_data[str(rnti)][str(j)]) > LAST_I*2:
                ue_data[str(rnti)][str(j)].pop(0)
            while len(ue_data_end[str(rnti)][str(j)]) > LAST_I*2:
                ue_data_end[str(rnti)][str(j)].pop(0)
            #print ("a"+ str(len(ue_data_end[str(rnti)][str(j)])) + " " + str(len(ue_data[str(rnti)][str(j)])))
            if len(ue_data[str(rnti)]['2']) > 0 and len(ue_data[str(rnti)]['1']) > 0:
                if (ue_data[str(rnti)]['2'][0] - ue_data[str(rnti)]['1'][0]) > 700 and len(ue_data[str(rnti)]['1'])>2 and len(ue_data_end[str(rnti)]['1'])>2 :
                    ue_data_end[str(rnti)][str(1)].pop(0)
                    ue_data[str(rnti)][str(1)].pop(0)

            if len(ue_data[str(rnti)]['2']) > 0 and len(ue_data[str(rnti)]['4']) > 0:
                if (ue_data[str(rnti)]['2'][0] - ue_data[str(rnti)]['4'][0]) > 700 and len(ue_data[str(rnti)]['4'])>2 and len(ue_data_end[str(rnti)]['4'])>2:
                    ue_data_end[str(rnti)][str(4)].pop(0)
                    ue_data[str(rnti)][str(4)].pop(0)
            
                        
        
        #print (ue_data[str(rnti)]['1'][0], ue_data[str(rnti)]['2'][0], ue_data[str(rnti)]['4'][0],ue_data[str(rnti)]['3'][0],ue_data[str(rnti)]['5'][0],ue_data[str(rnti)]['6'][0] )

    #print (ue_data_end)

    for rnti in ue_data.keys():
        #plt.plot(ue_data[str(rnti)]['1'], [1] * len(ue_data[str(rnti)]['1']),'r|' )
        if len(psm_paging) > 0:
            plt.plot(psm_paging[str(rnti)]['1'], [1] * len(psm_paging[str(rnti)]['1']),'g|' )
        
        #plt.plot(ue_data[str(rnti)]['2'], [2] * len(ue_data[str(rnti)]['2']),'b|')
        plt.plot(ue_data[str(rnti)]['3'], [2] * len(ue_data[str(rnti)]['3']),'g|' )
        #plt.plot(ue_data[str(rnti)]['4'], [4] * len(ue_data[str(rnti)]['4']),'c|' )
        plt.plot(ue_data[str(rnti)]['5'], [4] * len(ue_data[str(rnti)]['5']),'c|' )
        plt.plot(ue_data[str(rnti)]['6'], [4] * len(ue_data[str(rnti)]['6']),'k4' )
        
        plt.plot(ue_data[str(rnti)]['7'], [5] * len(ue_data[str(rnti)]['7']),'k|' )
        plt.plot(ue_data[str(rnti)]['8'], [5] * len(ue_data[str(rnti)]['8']),'c4' )
        
        #plt.plot(ue_data_end[str(rnti)]['1'], [1] * len(ue_data_end[str(rnti)]['1']),'r|' )
        #plt.plot(ue_data_end[str(rnti)]['2'], [2] * len(ue_data_end[str(rnti)]['2']),'b|' )
        #plt.plot(ue_data_end[str(rnti)]['4'], [4] * len(ue_data_end[str(rnti)]['4']),'b|' )
        
        #Create line between 2 points
        for i in range(0, len(ue_data_end[str(rnti)]['1'])):
            plt.plot([ ue_data[str(rnti)]['1'][i], ue_data_end[str(rnti)]['1'][i]], [1,1], 'r-')
        
        for i in range(0, len(ue_data_end[str(rnti)]['4'])):
            plt.plot([ ue_data[str(rnti)]['4'][i], ue_data_end[str(rnti)]['4'][i]], [3,3], 'm-')
        
        for i in range(0, len(ue_data_end[str(rnti)]['2'])):
            plt.plot([ ue_data[str(rnti)]['2'][i], ue_data_end[str(rnti)]['2'][i]], [2,2], 'b-')

        for i in range(0, len(ue_data[str(rnti)]['6'])):
            plt.plot([ ue_data[str(rnti)]['5'][i], ue_data[str(rnti)]['6'][i]], [4,4], 'c-')

        for i in range(0, len(ue_data[str(rnti)]['8'])):
            plt.plot([ ue_data[str(rnti)]['7'][i], ue_data[str(rnti)]['8'][i]], [5,5], 'c-')

    plt.set_title("1:PSM, 2:eDRX, 3:Connected, 4:DL Data, 5:UL Data \n NEW state: "+last)
                
        #plt.grid(True)
        #plt.gca().xaxis.grid(True)
        #plt.yticks([])
    plt.set_yticks([1,2,3,4,5,6], ['PSM', 'DRX', 'Connected', 'DL Data', 'UL Data'])
    plt.set_xlabel('Time (Seconds)')
    plt.set_ylabel('State')
#plt.xticks(np.arange(0, runtime+1, 50))
        


    
    #b.clear()
    #b.plot(xList,yList,label="DL Latency")
    #b.plot(xList,zList,label="UL Latency")



def animate2(i):
    if os.path.isfile("sampleData.txt") == False:
        os.system("touch sampleData.txt")
    
    
    pullData = open("sampleData.txt","r").read()
    dataList = pullData.split('\n')
    global xList2
    global yList2
    global zList2
    dldelaylist = [0]
 
    
    ul,dl,t = calculate_l(dataList)
    if os.path.isfile('td0.txt') == True:
        with open("td0.txt") as f1:
            line = f1.readline()
            text = line.strip()
            while line:
                if "AverageDLdelay:" in text:
                    if text.split(" ")[1] is "NAN":
                        val1 = -1
                    else:
                        val1 = float(text.split(" ")[1])
                    if val1 != dldelaylist[-1]:
                        dldelaylist = [val1]
            
                line = f1.readline()
        f1.close()


    #print (xList2,yList2,zList2)
    while len(dldelaylist) != len(xList2):
        dldelaylist.append(dldelaylist[-1])

    if len(xList2) > LAST_I :
        if xList2[LAST_I-1] != t:
            xList2.pop(0)
            yList2.pop(0)
            zList2.pop(0)
            dldelaylist.pop(0)

    else:
        xList2.append(t)
        yList2.append(dl)
        zList2.append(ul)
        dldelaylist.append(dldelaylist[-1])

    b.clear()
    b.plot(xList2,yList2,label="DL Latency")
    b.plot(xList2,dldelaylist,label="Average DL Latency")
    b.plot(xList2,zList2,label="UL Latency")
    #a.legend(bbox_to_anchor=(0,1,1,0),loc=3,ncol=2)
    b.legend()
    b.set_title("Latency vs Time \n DL latency:" + str(round(yList2[-1],3)) + "s  UL latency:" + str(round(zList2[-1],3)) + "s")
    b.set_xlabel('Time (Seconds)')
    b.set_ylabel('Latency (Seconds)')

def animate(i):
    if os.path.isfile("sampleData.txt") == False:
        os.system("touch sampleData.txt")
    
    
    pullData = open("sampleData.txt","r").read()
    dataList = pullData.split('\n')
    global xList1
    global yList1
    global zList1
    energylist = [0]
   
    
    dtime, dener = calculate_e(dataList)
    if os.path.isfile('td0.txt') == True:
        with open("td0.txt") as f1:
            line = f1.readline()
            text = line.strip()
            while line:
                if "AverageEnergy" in text:
                    if text.split(" ")[1] is "NAN":
                        val2 = -1
                    else:
                        val2 = float(text.split(" ")[1])
                        print (val2)
                    if val2 != energylist[-1]:
                        energylist = [val1]
                line = f1.readline()
        f1.close()
    
    
    #print ("dtime, dener")
    #print (xList1,yList1,zList1)
    
    if  dtime==0:
        av = 0
    else:
        av = round(float(dener)/float(dtime),3)

    while len(energylist) != len(xList1):
        energylist.append(energylist[-1])
    
    if len(xList1) > LAST_I :
        if dtime != xList1[LAST_I-1]:
            xList1.pop(0)
            yList1.pop(0)
            zList1.pop(0)
            energylist.pop(0)
        #else:
        #    if av !=0:
        #        popupmsg("SIMULATION FINISHED!!! \n \n \n \n ReConfigure or Exit \n \n \n Thanks !!!")

    
    else:
        xList1.append(dtime)
        yList1.append(dener)
        zList1.append(av)
        energylist.append(energylist[-1])

    #print (len(xList1),len(yList1),len(zList1))



    a.clear()
    a.plot(xList1,yList1,label="Total Energy")
    #a.plot(xList1,energylist,label="Model Avg. Energy")
    a.plot(xList1,zList1,label="Avg. Energy")
    #a.legend(bbox_to_anchor=(0,1,1,0),loc=3,ncol=2)
    a.legend()
    a.set_title("Time vs parameters \n Total energy: " + str(round(yList1[-1],3)) + "J  Avg energy: " + str(round(zList1[-1],3)) + "J/s" )
    a.set_xlabel('Time (Seconds)')
    a.set_ylabel('Energy (Joule)')

class SampleApp(tk.Tk):

    def __init__(self, *args, **kwargs):
        tk.Tk.__init__(self, *args, **kwargs)
        
        #tk.Tk.iconbitmap(self, default="imec.ico")
        img = tk.Image("photo", file='imec.png')
        self.tk.call('wm', 'iconphoto', self._w, img)
        self.title_font = tkfont.Font(family='Verdana', size=18)

        # the container is where we'll stack a bunch of frames
        # on top of each other, then the one we want visible
        # will be raised above the others
        #root = Tk()
        
        container = ttk.Frame(self)#, padding="30 30 120 120") # size of the GUI grid
        #container.grid(column=0, row=0, sticky=(N, W, E, S))
        container.pack(side="top",fill="both",expand=1)
        self.columnconfigure(0, weight=1)
        self.rowconfigure(0, weight=1)
        self.title("Simulator Analysis")
        #container = tk.Frame(self, padding="30 30 120 120")
        #container.pack(side="top", fill="both", expand=True)
        #container.grid_rowconfigure(0, weight=1)
        #container.grid_columnconfigure(0, weight=1)
        
        menubar = tk.Menu(container)
        filemenu = tk.Menu(menubar,tearoff=0)
        filemenu.add_command(label="Save settings", command=lambda: popupmsg("Not supported"))
        filemenu.add_separator()
        filemenu.add_command(label="Exit",command=quit)
        menubar.add_cascade(label="File",menu=filemenu)
        
        
        exchoice = tk.Menu(menubar,tearoff=1)
        exchoice.add_command(label="Energy", command=lambda: energygraph())
        exchoice.add_command(label="Latency", command=lambda: latencygraph())
        exchoice.add_separator()
        exchoice.add_command(label="Exit",command=quit)
        menubar.add_cascade(label="Select",menu=exchoice)
        
        tk.Tk.config(self,menu=menubar)
        
        
        self.frames = {}
        for F in (StartPage, PageOne, PageTwo, PageThree):
            F_name = F.__name__
            frame = F(parent=container, controller=self)
            self.frames[F_name] = frame

            # put all of the pages in the same location;
            # the one on the top of the stacking order
            # will be the one that is visible.
            frame.grid(row=0, column=0, sticky="nsew")

        self.show_frame("StartPage")

    def show_frame(self, page_name):
        '''Show a frame for the given page name'''
        frame = self.frames[page_name]
        frame.tkraise()

    def calculate(*args):
    
        MyOut = subprocess.Popen(['ls', '-l', '.'],
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT)
        stdout,stderr = MyOut.communicate()
        print(stdout)
        print(stderr)
        print (args[1])


class StartPage(tk.Frame):

    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        self.controller = controller
        label = ttk.Label(self, text="Welcome to MAGICIAN Demo", font=controller.title_font).grid(row=5, column=1600,padx=200, pady=20)
        label = ttk.Label(self, text="Analysis of Latency and Energy consumption", font=controller.title_font).grid(row=6, column=1600, pady=50)

        #label.pack(padx=10, pady=10)

        #button1 = ttk.Button(self, text="Go to Page One", command=lambda: controller.show_frame("PageOne")).grid(row=20, column=800)
        button2 = ttk.Button(self, text="Run Program",
                            command=lambda: controller.show_frame("PageTwo")).grid(row=22, column=1600)
        button4 = ttk.Button(self, text="Exit",
                             command=quit).grid(row=24, column=1600, pady=20)
        #button1.pack()
        #button2.pack()
        #button4.pack()


        gif1 = PhotoImage(file = 'imec.png')
        gif1 = gif1.subsample(4)
        label1 = Label(self, image=gif1)
        label1.image = gif1
        label1.grid(row = 80, column = 8000, columnspan = 1, rowspan=1)



class PageOne(tk.Frame):

    def __init__(self, parent, controller):
        ttk.Frame.__init__(self, parent)
        self.controller = controller
        label = ttk.Label(self, text="This is page 1", font=controller.title_font).grid(row=5, column=10,padx=500, pady=100)
        #label.pack(side="top", fill="x", pady=10)
        button = ttk.Button(self, text="Run command",
                           command= lambda: controller.calculate("Live")).grid(row=7, column=15)

        gif1 = PhotoImage(file = 'imec.png')
        gif1 = gif1.subsample(4)
        label1 = Label(self, image=gif1)
        label1.image = gif1
        label1.grid(row = 20, column = 100, columnspan = 1, rowspan=1)


class PageTwo(tk.Frame):

    def __init__(self, parent, controller):

        global process

        
        tk.Frame.__init__(self, parent)
        self.controller = controller
        label = ttk.Label(self, text="Parameters", font=controller.title_font).grid(row=5, column=10)
        button = ttk.Button(self, text="Go to Main menu page",
                           command=lambda: controller.show_frame("StartPage")).grid(row=7, column=15)

 


            #button4 = ttk.Button(self, text="Exit",
            #         command=quit).grid(row=9, column=15)

        gif2 = PhotoImage(file = 'timers.png')
        gif2 = gif2.subsample(3)
        label2 = Label(self, image=gif2)
        label2.image = gif2
        label2.grid(row = 9, column = 10, columnspan = 1, rowspan=1)
             
        labelText1=StringVar()
        labelText1.set("Enter PSM Timer-T3412 (s) ")
        labelDir1=Label(self, textvariable=labelText1, height=1).grid(row=10, column=9)
        e1 = Entry(self,width=25)
        e1.insert(0, "409.6")
        e1.grid(row=10, column=10)
        
        
        labelText2=StringVar()
        labelText2.set("Enter eDRX Timer-T3324 (s)")
        labelDir2 =Label(self, textvariable=labelText2, height=1).grid(row=11, column=9)
        e2 = Entry(self,width=25)
        e2.insert(0, "81.92")
        e2.grid(row=11, column=10)

    
        labelText3=StringVar()
        labelText3.set("Enter eDRX Cycle       (s)")
        labelDir3 =Label(self, textvariable=labelText3, height=1).grid(row=12, column=9)
        e3 = Entry(self,width=25)
        e3.insert(0, "20.48")
        e3.grid(row=12, column=10)

        
        labelText4 = StringVar()
        labelText4.set("Enter DL Interval       (s)")
        labelDir4 =Label(self, textvariable=labelText4, height=1).grid(row=13, column=9)
        e4 = Entry(self,width=25)
        e4.insert(0, "50")
        e4.grid(row=13, column=10)
        
        
        
        labelText5 = StringVar()
        labelText5.set("Enter UL Interval       (s)")
        labelDir5 =Label(self, textvariable=labelText5, height=1).grid(row=14, column=9)
        e5 = Entry(self,width=25)
        e5.insert(0, "150")
        e5.grid(row=14, column=10)
        
        labelText6 = StringVar()
        labelText6.set("Enter Number of Nodes")
        labelDir6  = Label(self, textvariable=labelText6, height=1).grid(row=15, column=9)
        e6 = Entry(self,width=25)
        e6.insert(0, "1")
        e6.grid(row=15, column=10)
        
        labelText7 = StringVar()
        labelText7.set("Enter RRC Inavtivity Timer (s)")
        labelDir7  = Label(self, textvariable=labelText7, height=1).grid(row=16, column=9)
        e7 = Entry(self,width=25)
        e7.insert(0, "10")
        e7.grid(row=16, column=10)
        
        
        labelText8 = StringVar()
        labelText8.set("Enter Packet size (bytes)")
        labelDir8  = Label(self, textvariable=labelText8, height=1).grid(row=17, column=9)
        e8 = Entry(self,width=25)
        e8.insert(0, "32")
        e8.grid(row=17, column=10)
        
        
        labelText9 = StringVar()
        labelText9.set("Enter Execution Time (s)")
        labelDir9  = Label(self, textvariable=labelText9, height=1).grid(row=18, column=9)
        e9 = Entry(self,width=25)
        e9.insert(0, "5000")
        e9.grid(row=18, column=10)
        
        
        labelText10 = StringVar()
        labelText10.set("Enable Randomness?")
        labelDir10  = Label(self, textvariable=labelText10, height=1).grid(row=19, column=9)
        e10 = Entry(self,width=25)
        e10.insert(0, "7")
        e10.grid(row=19, column=10)
        
        #execute_p(t3412,t3324,ptw,dli,uli,nn,rrc,ps,rand,rt)
        #e.focus_set()

        def callback():
            print (e1.get(),e2.get(),e3.get())
            os.system("echo "" > sampleData.txt")

            '''t = threading.Thread(target=execute_p,args=(e1.get(),e2.get(),e3.get(),e4.get(),e5.get(),e6.get(),e7.get(),e8.get(),e10.get(),e9.get()))
            t.daemon = True
            t.start()'''
            process = multiprocessing.Process(target=execute_p,args=(float(e1.get())*1000,float(e2.get())*1000,float(e3.get())*1000,float(e4.get())*1000,float(e5.get())*1000,e6.get(),float(e7.get())*1000,e8.get(),e10.get(),e9.get()))
            process.daemon = True
            process.start()
            processList.append(process)
        

        #label = Label(self,image='imec.png').grid(row=20, column=20)
        gif1 = PhotoImage(file = 'imec.png')
        gif1 = gif1.subsample(4)
        label1 = Label(self, image=gif1)
        label1.image = gif1
        label1.grid(row = 18, column = 150, columnspan = 1, rowspan=1)

        



        
            
        b = ttk.Button(self, text="RUN", width=10, command=callback)
        b.grid(row=15, column=15)

        button3 = ttk.Button(self, text="Show Graph Analysis",
                         command=lambda: controller.show_frame("PageThree")).grid(row=16, column=15)


class PageThree(tk.Frame):
    
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        self.controller = controller
        #label = ttk.Label(self, text="Page Graph", font=controller.title_font)
        #label.pack(side="top", fill="x", pady=10)
        button = ttk.Button(self, text="Go to the start page",
                            command=lambda: controller.show_frame("StartPage"))
        button.pack()

        button1 = ttk.Button(self, text="Go to previous page",
                            command=lambda: controller.show_frame("PageTwo"))
        button1.pack()
                                
        button4 = ttk.Button(self, text="Exit Analysis", command=quit)
        button4.pack()

        button5 = ttk.Button(self, text="Stop last Simulation", command=stop_simulation)
        button5.pack()
        
        canvas = FigureCanvasTkAgg(f, self)
        canvas.draw()
        canvas.get_tk_widget().pack(side = tk.LEFT,fill=tk.BOTH,expand=True)

        canvas = FigureCanvasTkAgg(f1, self)
        canvas.draw()
        canvas.get_tk_widget().pack(side = tk.RIGHT,fill=tk.BOTH,expand=True)

        canvas = FigureCanvasTkAgg(f2, self)
        canvas.draw()
        canvas.get_tk_widget().pack(side = tk.RIGHT,fill=tk.BOTH,expand=True)

if __name__ == "__main__":
    app = SampleApp()
    app.geometry("1920x800")
    ani = animation.FuncAnimation(f, animate, interval=1000) #1000 ms interval
    ani2 = animation.FuncAnimation(f1, animate2, interval=1000) #1000 ms interval
    ani3 = animation.FuncAnimation(f2, animate3, interval=1000) #1000 ms interval

    app.mainloop()
