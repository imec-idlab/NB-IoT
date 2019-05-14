# NB-IoT

This is the repository from Imec research group on development of NB-IoT features on NS-3 network simulator. These features include:

* Support of Multi-tone transmissions
* Evaluation of PHY layer performance
* Adapt the number of RB to one
* Limit modulation schemes to QPSK
* Separating subframes for control and data channels
* Power Saving features:
     Discontinuous reception in RRC idle state (DRX), Paging, Power saving mode (PSM)
* Energy evaluation module

## Research papers published on the code evaluation

* Sahithya Ravi, Pouria Zand, Mohieddine El Soussi, Majid Nabi. Evaluation, Modeling and Optimization of Coverage Enhancement Methods of NB-IoT. (Focusing repetition and single tone implementation) (https://arxiv.org/abs/1902.09455)

* M. El Soussi, Pouria Zand, Frank Pasveer, and Guido Dolmans. 2018. Evaluating the Performance of eMTC and NB-IoT for Smart City Applications. In IEEE International Conference on Communications (ICC). (https://ieeexplore.ieee.org/document/8422799/)

* AK Sultania, P Zand, C Blondia, J Famaey. Energy Modeling and Evaluation of NB-IoT with PSM and eDRX, 2018 IEEE Globecom Workshops (GC Wkshps), 2018 (https://www.researchgate.net/publication/329364141_Energy_Modeling_and_Evaluation_of_NB-IoT_with_PSM_and_eDRX)

* AK Sultania, C Delgado, J Famaey. Implementation of NB-IoT Power Saving Schemes in NS-3, in proceeding WNS3 workshop 2019 

If you use our work, you can cite our papers.


### Prerequisites

Check NS-3 page for more information

## Test Scripts

Few test scripts are present in scratch folder and /lte/examples. 

## Authors

* **Pouria Zand** - *Initial work, multisectoring, RBG, MCS* 
* **Sahithya Ravi** - *Repetition and coverage extention* 
* **Sultania Ashish** - *Energy evaluation*


## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE.md](LICENSE.md) file for details
