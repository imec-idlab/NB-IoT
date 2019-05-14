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
        while file_num < 3:
            '''if file_num == 1:
                file_num = 73+2
            elif file_num == 98+2:
                file_num = 523+2
            elif file_num == 548+2:
                file_num = 973+2
            elif file_num == 998+2:
                file_num = 1423+2
            elif file_num == 1448+2:
                file_num = 1448+2
                break'''
            
            # For latency PSM = 300
            '''if file_num == 31:
                file_num = 49
            elif file_num == 55:
                file_num = 73
            elif file_num == 79:
                file_num = 97'''
            
            count_state = {}
            ue_data = {}
            ue_data_end = {}
            psm_paging = {}
            latency_ul = {}
            filename = "ou"+str(file_num)+".txt"
            print filename
            with open(filename) as f:
                line = f.readline()
                while line:
                    text = line.strip()
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
                    
                    elif "LteUeRrc::SwitchToState from IDLE_PSM --> SUSPEND_PAGING" in text:
                        print text
                        rnti = text.split(" ")[1]
                        index = 1  # 1: PSM, 2: DRX, 3: PAging, 4: Connected, 5: DL Sent, 6 DL received, 7: UL Sent, 8: UL Received
                        time = text.split(" ")[0]
                        if rnti not in psm_paging:    
                            psm_paging.setdefault(rnti,{})
                            psm_paging[str(rnti)][str(index)] = []
                            psm_paging[str(rnti)][str(index)].append(float(time))
                        else:
                            psm_paging[str(rnti)][str(index)].append(float(time))
                            
                    elif "LteUeRrc::SwitchToState" in text and "--> IDLE_SUSPEND" in text:
                        rnti = text.split(" ")[1]
                        index = 2  # 1: PSM, 2: DRX, 3: Paging, 4: Connected, 5: DL Sent, 6 DL received, 7: UL Sent, 8: UL Received
                        time = text.split(" ")[0]
                        ue_data[str(rnti)][str(index)].append(float(time)) # Time of that state in seconds

                    elif "LteUeRrc::SwitchToState" in text and "--> SUSPEND_PAGING" in text:
                        rnti = text.split(" ")[1]
                        index = 3  # 1: PSM, 2: DRX, 3: Paging, 4: Connected, 5: DL Sent, 6 DL received, 7: UL Sent, 8: UL Received
                        time = text.split(" ")[0]
                        ue_data[str(rnti)][str(index)].append(float(time)) # Time of that state in seconds
                        
                    elif "LteUeRrc::SwitchToState" in text and "--> IDLE_PSM" in text:
                        rnti = text.split(" ")[1]
                        index = 1  # 1: PSM, 2: DRX, 3: Paging, 4: Connected, 5: DL Sent, 6 DL received, 7: UL Sent, 8: UL Received
                        time = text.split(" ")[0]
                        ue_data[str(rnti)][str(index)].append(float(time)) # Time of that state in seconds
                    
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
                        
                    line = f.readline()

            f.close()

            print ue_data
            if not ue_data:
                print "FILE NUMBER: " + str(file_num)
                print "-------------------------"
                #print latency

  
        
            for rnti in ue_data.keys():
                print rnti, runtime
                fig = plt.figure(figsize=(8, 6))
                #plt.plot(ue_data[str(rnti)]['1'], [1] * len(ue_data[str(rnti)]['1']),'r|' )
                plt.plot(psm_paging[str(rnti)]['1'], [1] * len(psm_paging[str(rnti)]['1']),'g|' )
                
                #plt.plot(ue_data[str(rnti)]['2'], [2] * len(ue_data[str(rnti)]['2']),'b|')
                plt.plot(ue_data[str(rnti)]['3'], [2] * len(ue_data[str(rnti)]['3']),'g|' )
                #plt.plot(ue_data[str(rnti)]['4'], [4] * len(ue_data[str(rnti)]['4']),'c|' )
                plt.plot(ue_data[str(rnti)]['5'], [4] * len(ue_data[str(rnti)]['5']),'c|' )
                plt.plot(ue_data[str(rnti)]['6'], [4] * len(ue_data[str(rnti)]['6']),'k4' )
                
                plt.plot(ue_data[str(rnti)]['7'], [5] * len(ue_data[str(rnti)]['7']),'k|' )
                plt.plot(ue_data[str(rnti)]['8'], [5] * len(ue_data[str(rnti)]['8']),'C0|' )
                
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
                    
                    
                #plt.grid(True)
                plt.gca().xaxis.grid(True)
                #plt.yticks([])
                plt.yticks([1,2,3,4,5,6], ['PSM', 'DRX', 'Connected', 'DL Data', 'UL Data'])
                plt.xticks(np.arange(0, runtime+1, 50))

                plt.xlabel('Time')
                print "wah_" + str(file_num) + "_" + str(rnti) + ".pdf"
                fig.savefig("wah_" + str(file_num) + "_" + str(rnti) + ".pdf")
                plt.show()
                
            file_num = file_num + 1
    except IOError:
        print "Error Exiting"
        exit()


if(__name__ == "__main__"):
    main()
