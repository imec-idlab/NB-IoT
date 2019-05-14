/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Universita' degli Studi di Firenze (UNIFI)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Samuele Foni <samuele.foni@stud.unifi.it> (NB-IOT)
 */

#include "nb-lte-rrc-sap.h"
#include <ns3/lte-enb-rrc.h>
#include <ns3/log.h>

namespace ns3 {

NbLteRrcSap::NbLteRrcSap() {
	// TODO Auto-generated constructor stub

}

NbLteRrcSap::~NbLteRrcSap() {
	// TODO Auto-generated destructor stub
}


void
NbLteRrcSap::SendSystemInformationNb ()
{
  //NS_LOG_FUNCTION (this << "SIB2");
/*
  for (auto &it: m_componentCarrierPhyConf)
    {
      uint8_t ccId = it.first;

      NbLteRrcSap::SystemInformationNb si;
      si.haveSib2 = true;
      si.sib2.freqInfo.ulCarrierFreqNb = it.second->GetUlEarfcn ();
      si.sib2.freqInfo.ulBandwidth = it.second->GetUlBandwidth ();
      //si.sib2.radioResourceConfigCommonSibNb.NpdschConfigCommonNb.referenceSignalPower = m_cphySapProvider.at (ccId)->GetReferenceSignalPower ();
      //si.sib2.radioResourceConfigCommonSibNb.pdschConfigCommon.pb = 0;

      LteEnbCmacSapProvider::RachConfig rc = m_cmacSapProvider.at (ccId)->GetRachConfig ();
      LteRrcSap::RachConfigCommon rachConfigCommon;
      rachConfigCommon.preambleInfo.numberOfRaPreambles = rc.numberOfRaPreambles;
      rachConfigCommon.raSupervisionInfo.preambleTransMax = rc.preambleTransMax;
      rachConfigCommon.raSupervisionInfo.raResponseWindowSize = rc.raResponseWindowSize;
      si.sib2.radioResourceConfigCommonSibNb.rachConfigCommonNb = rachConfigCommon;

      m_rrcSapUser->SendSystemInformation (it.second->GetCellId (), si); // Need to rewrite
    }
*/
  /*
   * For simplicity, we use the same periodicity for all SIBs. Note that in real
   * systems the periodicy of each SIBs could be different.
   */
/*
Simulator::Schedule (m_systemInformationPeriodicity, &LteEnbRrc::SendSystemInformation, this);
*/
}

}
/* namespace ns3 */
