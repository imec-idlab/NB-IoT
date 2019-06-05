# NB-IoT

This is the repository from Imec research group on development of NB-IoT features on ns-3 network simulator. It consists of two branches namely **energy_evaluation** and **repetition_coverage**. 

The main features that are included in branch **repetition_coverage** are as follows:
* Support of Multi-tone transmissions
* Evaluation of PHY layer performance
* Adapt the number of RB to one
* Limit modulation schemes to QPSK
* Separating subframes for control and data channels

The main features that are included in branch **energy_evaluation** are as follows:
* Power Saving features:
-- Discontinuous reception in RRC idle state (DRX)
-- Paging
-- Power saving mode (PSM)
* Energy evaluation module
* Updates with ns-3.29
* Merged the code of branch repeatition_coverage so this branch

## Research papers published on the code evaluation

* Sahithya Ravi, Pouria Zand, Mohieddine El Soussi, Majid Nabi. Evaluation, Modeling and Optimization of Coverage Enhancement Methods of NB-IoT. (Focusing repetition and single tone implementation) (https://arxiv.org/abs/1902.09455)

* M. El Soussi, Pouria Zand, Frank Pasveer, and Guido Dolmans. 2018. Evaluating the Performance of eMTC and NB-IoT for Smart City Applications. In IEEE International Conference on Communications (ICC). (https://ieeexplore.ieee.org/document/8422799/)

* AK Sultania, P Zand, C Blondia, J Famaey. Energy Modeling and Evaluation of NB-IoT with PSM and eDRX, 2018 IEEE Globecom Workshops (GC Wkshps), 2018 (https://www.researchgate.net/publication/329364141_Energy_Modeling_and_Evaluation_of_NB-IoT_with_PSM_and_eDRX)

* AK Sultania, C Delgado, J Famaey. Implementation of NB-IoT Power Saving Schemes in ns-3. In 2019 Workshop on Next-Generation Wireless with ns-3 (WNGW 2019), June 21, 2019, Florence, Italy.ACM, New York, NY, USA, 4 pages. (https://www.researchgate.net/publication/333632115_Implementation_of_NB-IoT_Power_Saving_Schemes_in_ns-3)

If you use our work, you can cite our papers.


### Prerequisites

Check ns-3 page for more information

## Test Scripts

Few test scripts are present in scratch folder and `/lte/examples`. 

## Authors

* **Pouria Zand** - *Initial work, multisectoring, RBG, MCS* 
* **Sahithya Ravi** - *Repetition and coverage extention* 
* **Sultania Ashish** - *Energy evaluation*


## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE.md](LICENSE.md) file for details
