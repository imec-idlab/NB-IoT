import matplotlib
matplotlib.use('Agg')
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import rc
from numpy import double

rc('mathtext', default='regular')

def main():
    file_num = 1
    psmval      = {}
    

    try:
        while file_num < 1483:
            
            count_state = {}
            ue_data = {}
            latency = {}
            latency_ul = {}
            t3412 = 0
            edrx_cycle = 0
            t3324 = 0
            filename = "logs/out"+str(file_num)+".txt"
            with open(filename) as f:
                line = f.readline()
                while line:
                    text = line.strip()
                    if "T3412:" in text:
                        t3412 = (float((text.split(" ")[5]).split("ns")[0]) - float((text.split(" ")[3]).split("ns")[0]))/1000000000
                        if t3412<0:
                            t3412 = 0
                        edrx_cycle = float((text.split(" ")[7]).split("ns")[0])/1000000000
                        t3324 = float((text.split(" ")[3]).split("ns")[0])/1000000000
                    if "m_total" in text:
                        rnti = text.split(" ")[1]
                        index = text.split(" ")[3]
                        runtime = float(text.split(" ")[6])/1000000000
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
                    line = f.readline()

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

                #print ue_data
                #print count_state
                #print count_state[str(1)][str(7)]

                i = 1
                total_energy= []
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
                    total_energy.append( tenergy)
                    psmval[str(i)]['totalenergy'].append(float(tenergy*3600/float(runtime)))

                    i = i + 1

                f.close()
                puid = {}
                lastpsm = 0
                lastsuspend = 0
                lastsuspend_d = 0
                lastpsm_d = 0
                with open(filename) as f:
                    line = f.readline()
                    while line:
                        text = line.strip()
                        if "--> IDLE_PSM" in text:
                            print text
                            lastpsm_d = float(text.split(" ")[0]) + t3412
                            lastpsm = float(text.split(" ")[0])
                            #print lastpsm
                        if "--> IDLE_SUSPEND" in text:
                            print text
                            lastsuspend_d = float(text.split(" ")[0]) + edrx_cycle
                            lastsuspend = float(text.split(" ")[0])
                                
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
                
                        line = f.readline()
                                    
                f.close()
                if latency:
                    print "FILE NUMBER: " + str(file_num)
                    print "-------------------------"
                    #print latency

                count = 0
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
                    

                    print "Average delay in DL for rnti[" + str(i) + "]: " + str(dllatency/dlcounter) + " seconds for avg received data " +\
                        str(dldata_r/dlcounter) + " bytes having received percentage= " + str((dlcounter*100)/(dlmissed_s+dlcounter)) + "% with UE Avg. energy consumption of " + str(total_energy[count]*3600/float(runtime)) + " J/hr running time "+ str(runtime) + " s"

                    psmval[i]['latency_dl'].append(float(dllatency/dlcounter))
                    count = count + 1
                count = 0
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

                    '''print "Average delay in UL for rnti[" + str(i) + "]: " + str(ullatency/ulcounter) + " seconds for avg received data " +\
                                    str(uldata_r/ulcounter) + " bytes having received percentage= " + str((uldata_r*100)/uldata_r) + "% with UE Avg. energy consumption of " + str(total_energy[count]*3600/float(runtime)) + " J/hr running time "+ str(runtime) + " s"'''
                

                    psmval[i]['latency_ul'].append(float(ullatency/ulcounter))
                    count = count + 1

                                    
                #print "-------------------------"
                file_num = file_num + 1
        # print "psmval" + str(psmval)
        
        
        
        # Graph 1 , Fix 3412 = 614, 3324=61, DLi= 30 x=UL interval, Y = Totol energy
        for countrnti in ue_data.keys():
            print len(psmval[countrnti]['totalenergy'])
            print len(psmval[countrnti]['latency_dl'])
            fig = plt.figure()
            plt.plot((2.56,5.12,10.24,20.48,40.96), (psmval[countrnti]['totalenergy'][72], psmval[countrnti]['totalenergy'][72+6], psmval[countrnti]['totalenergy'][72+12], psmval[countrnti]['totalenergy'][72+18], psmval[countrnti]['totalenergy'][72+24]) )
            
            plt.plot((2.56,5.12,10.24,20.48,40.96), (psmval[countrnti]['totalenergy'][432], psmval[countrnti]['totalenergy'][432+6], psmval[countrnti]['totalenergy'][432+12], psmval[countrnti]['totalenergy'][432+18], psmval[countrnti]['totalenergy'][432+24]) )
            
            plt.plot((2.56,5.12,10.24,20.48,40.96), (psmval[countrnti]['totalenergy'][762], psmval[countrnti]['totalenergy'][762+6], psmval[countrnti]['totalenergy'][762+12], psmval[countrnti]['totalenergy'][762+18], psmval[countrnti]['totalenergy'][762+24]) )
            
            plt.plot((2.56,5.12,10.24,20.48,40.96), (psmval[countrnti]['totalenergy'][1122], psmval[countrnti]['totalenergy'][1122+6], psmval[countrnti]['totalenergy'][1122+12], psmval[countrnti]['totalenergy'][1122+18], psmval[countrnti]['totalenergy'][1122+24]) )
            
            plt.legend(['RRC_Inact=1', 'RRC_Inact=10', 'RRC_Inact=30','RRC_Inact=60'], loc='center right')
            plt.xlabel('eDRX_cycle')
            plt.ylabel('Average Consumed Energy per hour (in Joules)')
            plt.title('PSM timer = 614.4s')
            fig.savefig("energy_E_eDRX_cycle_RRC1.pdf")
            plt.close()
                #plt.show()
                
                
            fig = plt.figure()
            plt.plot((2.56,5.12,10.24,20.48,40.96), (psmval[countrnti]['latency_dl'][72], psmval[countrnti]['latency_dl'][72+6], psmval[countrnti]['latency_dl'][72+12], psmval[countrnti]['latency_dl'][72+18], psmval[countrnti]['latency_dl'][72+24]) )
            
            plt.plot((2.56,5.12,10.24,20.48,40.96), (psmval[countrnti]['latency_dl'][432], psmval[countrnti]['latency_dl'][432+6], psmval[countrnti]['latency_dl'][432+12], psmval[countrnti]['latency_dl'][432+18], psmval[countrnti]['latency_dl'][432+24]) )
            
            plt.plot((2.56,5.12,10.24,20.48,40.96), (psmval[countrnti]['latency_dl'][762], psmval[countrnti]['latency_dl'][762+6], psmval[countrnti]['latency_dl'][762+12], psmval[countrnti]['latency_dl'][762+18], psmval[countrnti]['latency_dl'][762+24]) )
            
            plt.plot((2.56,5.12,10.24,20.48,40.96), (psmval[countrnti]['latency_dl'][1122], psmval[countrnti]['latency_dl'][1122+6], psmval[countrnti]['latency_dl'][1122+12], psmval[countrnti]['latency_dl'][1122+18], psmval[countrnti]['latency_dl'][1122+24]) )
            
            plt.legend(['RRC_Inact=1', 'RRC_Inact=10', 'RRC_Inact=30','RRC_Inact=60'], loc='center right')
            plt.xlabel('eDRX_cycle')
            plt.ylabel('Average Consumed DL Latency (in seconds)')
            plt.title('PSM timer = 614.4s')
            fig.savefig("latency_E_eDRX_cycle_RRC1.pdf")
            plt.close()
            #plt.show()
        
            
                    
    except IOError:
        print "Error Exiting"
        exit()


if(__name__ == "__main__"):
    main()
