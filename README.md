# NB-IoT energy efficiency features

This is the branch for energy evaluation development for the PSM and eDRX features of NB-IoT. These features include:

* Power Saving features:
-- Discontinuous reception in RRC idle state (DRX)
-- One Paging occasion within a Paging Time Window (PTW)
-- Power saving mode (PSM)
* Energy evaluation module
* Update with ns-3.29
* Merged the code of branch repeatition_coverage so this branch includes the features such as 
  * Support for multi-tone transmissions
  * Limit NB-IoT transmissions to single resource block (RB) and QPSK modulation
  * Separating subframes for control and data channels


![Power Saving Scheme parameters](https://cdn1.imggmi.com/uploads/2019/6/3/32797db177704d123a236ebec9699ef3-full.png)

## Research papers published on the code evaluation
If you use our work, you can cite our papers.

* AK Sultania, P Zand, C Blondia, J Famaey. Energy Modeling and Evaluation of NB-IoT with PSM and eDRX, 2018 IEEE Globecom Workshops (GC Wkshps), 2018. (https://www.researchgate.net/publication/329364141_Energy_Modeling_and_Evaluation_of_NB-IoT_with_PSM_and_eDRX)

* AK Sultania, C Delgado, J Famaey. Implementation of NB-IoT Power Saving Schemes in ns-3. In 2019 Workshop on Next-Generation Wireless with ns-3 (WNGW 2019), June 21, 2019, Florence, Italy.ACM, New York, NY, USA, 4 pages. (https://www.researchgate.net/publication/333632115_Implementation_of_NB-IoT_Power_Saving_Schemes_in_ns-3)


## Installation and usage instructions
#### Prerequisites
Check ns-3 page for more information

#### Instructions
* Clone the project from git
* Change into NB-IoT directory.
* Configure waf: `CXXFLAGS="-std=c++0x -Wall -g -O0" ./waf configure --build-profile=debug --enable-static --disable-examples --enable-modules=lte`
* Build: *./waf*
#### Test Scripts
The basic test scripts are present in scratch folder and /lte/examples.
We use the test script `/scratch/lena-simple-epc-1.cc` which defines the scenario to test 1 eNB associated with N-UEs sending bi- or uni- directional packets.
* Run the simulation: `./waf --run lena-simple-epc-1`
  - We can also configure parameters during executing the test script as:
     `./waf --run "lena-simple-epc-1 -simTime=500 -interPacketIntervaldl=50000 -interPacketIntervalul=000 -numberOfNodes=1 -t3324=20480 -t3412=51200 -rrc_release_timer=10000 -packetsize=32 -EnableRandom=6"`

#### Key Parameters
 * *numberOfNodes*: Number of UEs
 * *simTime*: Total duration of the simulation in seconds
 * *interPacketIntervaldl*: DL Inter packet interval in milliseconds
 * *interPacketIntervalul*: UL Inter packet interval in milliseconds 
 * *tc*:  Test case number, used to distinguish the output files
 * *packetsize*: Data packet size to send 
 * *t3324*: eDRX timer
 * *t3412*: PSM timer
 * *edrx_cycle*: edrx Cycle
 * *rrc_release_timer*: RRC Inactivity timer
 * *EnableRandom*: Enable Randomness in the packet transmission and to test different IoT scenarios. It can set as:
   -- 0: Packets will be sent in fixed interval 
   -- 1: Packets will be sent in poisson distributed
   -- 2: Simulate Voice call
   -- 3: Simulate periodic update in the configured interval
   -- 7: Packet send independently and receives response from the receiver

   #### Other important parametes
      > -- MaxMcs (`/src/lte/model/lte-amc.cc:91`): Set maximum MCS  
      > -- MaxPackets(`/scratch/lena-simple-epc-1.cc:194,202`): The maximum number of packets the eNB/UEs will send
      > -- Percent (`/scratch/lena-simple-epc-1.cc:197,205`): Percentage of randomness need to send packet as poisson distribution
      > -- EnableDiagnostic`(/scratch/lena-simple-epc-1.cc:198,206`): This is enable UL data of 32 bytes in an interval of 10 seconds

## Output Analysis
The test script will display some messages on the terminal. This output can be exported to a file. The important prints that can be useful to check are as follow:
 - *<time> <rnti> LteUeRrc::SwitchToState from <Previous_state> --> <Current_state>*: Denotes the UE state changes at UE side.
 - *<time> <rnti> eNb::SwitchToState <Previous_state> --> <Current_state>*: Denotes the UE state changes at eNB side for UE of mentioned rnti.
 - *<time> <energy_spent_inlaststate> Remaining energy in joules <remaining_energy> EnergyConsumed <total_energy_consumed_till_thistime>*: Denotes the total energy spent till the mentioned time.
 - *<time> <rnti> <previous_state>: <state_number> <time_spent_in_previous_state_in_ns> m_total: <current_time_in_ns>*: Denotes the time spent by the mentioned UE in the last state.
#### Energy calculation
The energy calculating module is present at `/src/energy-module-nbiot` and `energy-module-nbiot.cc` can be modified with the specified energy values for each states (as per the assumed devicce datasheet). Alternatively, the python script `parsestate.py` can be adapted and run on the exported terminal message file.

## Authors
* **Sultania Ashish** 

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE.md](LICENSE.md) file for details
