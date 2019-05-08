# README ##

### What is this repository for? ###

Implements NB-IoT module in NS-3 (The name of the module is still lte)

### How do I get set up? ###

Clone the repository and refer to NS-3 documentation for building NS-3.  
Branch name : **nbiot1**
Make sure waf is configured as shown below:

    CXXFLAGS="-Wall" ./waf configure --enable-examples --enable-tests

This is to prevent NS-3 from treating warnings as errors.

#### Configuration: ####

To configure NB-IoT, open file

 ns-3-dev-git-private/src/lte/model/lte-common.h

This file provides options to configure how the uplink link adaptation of NB-IoT is performed.

##### MCS #####

By default the uplink link adaptation of NB-IoT is performed based only on MCS as defined by,

    #define MCS 1
    
##### Tone #####

In order to enable uplink link adaptation based on tone, set TONE to 1 and SUBCARRIER to 1:

    #define TONE 1
    #define SUBCARRIER 1 

When SUBCARRIER is set to 1, the Power spectral density, SNR etc are based on subcarriers and 
not resource blocks. 

Resource allocation is also performed based on subcarriers. 

##### Repetition #####

In order to enable link adaptation based on repetition, set REPEAT to 1:

    #define REPEAT 1

##### Hybrid #####

In addition, there is an option for hybrid link adaptation based on repetitions, tones and MCS.  

The hybrid link adaptation strategy tries to find the combination of tones, repetitions and MCS 
that minimizes latency per user while achieving the required SNR.

There are two methods for hybrid link adaptation, exhaustive and analytical. 

The exhaustive method and analytical method give  the same result for latency per user, but the
analytical method is slightly faster in terms of execution time.

Analytical :

     #define ANALYTICAL 1
        
 Exhaustive : 

    #define EXHAUSTIVE 1


### Running the simulation ###

The example application is found in the following path:
ns-3-dev-git-private/src/lte/examples/lena-simple-epc.cc

It can be run using the following command.:

    ./waf --run lena-simple-epc
  
Refer NS-3 documentation for setting parameters like simulation time, no.of. users etc  with this command


