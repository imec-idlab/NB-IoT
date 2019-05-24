/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011, 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 *         Budiarto Herman <budiarto.herman@magister.fi>
 * Modified by:
 *          Danilo Abrignani <danilo.abrignani@unibo.it> (Carrier Aggregation - GSoC 2015)
 *          Biljana Bojovic <biljana.bojovic@cttc.es> (Carrier Aggregation)
 *          Samuele Foni <samuele.foni@stud.unifi.it> (NB-IOT)
 *
 */

#include "lte-ue-rrc.h"

#include <ns3/fatal-error.h>
#include <ns3/log.h>
#include <ns3/object-map.h>
#include <ns3/object-factory.h>
#include <ns3/simulator.h>

#include <ns3/lte-rlc.h>
#include <ns3/lte-rlc-tm.h>
#include <ns3/lte-rlc-um.h>
#include <ns3/lte-rlc-am.h>
#include <ns3/lte-pdcp.h>
#include <ns3/lte-radio-bearer-info.h>
#include <algorithm>

#include <cmath>

#include "ns3/simple-device-energy-model.h"
#include "ns3/li-ion-energy-source.h"
#include "ns3/energy-module-nbiot.h"
#include "ns3/energy-source-container.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LteUeRrc");

/////////////////////////////
// CMAC SAP forwarder
/////////////////////////////

/// UeMemberLteUeCmacSapUser class
class UeMemberLteUeCmacSapUser : public LteUeCmacSapUser
{
public:
  /**
   * Constructor
   *
   * \param rrc the RRC class
   */
  UeMemberLteUeCmacSapUser (LteUeRrc* rrc);

  virtual void SetTemporaryCellRnti (uint16_t rnti);
  virtual void NotifyEnergyChange();
  virtual void SetFrameSubframe(uint32_t frame, uint32_t sfn);
  virtual void NotifyRandomAccessSuccessful ();
  virtual void NotifyRandomAccessFailed ();

private:
  LteUeRrc* m_rrc; ///< the RRC class
};

UeMemberLteUeCmacSapUser::UeMemberLteUeCmacSapUser (LteUeRrc* rrc)
  : m_rrc (rrc)
{
}

void
UeMemberLteUeCmacSapUser::SetTemporaryCellRnti (uint16_t rnti)
{
  m_rrc->DoSetTemporaryCellRnti (rnti);
}

void
UeMemberLteUeCmacSapUser::NotifyEnergyChange ( )
{
  m_rrc->DoNotifyEnergyChange ();
}

void
UeMemberLteUeCmacSapUser::SetFrameSubframe(uint32_t frame, uint32_t sfn)
{
  m_rrc->DoSetFrameSubframe (frame,sfn);
}


void
UeMemberLteUeCmacSapUser::NotifyRandomAccessSuccessful ()
{
  m_rrc->DoNotifyRandomAccessSuccessful ();
}

void
UeMemberLteUeCmacSapUser::NotifyRandomAccessFailed ()
{
  m_rrc->DoNotifyRandomAccessFailed ();
}






/// Map each of UE RRC states to its string representation.
static const std::string g_ueRrcStateName[LteUeRrc::NUM_STATES] =
{
  "IDLE_START",
  "IDLE_CELL_SEARCH",
  "IDLE_WAIT_MIB_SIB1",
  "IDLE_WAIT_MIB",
  "IDLE_WAIT_SIB1",
  "IDLE_CAMPED_NORMALLY",
  "IDLE_WAIT_SIB2",
  "IDLE_PAGING",
  "IDLE_RANDOM_ACCESS",
  "IDLE_CONNECTING",
  "CONNECTED_NORMALLY",
  "CONNECTED_HANDOVER",
  "CONNECTED_PHY_PROBLEM",
  "CONNECTED_REESTABLISHING",
  "IDLE_SUSPEND",
  "SUSPEND_PAGING",
  "IDLE_PSM"
};

/**
 * \param s The UE RRC state.
 * \return The string representation of the given state.
 */
static const std::string & ToString (LteUeRrc::State s)
{
  return g_ueRrcStateName[s];
}


/////////////////////////////
// ue RRC methods
/////////////////////////////

NS_OBJECT_ENSURE_REGISTERED (LteUeRrc);


LteUeRrc::LteUeRrc ()
  : m_cmacSapProvider (0),
    m_rrcSapUser (0),
    m_macSapProvider (0),
    m_asSapUser (0),
    m_ccmRrcSapProvider (0),
    m_state (IDLE_START),
    m_imsi (0),
    m_rnti (0),
    m_cellId (0),
    m_useRlcSm (true),
    m_T(128),
    m_nb (128),
    m_pf(0),
    m_po(0),
    m_t3324(0),
    m_t3412(0),
    m_edrx_cycle(0),
    m_gotpaging(false),
    m_requirepagingflag (true),
    m_lastUpdateTime (NanoSeconds (0.0)),
    m_connectionPending (false),
    m_hasReceivedMib (false),
    m_hasReceivedSib1 (false),
    m_hasReceivedSib2 (false),
    m_hasReceivedPaging (false),
    m_csgWhiteList (0),
    m_numberOfComponentCarriers (MIN_NO_CC)
{
  NS_LOG_FUNCTION (this);

  m_cphySapUser.push_back (new MemberLteUeCphySapUser<LteUeRrc> (this));
  m_cmacSapUser.push_back (new UeMemberLteUeCmacSapUser (this));
  m_cphySapProvider.push_back(0);
  m_cmacSapProvider.push_back(0);
  m_rrcSapProvider = new MemberLteUeRrcSapProvider<LteUeRrc> (this);
  m_drbPdcpSapUser = new LtePdcpSpecificLtePdcpSapUser<LteUeRrc> (this);
  m_asSapProvider = new MemberLteAsSapProvider<LteUeRrc> (this);
  m_ccmRrcSapUser = new MemberLteUeCcmRrcSapUser<LteUeRrc> (this);
}

LteUeRrc::~LteUeRrc ()
{
  NS_LOG_FUNCTION (this);
}

void
LteUeRrc::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  std::cout<< Simulator::Now().GetSeconds()<<" LteUeRrc::DoDispose "<<m_state<<std::endl;
  SwitchToState (CONNECTED_NORMALLY); // To update the energy consumption and initialize the state
  for ( uint16_t i = 0; i < m_numberOfComponentCarriers; i++)
   {
      delete m_cphySapUser.at(i);
      delete m_cmacSapUser.at(i);
   }
  m_cphySapUser.clear ();
  m_cmacSapUser.clear ();
  delete m_rrcSapProvider;
  delete m_drbPdcpSapUser;
  delete m_asSapProvider;
  delete m_ccmRrcSapUser;
  m_cphySapProvider.erase(m_cphySapProvider.begin(), m_cphySapProvider.end());
  m_cphySapProvider.clear();
  m_cmacSapProvider.erase(m_cmacSapProvider.begin(), m_cmacSapProvider.end());
  m_cmacSapProvider.clear();
  m_drbMap.clear ();
}

TypeId
LteUeRrc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LteUeRrc")
    .SetParent<Object> ()
    .SetGroupName ("Lte")
    .AddConstructor<LteUeRrc> ()
    .AddAttribute ("DataRadioBearerMap", "List of UE RadioBearerInfo for Data Radio Bearers by LCID.",
                   ObjectMapValue (),
                   MakeObjectMapAccessor (&LteUeRrc::m_drbMap),
                   MakeObjectMapChecker<LteDataRadioBearerInfo> ())
    .AddAttribute ("Srb0", "SignalingRadioBearerInfo for SRB0",
                   PointerValue (),
                   MakePointerAccessor (&LteUeRrc::m_srb0),
                   MakePointerChecker<LteSignalingRadioBearerInfo> ())
    .AddAttribute ("Srb1", "SignalingRadioBearerInfo for SRB1",
                   PointerValue (),
                   MakePointerAccessor (&LteUeRrc::m_srb1),
                   MakePointerChecker<LteSignalingRadioBearerInfo> ())
    .AddAttribute ("CellId",
                   "Serving cell identifier",
                   UintegerValue (0), // unused, read-only attribute
                   MakeUintegerAccessor (&LteUeRrc::GetCellId),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("C-RNTI",
                   "Cell Radio Network Temporary Identifier",
                   UintegerValue (0), // unused, read-only attribute
                   MakeUintegerAccessor (&LteUeRrc::GetRnti),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("T300",
                   "Timer for the RRC Connection Establishment procedure "
                   "(i.e., the procedure is deemed as failed if it takes longer than this)",
                   TimeValue (MilliSeconds (100)),
                   MakeTimeAccessor (&LteUeRrc::m_t300),
                   MakeTimeChecker ())
    .AddAttribute ("T3324",
                  "Timer for the T3324 ",
                  IntegerValue ( (0)),
                  MakeIntegerAccessor (&LteUeRrc::m_t3324_d),
                  MakeIntegerChecker<int32_t> ())
    .AddAttribute ("T3412",
                    "Timer for the T3412 ",
                    IntegerValue ( (10000)),
                    MakeIntegerAccessor (&LteUeRrc::m_t3412_d),
                    MakeIntegerChecker<int64_t> ())
    .AddAttribute ("TeDRXC",
              "Timer for the TeDRXC ",
              IntegerValue ( (0)),
              MakeIntegerAccessor (&LteUeRrc::m_edrx_cycle_d),
              MakeIntegerChecker<int32_t> ())
    .AddTraceSource ("MibReceived",
                     "trace fired upon reception of Master Information Block",
                     MakeTraceSourceAccessor (&LteUeRrc::m_mibReceivedTrace),
                     "ns3::LteUeRrc::MibSibHandoverTracedCallback")
    .AddTraceSource ("Sib1Received",
                     "trace fired upon reception of System Information Block Type 1",
                     MakeTraceSourceAccessor (&LteUeRrc::m_sib1ReceivedTrace),
                     "ns3::LteUeRrc::MibSibHandoverTracedCallback")
    .AddTraceSource ("Sib2Received",
                     "trace fired upon reception of System Information Block Type 2",
                     MakeTraceSourceAccessor (&LteUeRrc::m_sib2ReceivedTrace),
                     "ns3::LteUeRrc::ImsiCidRntiTracedCallback")
    .AddTraceSource ("StateTransition",
                     "trace fired upon every UE RRC state transition",
                     MakeTraceSourceAccessor (&LteUeRrc::m_stateTransitionTrace),
                     "ns3::LteUeRrc::StateTracedCallback")
    .AddTraceSource ("InitialCellSelectionEndOk",
                     "trace fired upon successful initial cell selection procedure",
                     MakeTraceSourceAccessor (&LteUeRrc::m_initialCellSelectionEndOkTrace),
                     "ns3::LteUeRrc::CellSelectionTracedCallback")
    .AddTraceSource ("InitialCellSelectionEndError",
                     "trace fired upon failed initial cell selection procedure",
                     MakeTraceSourceAccessor (&LteUeRrc::m_initialCellSelectionEndErrorTrace),
                     "ns3::LteUeRrc::CellSelectionTracedCallback")
    .AddTraceSource ("RandomAccessSuccessful",
                     "trace fired upon successful completion of the random access procedure",
                     MakeTraceSourceAccessor (&LteUeRrc::m_randomAccessSuccessfulTrace),
                     "ns3::LteUeRrc::ImsiCidRntiTracedCallback")
    .AddTraceSource ("RandomAccessError",
                     "trace fired upon failure of the random access procedure",
                     MakeTraceSourceAccessor (&LteUeRrc::m_randomAccessErrorTrace),
                     "ns3::LteUeRrc::ImsiCidRntiTracedCallback")
    .AddTraceSource ("ConnectionEstablished",
                     "trace fired upon successful RRC connection establishment",
                     MakeTraceSourceAccessor (&LteUeRrc::m_connectionEstablishedTrace),
                     "ns3::LteUeRrc::ImsiCidRntiTracedCallback")
    .AddTraceSource ("ConnectionTimeout",
                     "trace fired upon timeout RRC connection establishment because of T300",
                     MakeTraceSourceAccessor (&LteUeRrc::m_connectionTimeoutTrace),
                     "ns3::LteUeRrc::ImsiCidRntiTracedCallback")
    .AddTraceSource ("ConnectionReconfiguration",
                     "trace fired upon RRC connection reconfiguration",
                     MakeTraceSourceAccessor (&LteUeRrc::m_connectionReconfigurationTrace),
                     "ns3::LteUeRrc::ImsiCidRntiTracedCallback")
    .AddTraceSource ("HandoverStart",
                     "trace fired upon start of a handover procedure",
                     MakeTraceSourceAccessor (&LteUeRrc::m_handoverStartTrace),
                     "ns3::LteUeRrc::MibSibHandoverTracedCallback")
    .AddTraceSource ("HandoverEndOk",
                     "trace fired upon successful termination of a handover procedure",
                     MakeTraceSourceAccessor (&LteUeRrc::m_handoverEndOkTrace),
                     "ns3::LteUeRrc::ImsiCidRntiTracedCallback")
    .AddTraceSource ("HandoverEndError",
                     "trace fired upon failure of a handover procedure",
                     MakeTraceSourceAccessor (&LteUeRrc::m_handoverEndErrorTrace),
                     "ns3::LteUeRrc::ImsiCidRntiTracedCallback")
  ;
  return tid;
}


void
LteUeRrc::SetLteUeCphySapProvider (LteUeCphySapProvider * s)
{
  NS_LOG_FUNCTION (this << s);
  m_cphySapProvider.at(0) = s;
}

void
LteUeRrc::SetLteUeCphySapProvider (LteUeCphySapProvider * s, uint8_t index)
{
  NS_LOG_FUNCTION (this << s);
  m_cphySapProvider.at(index) = s;
}

LteUeCphySapUser*
LteUeRrc::GetLteUeCphySapUser ()
{
  NS_LOG_FUNCTION (this);
  return m_cphySapUser.at(0);
}

LteUeCphySapUser*
LteUeRrc::GetLteUeCphySapUser (uint8_t index)
{
  NS_LOG_FUNCTION (this);
  return m_cphySapUser.at(index);
}

void
LteUeRrc::SetLteUeCmacSapProvider (LteUeCmacSapProvider * s)
{
  NS_LOG_FUNCTION (this << s);
  m_cmacSapProvider.at (0) = s;
}

void
LteUeRrc::SetLteUeCmacSapProvider (LteUeCmacSapProvider * s, uint8_t index)
{
  NS_LOG_FUNCTION (this << s);
  m_cmacSapProvider.at (index) = s;
}

LteUeCmacSapUser*
LteUeRrc::GetLteUeCmacSapUser ()
{
  NS_LOG_FUNCTION (this);
  return m_cmacSapUser.at (0);
}

LteUeCmacSapUser*
LteUeRrc::GetLteUeCmacSapUser (uint8_t index)
{
  NS_LOG_FUNCTION (this);
  return m_cmacSapUser.at (index);
}

void
LteUeRrc::SetLteUeRrcSapUser (LteUeRrcSapUser * s)
{
  NS_LOG_FUNCTION (this << s);
  m_rrcSapUser = s;
}

LteUeRrcSapProvider*
LteUeRrc::GetLteUeRrcSapProvider ()
{
  NS_LOG_FUNCTION (this);
  return m_rrcSapProvider;
}

void
LteUeRrc::SetLteMacSapProvider (LteMacSapProvider * s)
{
  NS_LOG_FUNCTION (this << s);
  m_macSapProvider = s;
}

void
LteUeRrc::SetLteCcmRrcSapProvider (LteUeCcmRrcSapProvider * s)
{
  NS_LOG_FUNCTION (this << s);
  m_ccmRrcSapProvider = s;
}

LteUeCcmRrcSapUser*
LteUeRrc::GetLteCcmRrcSapUser ()
{
  NS_LOG_FUNCTION (this);
  return m_ccmRrcSapUser;
}

void
LteUeRrc::SetAsSapUser (LteAsSapUser* s)
{
  m_asSapUser = s;
}

LteAsSapProvider* 
LteUeRrc::GetAsSapProvider ()
{
  return m_asSapProvider;
}

void 
LteUeRrc::SetImsi (uint64_t imsi)
{
  NS_LOG_FUNCTION (this << imsi);
  m_imsi = imsi;
}

uint64_t
LteUeRrc::GetImsi (void) const
{
  return m_imsi;
}

uint16_t
LteUeRrc::GetRnti () const
{
  NS_LOG_FUNCTION (this);
  return m_rnti;
}

uint16_t
LteUeRrc::GetCellId () const
{
  NS_LOG_FUNCTION (this);
  return m_cellId;
}


uint8_t 
LteUeRrc::GetUlBandwidth () const
{
  NS_LOG_FUNCTION (this);
  return m_ulBandwidth;
}

uint8_t 
LteUeRrc::GetDlBandwidth () const
{
  NS_LOG_FUNCTION (this);
  return m_dlBandwidth;
}

uint32_t
LteUeRrc::GetDlEarfcn () const
{
  return m_dlEarfcn;
}

uint32_t 
LteUeRrc::GetUlEarfcn () const
{
  NS_LOG_FUNCTION (this);
  return m_ulEarfcn;
}

LteUeRrc::State
LteUeRrc::GetState (void) const
{
  NS_LOG_FUNCTION (this);
  return m_state;
}

void
LteUeRrc::SetUseRlcSm (bool val) 
{
  NS_LOG_FUNCTION (this);
  m_useRlcSm = val;
}


void
LteUeRrc::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);

  // setup the UE side of SRB0
  uint8_t lcid = 0;

  Ptr<LteRlc> rlc = CreateObject<LteRlcTm> ()->GetObject<LteRlc> ();
  rlc->SetLteMacSapProvider (m_macSapProvider);
  rlc->SetRnti (m_rnti);
  rlc->SetLcId (lcid);

  m_srb0 = CreateObject<LteSignalingRadioBearerInfo> ();
  m_srb0->m_rlc = rlc;
  m_srb0->m_srbIdentity = 0;
  LteUeRrcSapUser::SetupParameters ueParams;
  ueParams.srb0SapProvider = m_srb0->m_rlc->GetLteRlcSapProvider ();
  ueParams.srb1SapProvider = 0;
  m_rrcSapUser->Setup (ueParams);

  // CCCH (LCID 0) is pre-configured, here is the hardcoded configuration:
  LteUeCmacSapProvider::LogicalChannelConfig lcConfig;
  lcConfig.priority = 0; // highest priority
  lcConfig.prioritizedBitRateKbps = 65535; // maximum
  lcConfig.bucketSizeDurationMs = 65535; // maximum
  lcConfig.logicalChannelGroup = 0; // all SRBs mapped to LCG 0
  m_cmacSapProvider.at(0)->AddLc (lcid, lcConfig, rlc->GetLteMacSapUser ());
}

void
LteUeRrc::InitializeSap (void)
{
  if (m_numberOfComponentCarriers < MIN_NO_CC || m_numberOfComponentCarriers > MAX_NO_CC)
    {
      // this check is neede in order to maintain backward compatibility with scripts and tests
      // if case lte-helper is not used (like in several tests) the m_numberOfComponentCarriers
      // is not set and then an error is rised
      // In this case m_numberOfComponentCarriers is set to 1
      m_numberOfComponentCarriers = MIN_NO_CC;
    }
  if (m_numberOfComponentCarriers > MIN_NO_CC )
    {
      for ( uint16_t i = 1; i < m_numberOfComponentCarriers; i++)
        {
          m_cphySapUser.push_back(new MemberLteUeCphySapUser<LteUeRrc> (this));
          m_cmacSapUser.push_back(new UeMemberLteUeCmacSapUser (this));
          m_cphySapProvider.push_back(0);
          m_cmacSapProvider.push_back(0);
        }
    }
}


void
LteUeRrc::DoSendData (Ptr<Packet> packet, uint8_t bid)
{
  NS_LOG_FUNCTION (this << packet);
  std::cout<<Simulator::Now().GetSeconds()<<" "<<GetRnti()<<" LteUeRrc::DoSendData "<<g_ueRrcStateName[m_state]<<" "<<packet->GetSize()<<" UID: "<<packet->GetUid() <<std::endl;

  uint8_t drbid = Bid2Drbid (bid);


  if (drbid != 0)
  {
      std::map<uint8_t, Ptr<LteDataRadioBearerInfo> >::iterator it =   m_drbMap.find (drbid);
      NS_ASSERT_MSG (it != m_drbMap.end (), "could not find bearer with drbid == " << drbid);

      LtePdcpSapProvider::TransmitPdcpSduParameters params;
      params.pdcpSdu = packet;
      params.rnti = m_rnti;
      params.lcid = it->second->m_logicalChannelIdentity;

      NS_LOG_LOGIC (this << " RNTI=" << m_rnti << " sending packet " << packet
                         << " on DRBID " << (uint32_t) drbid
                         << " (LCID " << (uint32_t) params.lcid << ")"
                         << " (" << packet->GetSize () << " bytes)");
      if(IDLE_PSM == m_state  || IDLE_SUSPEND == m_state)
      {
          SwitchToState(CONNECTED_NORMALLY);
      }
      it->second->m_pdcp->GetLtePdcpSapProvider ()->TransmitPdcpSdu (params);
  }
}


void
LteUeRrc::DoDisconnect ()
{
  NS_LOG_FUNCTION (this);

  switch (m_state)
    {
    case IDLE_START:
    case IDLE_CELL_SEARCH:
    case IDLE_WAIT_MIB_SIB1:
    case IDLE_WAIT_MIB:
    case IDLE_WAIT_SIB1:
    case IDLE_CAMPED_NORMALLY:
      NS_LOG_INFO ("already disconnected");
      break;

    case IDLE_WAIT_SIB2:
    case IDLE_CONNECTING:
      NS_FATAL_ERROR ("cannot abort connection setup procedure");
      break;

    case CONNECTED_NORMALLY:
    case CONNECTED_HANDOVER:
    case CONNECTED_PHY_PROBLEM:
    case CONNECTED_REESTABLISHING:
      LeaveConnectedMode ();
      break;

    default: // i.e. IDLE_RANDOM_ACCESS
      NS_FATAL_ERROR ("method unexpected in state " << ToString (m_state));
      break;
    }
}

void
LteUeRrc::DoReceivePdcpSdu (LtePdcpSapUser::ReceivePdcpSduParameters params)
{
  NS_LOG_FUNCTION (this);
  std::cout<< Simulator::Now().GetSeconds()<<" "<<GetRnti()<<" LteUeRrc::DoReceivePdcpSdu "<<m_state<<" "<<params.pdcpSdu->GetSize()<<" UID: "<<params.pdcpSdu->GetUid()<<std::endl;
  double idleTime= ((Simulator::Now()-m_lastUpdateTime).GetNanoSeconds());
  Callback<void, double> two;
  two = MakeCallback(&EnergyModuleLte::Only_downlink_rx, &LEM);
  m_lastUpdateTime=Simulator::Now();
  std::cout<<Simulator::Now().GetSeconds()<<" "<<GetRnti() <<" ";
  two(idleTime);
  m_asSapUser->RecvData (params.pdcpSdu);
}


void
LteUeRrc::DoSetTemporaryCellRnti (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << rnti);
  m_rnti = rnti;
  m_srb0->m_rlc->SetRnti (m_rnti);
  m_cphySapProvider.at(0)->SetRnti (m_rnti);
}

void
LteUeRrc::DoNotifyEnergyChange ()
{
    // TODO: Update according to Mac layer
    double idleTime= ((Simulator::Now()-m_lastUpdateTime).GetNanoSeconds());
    Callback<void, double> two;
    std::cout<<Simulator::Now().GetSeconds()<<" "<<GetRnti() <<" ";
    two = MakeCallback(&EnergyModuleLte::Only_uplink_tx, &LEM);
    m_lastUpdateTime=Simulator::Now();
    two(idleTime);
}

void LteUeRrc::DoSetFrameSubframe(uint32_t frame, uint32_t subframe)
{
    NS_LOG_FUNCTION (this << frame << subframe);
    m_frameNo = frame;
    m_subframeNo = subframe;
    if (m_pf == m_frameNo && m_po == m_subframeNo)
        std::cout<<m_hasReceivedPaging<<" "<<m_pf<<" "<<m_frameNo<<" "<<m_po<<" "<<m_subframeNo<<" "<<m_state<<std::endl;
    NS_LOG_FUNCTION (this << "Waiting for paging occasion" << m_hasReceivedPaging << m_pf << m_frameNo << m_po << m_subframeNo);
    //std::cout<<m_pf<<" == "<<m_frameNo<<" "<<m_po<<" == "<<m_subframeNo<<" "<<m_hasReceivedPaging<<std::endl;
    if(m_hasReceivedPaging && m_pf == m_frameNo && m_po == m_subframeNo && IDLE_PAGING == m_state)
    {
        m_hasReceivedPaging = false;
        StartConnection ();
    }

    if (IDLE_SUSPEND == m_state)
    {
        --m_t3324;
        --m_edrx_cycle;
        --m_t3412;
        if (m_edrx_cycle <0)
        {
            m_edrx_cycle = m_edrx_cycle_d;
        }
        //NS_LOG_UNCOND( Simulator::Now().GetSeconds()<<" LteUeRrc::DoSetFrameSubframe "<<"m_t3324 "<<m_t3324<<"m_t3412 "<<m_t3412<<" m_edrx_cycle "<<m_edrx_cycle<<" m_hasReceivedPaging "<<m_hasReceivedPaging);
        if(m_edrx_cycle ==0)  //Paging
        {
            SwitchToState (SUSPEND_PAGING);
            m_hasReceivedPaging = false;
        }
        else if(m_t3324 <= 0)  //DL case
        {
            SwitchToState (SUSPEND_PAGING);
            m_hasReceivedPaging = false;
        }

    }

    else if(IDLE_PSM == m_state)
    {
        --m_t3412;
        if ( 0 >= (m_t3412 - m_t3324) )
        {
            SwitchToState(SUSPEND_PAGING);
        }
    }

}
void
LteUeRrc::DoNotifyRandomAccessSuccessful ()
{
  NS_LOG_FUNCTION (this << m_imsi << ToString (m_state));
  m_randomAccessSuccessfulTrace (m_imsi, m_cellId, m_rnti);

  switch (m_state)
    {
    case IDLE_RANDOM_ACCESS:
      {
        // we just received a RAR with a T-C-RNTI and an UL grant
        // send RRC connection request as message 3 of the random access procedure 
        SwitchToState (IDLE_CONNECTING);
        LteRrcSap::RrcConnectionRequest msg;
        msg.ueIdentity = m_imsi;
        m_rrcSapUser->SendRrcConnectionRequest (msg); 
        m_connectionTimeout = Simulator::Schedule (m_t300,
                                                   &LteUeRrc::ConnectionTimeout,
                                                   this);
      }
      break;

    case CONNECTED_HANDOVER:
      {
        LteRrcSap::RrcConnectionReconfigurationCompleted msg;
        msg.rrcTransactionIdentifier = m_lastRrcTransactionIdentifier;
        m_rrcSapUser->SendRrcConnectionReconfigurationCompleted (msg);

        // 3GPP TS 36.331 section 5.5.6.1 Measurements related actions upon handover
        std::map<uint8_t, LteRrcSap::MeasIdToAddMod>::iterator measIdIt;
        for (measIdIt = m_varMeasConfig.measIdList.begin ();
             measIdIt != m_varMeasConfig.measIdList.end ();
             ++measIdIt)
          {
            VarMeasReportListClear (measIdIt->second.measId);
          }

        SwitchToState (CONNECTED_NORMALLY);
        m_handoverEndOkTrace (m_imsi, m_cellId, m_rnti);
      }
      break;

    default:
      NS_FATAL_ERROR ("unexpected event in state " << ToString (m_state));
      break; 
    }
}

void
LteUeRrc::DoNotifyRandomAccessFailed ()
{
  NS_LOG_FUNCTION (this << m_imsi << ToString (m_state));
  m_randomAccessErrorTrace (m_imsi, m_cellId, m_rnti);

  switch (m_state)
    {
    case IDLE_RANDOM_ACCESS:
      {
        SwitchToState (IDLE_CAMPED_NORMALLY);
        m_asSapUser->NotifyConnectionFailed ();
      }
      break;

    case CONNECTED_HANDOVER:
      {
        m_handoverEndErrorTrace (m_imsi, m_cellId, m_rnti);
        /**
         * \todo After a handover failure because of a random access failure,
         *       send an RRC Connection Re-establishment and switch to
         *       CONNECTED_REESTABLISHING state.
         */
      }
      break;

    default:
      NS_FATAL_ERROR ("unexpected event in state " << ToString (m_state));
      break;
    }
}


void
LteUeRrc::DoSetCsgWhiteList (uint32_t csgId)
{
  NS_LOG_FUNCTION (this << m_imsi << csgId);
  m_csgWhiteList = csgId;
}

void 
LteUeRrc::DoStartCellSelection (uint32_t dlEarfcn)
{
  NS_LOG_FUNCTION (this << m_imsi << dlEarfcn);
  NS_ASSERT_MSG (m_state == IDLE_START,
                 "cannot start cell selection from state " << ToString (m_state));
  m_dlEarfcn = dlEarfcn;
  m_cphySapProvider.at(0)->StartCellSearch (dlEarfcn);
  SwitchToState (IDLE_CELL_SEARCH);
}

void 
LteUeRrc::DoForceCampedOnEnb (uint16_t cellId, uint32_t dlEarfcn)
{
  NS_LOG_FUNCTION (this << m_imsi << cellId << dlEarfcn);

  switch (m_state)
    {
    case IDLE_START:
      m_cellId = cellId;
      m_dlEarfcn = dlEarfcn;
      m_cphySapProvider.at(0)->SynchronizeWithEnb (m_cellId, m_dlEarfcn);
      SwitchToState (IDLE_WAIT_MIB);
      break;

    case IDLE_CELL_SEARCH:
    case IDLE_WAIT_MIB_SIB1:
    case IDLE_WAIT_SIB1:
      NS_FATAL_ERROR ("cannot abort cell selection " << ToString (m_state));
      break;

    case IDLE_WAIT_MIB:
      NS_LOG_INFO ("already forced to camp to cell " << m_cellId);
      break;

    case IDLE_CAMPED_NORMALLY:
    case IDLE_WAIT_SIB2:
    case IDLE_RANDOM_ACCESS:
    case IDLE_CONNECTING:
      NS_LOG_INFO ("already camped to cell " << m_cellId);
      break;

    case CONNECTED_NORMALLY:
    case CONNECTED_HANDOVER:
    case CONNECTED_PHY_PROBLEM:
    case CONNECTED_REESTABLISHING:
      NS_LOG_INFO ("already connected to cell " << m_cellId);
      break;

    default:
      NS_FATAL_ERROR ("unexpected event in state " << ToString (m_state));
      break;
    }

}

void
LteUeRrc::DoConnect ()
{
  NS_LOG_FUNCTION (this << m_imsi);

  switch (m_state)
    {
    case IDLE_START:
    case IDLE_CELL_SEARCH:
    case IDLE_WAIT_MIB_SIB1:
    case IDLE_WAIT_SIB1:
    case IDLE_WAIT_MIB:
      m_connectionPending = true;
      break;

    case IDLE_CAMPED_NORMALLY:
      m_connectionPending = true;
      SwitchToState (IDLE_WAIT_SIB2);
      break;

    case IDLE_WAIT_SIB2:
    case IDLE_RANDOM_ACCESS:
    case IDLE_CONNECTING:
      NS_LOG_INFO ("already connecting");
      break;

    case CONNECTED_NORMALLY:
    case CONNECTED_REESTABLISHING:
    case CONNECTED_HANDOVER:
      NS_LOG_INFO ("already connected");
      break;

    default:
      NS_FATAL_ERROR ("unexpected event in state " << ToString (m_state));
      break;
    }
}



// CPHY SAP methods
void
LteUeRrc::DoRecvPagingInformation (uint16_t cellId,
                                        LteRrcSap::PagingInformation msg)
{
    NS_LOG_FUNCTION (this << cellId);
    //NS_LOG_UNCOND("******************************************"<<m_hasReceivedPaging);
    m_hasReceivedPaging = true;

}

void
LteUeRrc::DoRecvMasterInformationBlock (uint16_t cellId,
                                        LteRrcSap::MasterInformationBlock msg)
{ 
  m_dlBandwidth = msg.dlBandwidth;
  m_cphySapProvider.at(0)->SetDlBandwidth (msg.dlBandwidth);
  m_hasReceivedMib = true;
  m_mibReceivedTrace (m_imsi, m_cellId, m_rnti, cellId);

  switch (m_state)
    {
    case IDLE_WAIT_MIB:
      // manual attachment
      SwitchToState (IDLE_CAMPED_NORMALLY);
      break;

    case IDLE_WAIT_MIB_SIB1:
      // automatic attachment from Idle mode cell selection
      SwitchToState (IDLE_WAIT_SIB1);
      break;

    default:
      // do nothing extra
      break;
    }
}

void
LteUeRrc::DoRecvSystemInformationBlockType1 (uint16_t cellId,
                                             LteRrcSap::SystemInformationBlockType1 msg)
{
  NS_LOG_FUNCTION (this);
  switch (m_state)
    {
    case IDLE_WAIT_SIB1:
      NS_ASSERT_MSG (cellId == msg.cellAccessRelatedInfo.cellIdentity,
                     "Cell identity in SIB1 does not match with the originating cell");
      m_hasReceivedSib1 = true;
      m_lastSib1 = msg;
      m_sib1ReceivedTrace (m_imsi, m_cellId, m_rnti, cellId);
      EvaluateCellForSelection ();
      break;

    case IDLE_CAMPED_NORMALLY:
    case IDLE_RANDOM_ACCESS:
    case IDLE_CONNECTING:
    case CONNECTED_NORMALLY:
    case CONNECTED_HANDOVER:
    case CONNECTED_PHY_PROBLEM:
    case CONNECTED_REESTABLISHING:
      NS_ASSERT_MSG (cellId == msg.cellAccessRelatedInfo.cellIdentity,
                     "Cell identity in SIB1 does not match with the originating cell");
      m_hasReceivedSib1 = true;
      m_lastSib1 = msg;
      m_sib1ReceivedTrace (m_imsi, m_cellId, m_rnti, cellId);
      break;

    case IDLE_WAIT_MIB_SIB1:
      // MIB has not been received, so ignore this SIB1
      break;

    default: // e.g. IDLE_START, IDLE_CELL_SEARCH, IDLE_WAIT_MIB, IDLE_WAIT_SIB2
      // do nothing
      break;
    }
}


/*
 * todo
 * Method uses to enable the NB-IoT management mode on the UE RRC.
 */
void
LteUeRrc::EnableNbIotUeManager()
{
  m_nbIotActiveMode = true;
}

/*
 * todo
 * NB-IoT Method DoRecvMasterInformationBlockNb.
 *
 * This method in future will be used by an another class. Cause we need to operate
 * with a Cat NB1 device that has a different management of the RRC protocol.
 * Now, to test the signaling traffic module, we are integrating this method to the
 * legacy LTE RRC layer, UE side.
 * Notice that the eNB RRC layer needs to manage both UE category, so it will be
 * one class only.
 *
 * If the NB-IoT module is enabled (this job is made by the boolean variable
 * m_EnabledNbIot in the LteEnbPhy class) this method is automatically set as
 * default and it is not possible that the DoRecvMasterInformationBlock method
 * is invoked in its place.
 */
void
LteUeRrc::DoRecvMasterInformationBlockNb (uint16_t cellId,
                                        NbLteRrcSap::MasterInformationBlockNb msg)	// Needed to manage NB-IoT connections. 3GPP Release 13.
{
  switch(msg.schedulingInfoSib1)
  {
    case 0:
    case 3:
    case 6:
    case 9:
      m_numberOfNpdschRepetitions = 4;
      break;

    case 1:
    case 4:
    case 7:
    case 10:
      m_numberOfNpdschRepetitions = 8;
      break;

    default:
      m_numberOfNpdschRepetitions = 16;
      break;
  }

  /*
   * This is the transport block size of the SIB1-NB message. This value must be set also in the
   * schedulingInfoListNb.siTb parameter, inside a SIB1-NB message. To get more information
   * about this see the declaration of the m_transportBlockSize attribute.
   */
  if(msg.schedulingInfoSib1 < 3)
    {
      m_transportBlockSize = 280;
    }else if(msg.schedulingInfoSib1 < 6)
    {
      m_transportBlockSize = 328;

    }else if(msg.schedulingInfoSib1 < 9)
    {
      m_transportBlockSize = 440;

    }else
    {
      m_transportBlockSize = 680;
    }

  /*
   * todo
   * This is an unused attribute because we used the Legacy LTE structure to make first tests.
   * So we have to set also the m_hasReceivedMib to reuse the LTE finite state machine. This is
   * not a problem cause the methods DoRecvMasterInformationBlock and DoRecvMasterInformationBlockNb
   * are mutually exclusive.
   */
  m_hasReceivedMibNb = true;
  m_hasReceivedMib = true;


  /*
   * m_dlBandwidth attribute represents the downlink bandwidth in RBs. We know that NB-IoT uses one
   * RB only, so we must set this value to 1. Notice that this value is not sent over the MIB-NB message
   * cause it is understood.
   */
  m_dlBandwidth = 1;
  m_cphySapProvider.at(0)->SetDlBandwidth (m_dlBandwidth);

  /*
   * todo
   * The configuration is finished. Now we can set the legacy m_mibReceivedTrace. In future this
   * configuration will vary.
   * Notice that also Cat NB1 devices need to have a tmsi (Temporary Mobile Subscriber Identity),
   * but it does not have a SIM card, so it can not have a pure imsi identifier. Actually we can
   * leave the default value to make some tests, but this idea will change.
   */
  m_mibReceivedTrace (m_imsi, m_cellId, m_rnti, cellId);

  /*
   * The state flow will be the same of the legacy LTE, but the manual attachment in NB-IoT is a little
   * bit different from the LTE one, so we consider the automatic attachment only.
   */
  switch (m_state)
    {
      case IDLE_WAIT_MIB:
      case IDLE_WAIT_MIB_SIB1:
        // automatic attachment from Idle mode cell selection
        SwitchToState (IDLE_WAIT_SIB1);
        break;

      default:
        // do nothing extra
        break;
    }

}


/*
 * todo
 * NB-IoT Method DoRecvSystemInformationBlockType1Nb.
 *
 * This method in future will be used by an another class. Cause we need to operate
 * with a Cat NB1 device that has a different management of the RRC protocol.
 * Now, to test the signaling traffic module, we are integrating this method to the
 * legacy LTE RRC layer, UE side.
 * Notice that the eNB RRC layer needs to manage both UE category, so it will be
 * one class only.
 *
 * If the NB-IoT module is enabled (this job is made by the boolean variable
 * m_EnabledNbIot in the LteEnbPhy class) this method is automatically set as
 * default and it is not possible that the DoRecvSystemInformationBlockType1
 * method is invoked in its place.
 */
void
LteUeRrc::DoRecvSystemInformationBlockType1Nb (uint16_t cellId, NbLteRrcSap::SystemInformationBlockType1Nb msg)
{
  NS_LOG_FUNCTION (this);
  switch (m_state)
    {
    case IDLE_WAIT_SIB1:
      {
        NS_ASSERT_MSG (cellId == msg.cellAccessRelatedInfo.cellIdentity,
                       "Cell identity in SIB1-NB does not match with the originating cell");
        /*
         * todo
         * This is an unused attribute because we used the Legacy LTE structure to make first tests.
         * So we have to set also the m_hasReceivedSib1 to reuse the LTE finite state machine. This is
         * not a problem cause the methods DoRecvSystemInformationBlockType1 and
         * DoRecvSystemInformationBlockType1Nb are mutually exclusive.
         */
        m_hasReceivedSib1Nb=true;
        m_hasReceivedSib1 = true;

        /*
         * We need a coherent structure to stored the last SIB1-NB message received.
         */
        m_lastSib1Nb = msg;

        /*
         * todo
         * The configuration is finished. Now we can set the legacy m_sib1ReceivedTrace. In future this
         * configuration will vary.
         * Notice that also Cat NB1 devices need to have a tmsi (Temporary Mobile Subscriber Identity),
         * but it does not have a SIM card, so it can not have a pure imsi identifier. Actually we can
         * leave the default value to make some tests, but this idea will change.
         */
        m_sib1ReceivedTrace (m_imsi, m_cellId, m_rnti, cellId);

        /*
         * We can't use the EvaluateCellForSelection method. We need to override this.
         */
        // BEGIN EVALUATE CELL FOR SELECTION
        NS_ASSERT (m_state == IDLE_WAIT_SIB1);
        NS_ASSERT (m_hasReceivedMibNb);
        NS_ASSERT (m_hasReceivedSib1Nb);

        // Cell selection criteria evaluation

        bool isSuitableCell = false;
        bool isAcceptableCell = false;
        std::map<uint16_t, MeasValues>::iterator storedMeasIt = m_storedMeasValues.find (m_lastSib1Nb.cellAccessRelatedInfo.cellIdentity);
        double qRxLevMeas = storedMeasIt->second.rsrp;

        /// Note that we must specify the qRxLevMin for NB-IoT that will be different from legacy LTE.
        /// We print into the LOG console also the value of the current TBS of a SIB1-NB message.
        double qRxLevMin = EutranMeasurementMapping::IeValue2ActualQRxLevMin (m_lastSib1Nb.cellSelectionInfo.qRxLevMin);
        NS_LOG_LOGIC (this << " cell selection to cellId=" << m_lastSib1Nb.cellAccessRelatedInfo.cellIdentity
                           << " qrxlevmeas=" << qRxLevMeas << " dBm"
                           << " qrxlevmin=" << qRxLevMin << " dBm"
                           << " sib1NbTransportBlockSize="<< m_transportBlockSize <<" RB");

        if (qRxLevMeas - qRxLevMin > 0)
          {
            isAcceptableCell = true;

            // CSG informations are not present in NB-IoT, so this is enough.
            isSuitableCell = true;
          }

        // Cell selection decision

        if (isSuitableCell)
          {
            m_cellId = m_lastSib1Nb.cellAccessRelatedInfo.cellIdentity;
            m_cphySapProvider.at(0)->SynchronizeWithEnb (m_lastSib1Nb.cellAccessRelatedInfo.cellIdentity, m_dlEarfcn);
            m_cphySapProvider.at(0)->SetDlBandwidth (m_dlBandwidth);
            m_initialCellSelectionEndOkTrace (m_imsi, m_lastSib1Nb.cellAccessRelatedInfo.cellIdentity);
            SwitchToState (IDLE_CAMPED_NORMALLY);
          }
        else
          {
            // ignore the MIB-NB and SIB1-NB received from this cell
            m_hasReceivedMibNb = false;
            m_hasReceivedSib1Nb = false;
            m_hasReceivedMib = false;
            m_hasReceivedSib1 = false;

            m_initialCellSelectionEndErrorTrace (m_imsi, m_lastSib1Nb.cellAccessRelatedInfo.cellIdentity);

            if (isAcceptableCell)
              {
                /*
                 * The cells inserted into this list will not be considered for
                 * subsequent cell search attempt.
                 */
                m_acceptableCell.insert (m_lastSib1Nb.cellAccessRelatedInfo.cellIdentity);
              }

            SwitchToState (IDLE_CELL_SEARCH);
            SynchronizeToStrongestCell (); // retry to a different cell
          }
          //END EVALUATE CELL FOR SELECTION
        }
      break; /// END CASE IDLE_WAIT_SIB1

    case IDLE_CAMPED_NORMALLY:
    case IDLE_RANDOM_ACCESS:
    case IDLE_CONNECTING:
    case CONNECTED_NORMALLY:
    case CONNECTED_HANDOVER:
    case CONNECTED_PHY_PROBLEM:
    case CONNECTED_REESTABLISHING:
      {
          NS_ASSERT_MSG (cellId == msg.cellAccessRelatedInfo.cellIdentity,
                         "Cell identity in SIB1-NB does not match with the originating cell");

          /*
           * todo
           * This is an unused attribute because we used the Legacy LTE structure to make first tests.
           * So we have to set also the m_hasReceivedSib1 to reuse the LTE finite state machine. This is
           * not a problem cause the methods DoRecvSystemInformationBlockType1 and
           * DoRecvSystemInformationBlockType1Nb are mutually exclusive.
           */
          m_hasReceivedSib1Nb=true;
          m_hasReceivedSib1 = true;
          m_lastSib1Nb = msg;
          m_sib1ReceivedTrace (m_imsi, m_cellId, m_rnti, cellId);
        }
      break;

    default: // e.g. IDLE_START, IDLE_CELL_SEARCH, IDLE_WAIT_MIB, IDLE_WAIT_SIB2, IDLE_WAIT_MIB_SIB1
      // do nothing
      break;
    }
}

void
LteUeRrc::DoReportUeMeasurements (LteUeCphySapUser::UeMeasurementsParameters params)
{
  NS_LOG_FUNCTION (this);

  // layer 3 filtering does not apply in IDLE mode
  bool useLayer3Filtering = (m_state == CONNECTED_NORMALLY);
  bool triggering = true;
  std::vector <LteUeCphySapUser::UeMeasurementsElement>::iterator newMeasIt;
  for (newMeasIt = params.m_ueMeasurementsList.begin ();
       newMeasIt != params.m_ueMeasurementsList.end (); ++newMeasIt)
    {
      if (params.m_componentCarrierId != 0)
        {
          triggering = false; // report is triggered only when an event is on the primary carrier
          // in this case the measurement received is related to secondary carriers
          // measurements related to secondary carriers are saved on a different portion of memory
          SaveScellUeMeasurements (newMeasIt->m_cellId, newMeasIt->m_rsrp,
                                   newMeasIt->m_rsrq, useLayer3Filtering, 
                                   params.m_componentCarrierId );
        }
      else
        {
          SaveUeMeasurements (newMeasIt->m_cellId, newMeasIt->m_rsrp,
                              newMeasIt->m_rsrq, useLayer3Filtering);
        }
    }

  if (m_state == IDLE_CELL_SEARCH)
    {
      // start decoding BCH
      SynchronizeToStrongestCell ();
    }
  else
    {
      if (triggering)
        {
          std::map<uint8_t, LteRrcSap::MeasIdToAddMod>::iterator measIdIt;
          for (measIdIt = m_varMeasConfig.measIdList.begin ();
               measIdIt != m_varMeasConfig.measIdList.end (); ++measIdIt)
            {
              MeasurementReportTriggering (measIdIt->first);
             }
        }
    }

} // end of LteUeRrc::DoReportUeMeasurements



// RRC SAP methods

void
LteUeRrc::DoCompleteSetup (LteUeRrcSapProvider::CompleteSetupParameters params)
{
  NS_LOG_FUNCTION (this << " RNTI " << m_rnti);
  m_srb0->m_rlc->SetLteRlcSapUser (params.srb0SapUser);
  if (m_srb1)
    {
      m_srb1->m_pdcp->SetLtePdcpSapUser (params.srb1SapUser);
    }
}


void LteUeRrc::DoPaging()
{

    uint16_t ue_id = m_imsi%1024;
    uint16_t N = std::min(m_T,m_nb);
    //uint16_t RHS = (m_T/N) * (ue_id%N);
    int32_t temp = int32_t((m_nb/m_T));
    int32_t Ns = (std::max(1,temp));
    int32_t i_s = int32_t(floor(ue_id/N))%Ns;

    if (0 == i_s)
    {
        switch(Ns)
        {
            case 1:
                m_po = 9;
                break;
            case 2:
                m_po = 4;
                break;
            case 4:
                m_po = 0;
                break;
        }
    }
    else if (1 == i_s)
    {
        switch(Ns)
        {
            case 2:
                m_po = 9;
                break;
            case 4:
                m_po = 4;
                break;
        }
    }
    else if (2 == i_s && 4 == Ns)
    {
        m_po = 5;
    }
    else if (3 == i_s && 4 == Ns)
    {
        m_po = 9;
    }
    else
    {
        m_po = 0; // Assumed for "Not define"
    }


    /*if (m_requirepagingflag)
    {
        m_pf = (m_pf) + m_T;
    }
    else
    {
        m_pf =  RHS;
    }*/
    m_pf = m_frameNo;

    NS_LOG_FUNCTION (this << "pf" << m_pf <<"po" << m_po);

    NS_ASSERT (m_connectionPending);
    SwitchToState (IDLE_PAGING);

    //NS_LOG_FUNCTION (this << "Waiting for paging occasion" << m_hasReceivedPaging << m_pf << m_frameNo << m_po << m_subframeNo);
    /*if(m_hasReceivedPaging && m_pf == m_frameNo && m_po == m_subframeNo)
    {
        m_hasReceivedPaging = false;
        StartConnection ();
    }*/


}

void 
LteUeRrc::DoRecvSystemInformation (LteRrcSap::SystemInformation msg)
{
  NS_LOG_FUNCTION (this << " RNTI " << m_rnti << m_state);

  if (msg.haveSib2)
    {
      switch (m_state)
        {
        case IDLE_CAMPED_NORMALLY:
        case IDLE_WAIT_SIB2:
        case IDLE_RANDOM_ACCESS:
        case IDLE_CONNECTING:
        case CONNECTED_NORMALLY:
        case CONNECTED_HANDOVER:
        case CONNECTED_PHY_PROBLEM:
        case CONNECTED_REESTABLISHING:
          m_hasReceivedSib2 = true;
          m_ulBandwidth = msg.sib2.freqInfo.ulBandwidth;
          m_ulEarfcn = msg.sib2.freqInfo.ulCarrierFreq;
          m_sib2ReceivedTrace (m_imsi, m_cellId, m_rnti);
          LteUeCmacSapProvider::RachConfig rc;
          rc.numberOfRaPreambles = msg.sib2.radioResourceConfigCommon.rachConfigCommon.preambleInfo.numberOfRaPreambles;
          rc.preambleTransMax = msg.sib2.radioResourceConfigCommon.rachConfigCommon.raSupervisionInfo.preambleTransMax;
          rc.raResponseWindowSize = msg.sib2.radioResourceConfigCommon.rachConfigCommon.raSupervisionInfo.raResponseWindowSize;
          m_cmacSapProvider.at (0)->ConfigureRach (rc);
          m_cphySapProvider.at (0)->ConfigureUplink (m_ulEarfcn, m_ulBandwidth);
          m_cphySapProvider.at (0)->ConfigureReferenceSignalPower (msg.sib2.radioResourceConfigCommon.pdschConfigCommon.referenceSignalPower);
          if (m_state == IDLE_WAIT_SIB2)
            {
              m_T   = 128;
              m_nb  = 1*m_T;
              DoPaging();

            }
          break;

        default: // IDLE_START, IDLE_CELL_SEARCH, IDLE_WAIT_MIB, IDLE_WAIT_MIB_SIB1, IDLE_WAIT_SIB1
          // do nothing
          break;
        }
    }

}

/*
void 
LteUeRrc::DoRecvSystemInformationNb (NbLteRrcSap::SystemInformationNb msg)
{
  NS_LOG_FUNCTION (this << " RNTI " << m_rnti << m_state);

  if (msg.haveSib2)
    {
      switch (m_state)
        {
        case IDLE_CAMPED_NORMALLY:
        case IDLE_WAIT_SIB2:
        case IDLE_RANDOM_ACCESS:
        case IDLE_CONNECTING:
        case CONNECTED_NORMALLY:
        case CONNECTED_HANDOVER:
        case CONNECTED_PHY_PROBLEM:
        case CONNECTED_REESTABLISHING:
          m_hasReceivedSib2 = true;
          m_ulBandwidth = msg.sib2.freqInfo.ulBandwidth;
          m_ulEarfcn = msg.sib2.freqInfo.ulCarrierFreqNb;
          m_sib2ReceivedTrace (m_imsi, m_cellId, m_rnti);

          LteUeCmacSapProvider::RachConfig rc;

          rc.numberOfRaPreambles = msg.sib2.radioResourceConfigCommonSibNb.rachConfigCommonNb.preambleInfo.numberOfRaPreambles;
          rc.preambleTransMax = msg.sib2.radioResourceConfigCommonSibNb.rachConfigCommonNb.raSupervisionInfo.preambleTransMax;
          rc.raResponseWindowSize = msg.sib2.radioResourceConfigCommonSibNb.rachConfigCommonNb.raSupervisionInfo.raResponseWindowSize;

          m_cmacSapProvider.at (0)->ConfigureRach (rc);
          m_cphySapProvider.at (0)->ConfigureUplink (m_ulEarfcn, m_ulBandwidth);

          //m_cphySapProvider.at (0)->ConfigureReferenceSignalPower (msg.sib2.radioResourceConfigCommonNb.pdschConfigCommon.referenceSignalPower);
          if (m_state == IDLE_WAIT_SIB2)
            {
              m_T   = 128;
              m_nb  = 1*m_T;
              DoPaging();

            }
          break;

        default: // IDLE_START, IDLE_CELL_SEARCH, IDLE_WAIT_MIB, IDLE_WAIT_MIB_SIB1, IDLE_WAIT_SIB1
          // do nothing
          break;
        }
    }

}

*/

void
LteUeRrc::DoRecvRrcConnectionSetup (LteRrcSap::RrcConnectionSetup msg)
{
  NS_LOG_FUNCTION (this << " RNTI " << m_rnti);
  switch (m_state)
    {
    case IDLE_CONNECTING:
    case SUSPEND_PAGING:
      {
        ApplyRadioResourceConfigDedicated (msg.radioResourceConfigDedicated);
        m_connectionTimeout.Cancel ();
        SwitchToState (CONNECTED_NORMALLY);
        LteRrcSap::RrcConnectionSetupCompleted msg2;
        msg2.rrcTransactionIdentifier = msg.rrcTransactionIdentifier;
        m_rrcSapUser->SendRrcConnectionSetupCompleted (msg2);
        m_asSapUser->NotifyConnectionSuccessful ();
        m_connectionEstablishedTrace (m_imsi, m_cellId, m_rnti);
      }
      break;

    default:
      NS_FATAL_ERROR ("method unexpected in state " << ToString (m_state));
      break;
    }
}

void
LteUeRrc::DoRecvRrcConnectionReconfiguration (LteRrcSap::RrcConnectionReconfiguration msg)
{
  NS_LOG_FUNCTION (this << " RNTI " << m_rnti);
  NS_LOG_INFO ("DoRecvRrcConnectionReconfiguration haveNonCriticalExtension:" << msg.haveNonCriticalExtension );
  switch (m_state)
    {
    case CONNECTED_NORMALLY:


          if ((msg.haveMobilityControlInfo)&&(m_nbIotActiveMode==false))
            {
              NS_LOG_INFO ("haveMobilityControlInfo == true");
              SwitchToState (CONNECTED_HANDOVER);
              const LteRrcSap::MobilityControlInfo& mci = msg.mobilityControlInfo;
              m_handoverStartTrace (m_imsi, m_cellId, m_rnti, mci.targetPhysCellId);
              m_cmacSapProvider.at(0)->Reset ();
              m_cphySapProvider.at(0)->Reset ();
              m_cellId = mci.targetPhysCellId;
              NS_ASSERT (mci.haveCarrierFreq);
              NS_ASSERT (mci.haveCarrierBandwidth);
              m_cphySapProvider.at(0)->SynchronizeWithEnb (m_cellId, mci.carrierFreq.dlCarrierFreq);
              m_cphySapProvider.at(0)->SetDlBandwidth ( mci.carrierBandwidth.dlBandwidth);
              m_cphySapProvider.at(0)->ConfigureUplink (mci.carrierFreq.ulCarrierFreq, mci.carrierBandwidth.ulBandwidth);
              m_rnti = msg.mobilityControlInfo.newUeIdentity;
              m_srb0->m_rlc->SetRnti (m_rnti);
              NS_ASSERT_MSG (mci.haveRachConfigDedicated, "handover is only supported with non-contention-based random access procedure");
              m_cmacSapProvider.at(0)->StartNonContentionBasedRandomAccessProcedure (m_rnti, mci.rachConfigDedicated.raPreambleIndex, mci.rachConfigDedicated.raPrachMaskIndex);
              m_cphySapProvider.at(0)->SetRnti (m_rnti);
              m_lastRrcTransactionIdentifier = msg.rrcTransactionIdentifier;
              NS_ASSERT (msg.haveRadioResourceConfigDedicated);

              // we re-establish SRB1 by creating a new entity
              // note that we can't dispose the old entity now, because
              // it's in the current stack, so we would corrupt the stack
              // if we did so. Hence we schedule it for later disposal
              m_srb1Old = m_srb1;
              Simulator::ScheduleNow (&LteUeRrc::DisposeOldSrb1, this);
              m_srb1 = 0; // new instance will be be created within ApplyRadioResourceConfigDedicated

              m_drbMap.clear (); // dispose all DRBs
              ApplyRadioResourceConfigDedicated (msg.radioResourceConfigDedicated);

              if (msg.haveMeasConfig)
                {
                  ApplyMeasConfig (msg.measConfig);
                }
              // RRC connection reconfiguration completed will be sent
              // after handover is complete
            }
          else
            {
              if(m_nbIotActiveMode)
                {
                  NS_LOG_FUNCTION (this << " NB-IoT mode does not support handover. Provide an RRC reconfiguration.");
                }

              NS_LOG_INFO ("haveMobilityControlInfo == false");
              if (msg.haveNonCriticalExtension)
                {
                  ApplyRadioResourceConfigDedicatedSecondaryCarrier (msg.nonCriticalExtension);
                  NS_LOG_FUNCTION ( this << "RNTI " << m_rnti << " Configured for CA" );
                }
              if (msg.haveRadioResourceConfigDedicated)
                {
                  ApplyRadioResourceConfigDedicated (msg.radioResourceConfigDedicated);
                }
              if (msg.haveMeasConfig)
                {
                  ApplyMeasConfig (msg.measConfig);
                }
              LteRrcSap::RrcConnectionReconfigurationCompleted msg2;
              msg2.rrcTransactionIdentifier = msg.rrcTransactionIdentifier;
              m_rrcSapUser->SendRrcConnectionReconfigurationCompleted (msg2);
              m_connectionReconfigurationTrace (m_imsi, m_cellId, m_rnti);
            }

      break;

    default:
      NS_FATAL_ERROR ("method unexpected in state " << ToString (m_state));
      break;
    }
}

void 
LteUeRrc::DoRecvRrcConnectionReestablishment (LteRrcSap::RrcConnectionReestablishment msg)
{
  NS_LOG_FUNCTION (this << " RNTI " << m_rnti);
  switch (m_state)
    {
    case CONNECTED_REESTABLISHING:
      {
        /**
         * \todo After receiving RRC Connection Re-establishment, stop timer
         *       T301, fire a new trace source, reply with RRC Connection
         *       Re-establishment Complete, and finally switch to
         *       CONNECTED_NORMALLY state. See Section 5.3.7.5 of 3GPP TS
         *       36.331.
         */
      }
      break;

    default:
      NS_FATAL_ERROR ("method unexpected in state " << ToString (m_state));
      break;
    }
}

void 
LteUeRrc::DoRecvRrcConnectionReestablishmentReject (LteRrcSap::RrcConnectionReestablishmentReject msg)
{
  NS_LOG_FUNCTION (this << " RNTI " << m_rnti);
  switch (m_state)
    {
    case CONNECTED_REESTABLISHING:
      {
        /**
         * \todo After receiving RRC Connection Re-establishment Reject, stop
         *       timer T301. See Section 5.3.7.8 of 3GPP TS 36.331.
         */
        LeaveConnectedMode ();
      }
      break;

    default:
      NS_FATAL_ERROR ("method unexpected in state " << ToString (m_state));
      break;
    }
}

void LteUeRrc::DoRecvRrcPagingDirect()
{
    std::cout<<Simulator::Now().GetSeconds()<<" "<<m_rnti<<" LteUeRrc::DoRecvRrcPagingDirect  "<<g_ueRrcStateName[m_state]<<std::endl;
    m_hasReceivedPaging = true;
}

void 
LteUeRrc::DoRecvRrcConnectionRelease (LteRrcSap::RrcConnectionRelease msg)
{
    NS_LOG_FUNCTION (this << " RNTI " << m_rnti);
    std::cout<<Simulator::Now().GetSeconds()<<" LteUeRrc::DoRecvRrcConnectionRelease  "<<g_ueRrcStateName[m_state]<<std::endl;
    // \todo Currently not implemented, see Section 5.3.8 of 3GPP TS 36.331.

    switch (m_state)
    {
        case CONNECTED_NORMALLY:
            SwitchToState(IDLE_SUSPEND);
            m_hasReceivedPaging = false;
            break;

        default:
            break;

    }

}

void 
LteUeRrc::DoRecvRrcConnectionReject (LteRrcSap::RrcConnectionReject msg)
{
  NS_LOG_FUNCTION (this);
  m_connectionTimeout.Cancel ();

  m_cmacSapProvider.at (0)->Reset ();       // reset the MAC
  m_hasReceivedSib2 = false;         // invalidate the previously received SIB2
  SwitchToState (IDLE_CAMPED_NORMALLY);
  m_asSapUser->NotifyConnectionFailed ();  // inform upper layer
}



void
LteUeRrc::SynchronizeToStrongestCell ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_state == IDLE_CELL_SEARCH);

  uint16_t maxRsrpCellId = 0;
  double maxRsrp = -std::numeric_limits<double>::infinity ();

  std::map<uint16_t, MeasValues>::iterator it;
  for (it = m_storedMeasValues.begin (); it != m_storedMeasValues.end (); it++)
    {
      /*
       * This block attempts to find a cell with strongest RSRP and has not
       * yet been identified as "acceptable cell".
       */
      if (maxRsrp < it->second.rsrp)
        {
          std::set<uint16_t>::const_iterator itCell;
          itCell = m_acceptableCell.find (it->first);
          if (itCell == m_acceptableCell.end ())
            {
              maxRsrpCellId = it->first;
              maxRsrp = it->second.rsrp;
            }
        }
    }

  if (maxRsrpCellId == 0)
    {
      NS_LOG_WARN (this << " Cell search is unable to detect surrounding cell to attach to");
    }
  else
    {
      NS_LOG_LOGIC (this << " cell " << maxRsrpCellId
                         << " is the strongest untried surrounding cell");
      m_cphySapProvider.at(0)->SynchronizeWithEnb (maxRsrpCellId, m_dlEarfcn);
      SwitchToState (IDLE_WAIT_MIB_SIB1);
    }

} // end of void LteUeRrc::SynchronizeToStrongestCell ()


void
LteUeRrc::EvaluateCellForSelection ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_state == IDLE_WAIT_SIB1);
  NS_ASSERT (m_hasReceivedMib);
  NS_ASSERT (m_hasReceivedSib1);
  uint16_t cellId = m_lastSib1.cellAccessRelatedInfo.cellIdentity;

  // Cell selection criteria evaluation

  bool isSuitableCell = false;
  bool isAcceptableCell = false;
  std::map<uint16_t, MeasValues>::iterator storedMeasIt = m_storedMeasValues.find (cellId);
  double qRxLevMeas = storedMeasIt->second.rsrp;
  double qRxLevMin = EutranMeasurementMapping::IeValue2ActualQRxLevMin (m_lastSib1.cellSelectionInfo.qRxLevMin);
  NS_LOG_LOGIC (this << " cell selection to cellId=" << cellId
                     << " qrxlevmeas=" << qRxLevMeas << " dBm"
                     << " qrxlevmin=" << qRxLevMin << " dBm");

  if (qRxLevMeas - qRxLevMin > 0)
    {
      isAcceptableCell = true;

      uint32_t cellCsgId = m_lastSib1.cellAccessRelatedInfo.csgIdentity;
      bool cellCsgIndication = m_lastSib1.cellAccessRelatedInfo.csgIndication;

      isSuitableCell = (cellCsgIndication == false) || (cellCsgId == m_csgWhiteList);

      NS_LOG_LOGIC (this << " csg(ue/cell/indication)=" << m_csgWhiteList << "/"
                         << cellCsgId << "/" << cellCsgIndication);
    }

  // Cell selection decision

  if (isSuitableCell)
    {
      m_cellId = cellId;
      m_cphySapProvider.at(0)->SynchronizeWithEnb (cellId, m_dlEarfcn);
      m_cphySapProvider.at(0)->SetDlBandwidth (m_dlBandwidth);
      m_initialCellSelectionEndOkTrace (m_imsi, cellId);
      SwitchToState (IDLE_CAMPED_NORMALLY);
    }
  else
    {
      // ignore the MIB and SIB1 received from this cell
      m_hasReceivedMib = false;
      m_hasReceivedSib1 = false;

      m_initialCellSelectionEndErrorTrace (m_imsi, cellId);

      if (isAcceptableCell)
        {
          /*
           * The cells inserted into this list will not be considered for
           * subsequent cell search attempt.
           */
          m_acceptableCell.insert (cellId);
        }

      SwitchToState (IDLE_CELL_SEARCH);
      SynchronizeToStrongestCell (); // retry to a different cell
    }

} // end of void LteUeRrc::EvaluateCellForSelection ()


void
LteUeRrc::ApplyRadioResourceConfigDedicatedSecondaryCarrier (LteRrcSap::NonCriticalExtensionConfiguration nonCec)
{
  NS_LOG_FUNCTION (this);

  for(std::list<LteRrcSap::SCellToAddMod>::iterator it = nonCec.sCellsToAddModList.begin(); it!=nonCec.sCellsToAddModList.end(); it++)
    {
      LteRrcSap::SCellToAddMod scell = *it;
      uint8_t ccId = scell.sCellIndex;


      uint8_t ulBand = scell.radioResourceConfigCommonSCell.ulConfiguration.ulFreqInfo.ulBandwidth;
      uint32_t ulEarfcn = scell.radioResourceConfigCommonSCell.ulConfiguration.ulFreqInfo.ulCarrierFreq;
      uint8_t dlBand = scell.radioResourceConfigCommonSCell.nonUlConfiguration.dlBandwidth;
      uint32_t dlEarfcn = scell.cellIdentification.dlCarrierFreq;
      uint8_t txMode = scell.radioResourceConfigDedicateSCell.physicalConfigDedicatedSCell.antennaInfo.transmissionMode;
      uint8_t srsIndex = scell.radioResourceConfigDedicateSCell.physicalConfigDedicatedSCell.soundingRsUlConfigDedicated.srsConfigIndex;

      m_cphySapProvider.at (ccId)->SynchronizeWithEnb (m_cellId, dlEarfcn);
      m_cphySapProvider.at (ccId)->SetDlBandwidth (dlBand);
      m_cphySapProvider.at (ccId)->ConfigureUplink (ulEarfcn, ulBand);
      m_cphySapProvider.at (ccId)->ConfigureReferenceSignalPower (scell.radioResourceConfigCommonSCell.nonUlConfiguration.pdschConfigCommon.referenceSignalPower);
      m_cphySapProvider.at (ccId)->SetTransmissionMode (txMode);
      m_cphySapProvider.at (ccId)->SetRnti(m_rnti);
      m_cmacSapProvider.at (ccId)->SetRnti(m_rnti);
      // update PdschConfigDedicated (i.e. P_A value)
      LteRrcSap::PdschConfigDedicated pdschConfigDedicated = scell.radioResourceConfigDedicateSCell.physicalConfigDedicatedSCell.pdschConfigDedicated;
      double paDouble = LteRrcSap::ConvertPdschConfigDedicated2Double (pdschConfigDedicated);
      m_cphySapProvider.at (ccId)->SetPa (paDouble);
      m_cphySapProvider.at (ccId)->SetSrsConfigurationIndex (srsIndex);
    }
}

void 
LteUeRrc::ApplyRadioResourceConfigDedicated (LteRrcSap::RadioResourceConfigDedicated rrcd)
{
  NS_LOG_FUNCTION (this);
  const struct LteRrcSap::PhysicalConfigDedicated& pcd = rrcd.physicalConfigDedicated;

  if (pcd.haveAntennaInfoDedicated)
    {
      m_cphySapProvider.at(0)->SetTransmissionMode (pcd.antennaInfo.transmissionMode);
    }
  if (pcd.haveSoundingRsUlConfigDedicated)
    {
      m_cphySapProvider.at(0)->SetSrsConfigurationIndex (pcd.soundingRsUlConfigDedicated.srsConfigIndex);
    }

  if (pcd.havePdschConfigDedicated)
    {
      // update PdschConfigDedicated (i.e. P_A value)
      m_pdschConfigDedicated = pcd.pdschConfigDedicated;
      double paDouble = LteRrcSap::ConvertPdschConfigDedicated2Double (m_pdschConfigDedicated);
      m_cphySapProvider.at(0)->SetPa (paDouble);
    }

  std::list<LteRrcSap::SrbToAddMod>::const_iterator stamIt = rrcd.srbToAddModList.begin ();
  if (stamIt != rrcd.srbToAddModList.end ())
    {
      if (m_srb1 == 0)
        {
          // SRB1 not setup yet        
          NS_ASSERT_MSG ((m_state == IDLE_CONNECTING) || (m_state == CONNECTED_HANDOVER), 
                         "unexpected state " << ToString (m_state));
          NS_ASSERT_MSG (stamIt->srbIdentity == 1, "only SRB1 supported");

          const uint8_t lcid = 1; // fixed LCID for SRB1

          Ptr<LteRlc> rlc = CreateObject<LteRlcAm> ();
          rlc->SetLteMacSapProvider (m_macSapProvider);
          rlc->SetRnti (m_rnti);
          rlc->SetLcId (lcid);      

          Ptr<LtePdcp> pdcp = CreateObject<LtePdcp> ();
          pdcp->SetRnti (m_rnti);
          pdcp->SetLcId (lcid);
          pdcp->SetLtePdcpSapUser (m_drbPdcpSapUser);
          pdcp->SetLteRlcSapProvider (rlc->GetLteRlcSapProvider ());
          rlc->SetLteRlcSapUser (pdcp->GetLteRlcSapUser ());

          m_srb1 = CreateObject<LteSignalingRadioBearerInfo> ();
          m_srb1->m_rlc = rlc;
          m_srb1->m_pdcp = pdcp;
          m_srb1->m_srbIdentity = 1;
          
          m_srb1->m_logicalChannelConfig.priority = stamIt->logicalChannelConfig.priority;
          m_srb1->m_logicalChannelConfig.prioritizedBitRateKbps = stamIt->logicalChannelConfig.prioritizedBitRateKbps;
          m_srb1->m_logicalChannelConfig.bucketSizeDurationMs = stamIt->logicalChannelConfig.bucketSizeDurationMs;
          m_srb1->m_logicalChannelConfig.logicalChannelGroup = stamIt->logicalChannelConfig.logicalChannelGroup;

          struct LteUeCmacSapProvider::LogicalChannelConfig lcConfig;
          lcConfig.priority = stamIt->logicalChannelConfig.priority;
          lcConfig.prioritizedBitRateKbps = stamIt->logicalChannelConfig.prioritizedBitRateKbps;
          lcConfig.bucketSizeDurationMs = stamIt->logicalChannelConfig.bucketSizeDurationMs;
          lcConfig.logicalChannelGroup = stamIt->logicalChannelConfig.logicalChannelGroup;
          m_cmacSapProvider.at (0)->AddLc (lcid, lcConfig, rlc->GetLteMacSapUser ());
          ++stamIt;
          NS_ASSERT_MSG (stamIt == rrcd.srbToAddModList.end (), "at most one SrbToAdd supported");
          
          LteUeRrcSapUser::SetupParameters ueParams;
          ueParams.srb0SapProvider = m_srb0->m_rlc->GetLteRlcSapProvider ();
          ueParams.srb1SapProvider = m_srb1->m_pdcp->GetLtePdcpSapProvider ();
          m_rrcSapUser->Setup (ueParams);
        }
      else
        {
          NS_LOG_INFO ("request to modify SRB1 (skipping as currently not implemented)");
          // would need to modify m_srb1, and then propagate changes to the MAC
        }
    }


  std::list<LteRrcSap::DrbToAddMod>::const_iterator dtamIt;
  for (dtamIt = rrcd.drbToAddModList.begin ();
       dtamIt != rrcd.drbToAddModList.end ();
       ++dtamIt)
    {
      NS_LOG_INFO (this << " IMSI " << m_imsi << " adding/modifying DRBID " << (uint32_t) dtamIt->drbIdentity << " LC " << (uint32_t) dtamIt->logicalChannelIdentity);
      NS_ASSERT_MSG (dtamIt->logicalChannelIdentity > 2, "LCID value " << dtamIt->logicalChannelIdentity << " is reserved for SRBs");

      std::map<uint8_t, Ptr<LteDataRadioBearerInfo> >::iterator drbMapIt = m_drbMap.find (dtamIt->drbIdentity);
      if (drbMapIt == m_drbMap.end ())
        {
          NS_LOG_INFO ("New Data Radio Bearer");
        
          TypeId rlcTypeId;
          if (m_useRlcSm)
            {
              rlcTypeId = LteRlcSm::GetTypeId ();
            }
          else
            {
              switch (dtamIt->rlcConfig.choice)
                {
                case LteRrcSap::RlcConfig::AM: 
                  rlcTypeId = LteRlcAm::GetTypeId ();
                  break;
          
                case LteRrcSap::RlcConfig::UM_BI_DIRECTIONAL: 
                  rlcTypeId = LteRlcUm::GetTypeId ();
                  break;
          
                default:
                  NS_FATAL_ERROR ("unsupported RLC configuration");
                  break;                
                }
            }
  
          ObjectFactory rlcObjectFactory;
          rlcObjectFactory.SetTypeId (rlcTypeId);
          Ptr<LteRlc> rlc = rlcObjectFactory.Create ()->GetObject<LteRlc> ();
          rlc->SetLteMacSapProvider (m_macSapProvider);
          rlc->SetRnti (m_rnti);
          rlc->SetLcId (dtamIt->logicalChannelIdentity);

          Ptr<LteDataRadioBearerInfo> drbInfo = CreateObject<LteDataRadioBearerInfo> ();
          drbInfo->m_rlc = rlc;
          drbInfo->m_epsBearerIdentity = dtamIt->epsBearerIdentity;
          drbInfo->m_logicalChannelIdentity = dtamIt->logicalChannelIdentity;
          drbInfo->m_drbIdentity = dtamIt->drbIdentity;
 
          // we need PDCP only for real RLC, i.e., RLC/UM or RLC/AM
          // if we are using RLC/SM we don't care of anything above RLC
          if (rlcTypeId != LteRlcSm::GetTypeId ())
            {
              Ptr<LtePdcp> pdcp = CreateObject<LtePdcp> ();
              pdcp->SetRnti (m_rnti);
              pdcp->SetLcId (dtamIt->logicalChannelIdentity);
              pdcp->SetLtePdcpSapUser (m_drbPdcpSapUser);
              pdcp->SetLteRlcSapProvider (rlc->GetLteRlcSapProvider ());
              rlc->SetLteRlcSapUser (pdcp->GetLteRlcSapUser ());
              drbInfo->m_pdcp = pdcp;
            }

          m_bid2DrbidMap[dtamIt->epsBearerIdentity] = dtamIt->drbIdentity;
  
          m_drbMap.insert (std::pair<uint8_t, Ptr<LteDataRadioBearerInfo> > (dtamIt->drbIdentity, drbInfo));
  

          struct LteUeCmacSapProvider::LogicalChannelConfig lcConfig;
          lcConfig.priority = dtamIt->logicalChannelConfig.priority;
          lcConfig.prioritizedBitRateKbps = dtamIt->logicalChannelConfig.prioritizedBitRateKbps;
          lcConfig.bucketSizeDurationMs = dtamIt->logicalChannelConfig.bucketSizeDurationMs;
          lcConfig.logicalChannelGroup = dtamIt->logicalChannelConfig.logicalChannelGroup;      

          for (uint32_t i = 0; i < m_numberOfComponentCarriers; i++)
          {
            m_cmacSapProvider.at (i)->AddLc (dtamIt->logicalChannelIdentity,
                                    lcConfig,
                                    rlc->GetLteMacSapUser ());
          }
          rlc->Initialize ();
        }
      else
        {
          NS_LOG_INFO ("request to modify existing DRBID");
          Ptr<LteDataRadioBearerInfo> drbInfo = drbMapIt->second;
          /// \todo currently not implemented. Would need to modify drbInfo, and then propagate changes to the MAC
        }
    }
  
  std::list<uint8_t>::iterator dtdmIt;
  for (dtdmIt = rrcd.drbToReleaseList.begin ();
       dtdmIt != rrcd.drbToReleaseList.end ();
       ++dtdmIt)
    {
      uint8_t drbid = *dtdmIt;
      NS_LOG_INFO (this << " IMSI " << m_imsi << " releasing DRB " << (uint32_t) drbid << drbid);
      std::map<uint8_t, Ptr<LteDataRadioBearerInfo> >::iterator it =   m_drbMap.find (drbid);
      NS_ASSERT_MSG (it != m_drbMap.end (), "could not find bearer with given lcid");
      m_drbMap.erase (it);      
      m_bid2DrbidMap.erase (drbid);
      //Remove LCID
      for (uint32_t i = 0; i < m_numberOfComponentCarriers; i++)
       {
         m_cmacSapProvider.at (i)->RemoveLc (drbid + 2);
       }
    }
}


void 
LteUeRrc::ApplyMeasConfig (LteRrcSap::MeasConfig mc)
{
  NS_LOG_FUNCTION (this);

  // perform the actions specified in 3GPP TS 36.331 section 5.5.2.1 

  // 3GPP TS 36.331 section 5.5.2.4 Measurement object removal
  for (std::list<uint8_t>::iterator it = mc.measObjectToRemoveList.begin ();
       it !=  mc.measObjectToRemoveList.end ();
       ++it)
    {
      uint8_t measObjectId = *it;
      NS_LOG_LOGIC (this << " deleting measObjectId " << (uint32_t)  measObjectId);
      m_varMeasConfig.measObjectList.erase (measObjectId);
      std::map<uint8_t, LteRrcSap::MeasIdToAddMod>::iterator measIdIt = m_varMeasConfig.measIdList.begin ();
      while (measIdIt != m_varMeasConfig.measIdList.end ())
        {
          if (measIdIt->second.measObjectId == measObjectId)
            {
              uint8_t measId = measIdIt->second.measId;
              NS_ASSERT (measId == measIdIt->first);
              NS_LOG_LOGIC (this << " deleting measId " << (uint32_t) measId << " because referring to measObjectId " << (uint32_t)  measObjectId);
              // note: postfix operator preserves iterator validity
              m_varMeasConfig.measIdList.erase (measIdIt++);
              VarMeasReportListClear (measId);
            }
          else
            {
              ++measIdIt;
            }
        }

    }

  // 3GPP TS 36.331 section 5.5.2.5  Measurement object addition/ modification
  for (std::list<LteRrcSap::MeasObjectToAddMod>::iterator it = mc.measObjectToAddModList.begin ();
       it !=  mc.measObjectToAddModList.end ();
       ++it)
    {
      // simplifying assumptions
      NS_ASSERT_MSG (it->measObjectEutra.cellsToRemoveList.empty (), "cellsToRemoveList not supported");
      NS_ASSERT_MSG (it->measObjectEutra.cellsToAddModList.empty (), "cellsToAddModList not supported");
      NS_ASSERT_MSG (it->measObjectEutra.cellsToRemoveList.empty (), "blackCellsToRemoveList not supported");
      NS_ASSERT_MSG (it->measObjectEutra.blackCellsToAddModList.empty (), "blackCellsToAddModList not supported");
      NS_ASSERT_MSG (it->measObjectEutra.haveCellForWhichToReportCGI == false, "cellForWhichToReportCGI is not supported");

      uint8_t measObjectId = it->measObjectId;
      std::map<uint8_t, LteRrcSap::MeasObjectToAddMod>::iterator measObjectIt = m_varMeasConfig.measObjectList.find (measObjectId);
      if (measObjectIt != m_varMeasConfig.measObjectList.end ())
        {
          NS_LOG_LOGIC ("measObjectId " << (uint32_t) measObjectId << " exists, updating entry");
          measObjectIt->second = *it;
          for (std::map<uint8_t, LteRrcSap::MeasIdToAddMod>::iterator measIdIt 
                 = m_varMeasConfig.measIdList.begin ();
               measIdIt != m_varMeasConfig.measIdList.end ();
               ++measIdIt)
            {
              if (measIdIt->second.measObjectId == measObjectId)
                {
                  uint8_t measId = measIdIt->second.measId;
                  NS_LOG_LOGIC (this << " found measId " << (uint32_t) measId << " referring to measObjectId " << (uint32_t)  measObjectId);
                  VarMeasReportListClear (measId);
                }
            }
        }
      else
        {
          NS_LOG_LOGIC ("measObjectId " << (uint32_t) measObjectId << " is new, adding entry");
          m_varMeasConfig.measObjectList[measObjectId] = *it;
        }

    }

  // 3GPP TS 36.331 section 5.5.2.6 Reporting configuration removal
  for (std::list<uint8_t>::iterator it = mc.reportConfigToRemoveList.begin ();
       it !=  mc.reportConfigToRemoveList.end ();
       ++it)
    {
      uint8_t reportConfigId = *it;
      NS_LOG_LOGIC (this << " deleting reportConfigId " << (uint32_t)  reportConfigId);
      m_varMeasConfig.reportConfigList.erase (reportConfigId);
      std::map<uint8_t, LteRrcSap::MeasIdToAddMod>::iterator measIdIt = m_varMeasConfig.measIdList.begin ();
      while (measIdIt != m_varMeasConfig.measIdList.end ())
        {
          if (measIdIt->second.reportConfigId == reportConfigId)
            {
              uint8_t measId = measIdIt->second.measId;
              NS_ASSERT (measId == measIdIt->first);
              NS_LOG_LOGIC (this << " deleting measId " << (uint32_t) measId << " because referring to reportConfigId " << (uint32_t)  reportConfigId);
              // note: postfix operator preserves iterator validity
              m_varMeasConfig.measIdList.erase (measIdIt++);
              VarMeasReportListClear (measId);
            }
          else
            {
              ++measIdIt;
            }
        }

    }

  // 3GPP TS 36.331 section 5.5.2.7 Reporting configuration addition/ modification
  for (std::list<LteRrcSap::ReportConfigToAddMod>::iterator it = mc.reportConfigToAddModList.begin ();
       it !=  mc.reportConfigToAddModList.end ();
       ++it)
    {
      // simplifying assumptions
      NS_ASSERT_MSG (it->reportConfigEutra.triggerType == LteRrcSap::ReportConfigEutra::EVENT,
                     "only trigger type EVENT is supported");

      uint8_t reportConfigId = it->reportConfigId;
      std::map<uint8_t, LteRrcSap::ReportConfigToAddMod>::iterator reportConfigIt = m_varMeasConfig.reportConfigList.find (reportConfigId);
      if (reportConfigIt != m_varMeasConfig.reportConfigList.end ())
        {
          NS_LOG_LOGIC ("reportConfigId " << (uint32_t) reportConfigId << " exists, updating entry");
          m_varMeasConfig.reportConfigList[reportConfigId] = *it;
          for (std::map<uint8_t, LteRrcSap::MeasIdToAddMod>::iterator measIdIt 
                 = m_varMeasConfig.measIdList.begin ();
               measIdIt != m_varMeasConfig.measIdList.end ();
               ++measIdIt)
            {
              if (measIdIt->second.reportConfigId == reportConfigId)
                {
                  uint8_t measId = measIdIt->second.measId;
                  NS_LOG_LOGIC (this << " found measId " << (uint32_t) measId << " referring to reportConfigId " << (uint32_t)  reportConfigId);
                  VarMeasReportListClear (measId);
                }
            }
        }
      else
        {
          NS_LOG_LOGIC ("reportConfigId " << (uint32_t) reportConfigId << " is new, adding entry");
          m_varMeasConfig.reportConfigList[reportConfigId] = *it;
        }

    }

  // 3GPP TS 36.331 section 5.5.2.8 Quantity configuration
  if (mc.haveQuantityConfig)
    {
      NS_LOG_LOGIC (this << " setting quantityConfig");
      m_varMeasConfig.quantityConfig = mc.quantityConfig;
      // we calculate here the coefficient a used for Layer 3 filtering, see 3GPP TS 36.331 section 5.5.3.2
      m_varMeasConfig.aRsrp = std::pow (0.5, mc.quantityConfig.filterCoefficientRSRP / 4.0);
      m_varMeasConfig.aRsrq = std::pow (0.5, mc.quantityConfig.filterCoefficientRSRQ / 4.0);
      NS_LOG_LOGIC (this << " new filter coefficients: aRsrp=" << m_varMeasConfig.aRsrp << ", aRsrq=" << m_varMeasConfig.aRsrq);

      for (std::map<uint8_t, LteRrcSap::MeasIdToAddMod>::iterator measIdIt
             = m_varMeasConfig.measIdList.begin ();
           measIdIt != m_varMeasConfig.measIdList.end ();
           ++measIdIt)
        {
          VarMeasReportListClear (measIdIt->second.measId);
        }
    }

  // 3GPP TS 36.331 section 5.5.2.2 Measurement identity removal
  for (std::list<uint8_t>::iterator it = mc.measIdToRemoveList.begin ();
       it !=  mc.measIdToRemoveList.end ();
       ++it)
    {
      uint8_t measId = *it;
      NS_LOG_LOGIC (this << " deleting measId " << (uint32_t) measId);
      m_varMeasConfig.measIdList.erase (measId);
      VarMeasReportListClear (measId);

      // removing time-to-trigger queues
      m_enteringTriggerQueue.erase (measId);
      m_leavingTriggerQueue.erase (measId);
    }

  // 3GPP TS 36.331 section 5.5.2.3 Measurement identity addition/ modification
  for (std::list<LteRrcSap::MeasIdToAddMod>::iterator it = mc.measIdToAddModList.begin ();
       it !=  mc.measIdToAddModList.end ();
       ++it)
    {
      NS_LOG_LOGIC (this << " measId " << (uint32_t) it->measId
                         << " (measObjectId=" << (uint32_t) it->measObjectId
                         << ", reportConfigId=" << (uint32_t) it->reportConfigId
                         << ")");
      NS_ASSERT (m_varMeasConfig.measObjectList.find (it->measObjectId)
                 != m_varMeasConfig.measObjectList.end ());
      NS_ASSERT (m_varMeasConfig.reportConfigList.find (it->reportConfigId)
                 != m_varMeasConfig.reportConfigList.end ());
      m_varMeasConfig.measIdList[it->measId] = *it; // side effect: create new entry if not exists
      std::map<uint8_t, VarMeasReport>::iterator measReportIt = m_varMeasReportList.find (it->measId);
      if (measReportIt != m_varMeasReportList.end ())
        {
          measReportIt->second.periodicReportTimer.Cancel ();
          m_varMeasReportList.erase (measReportIt);
        }
      NS_ASSERT (m_varMeasConfig.reportConfigList.find (it->reportConfigId)
                 ->second.reportConfigEutra.triggerType != LteRrcSap::ReportConfigEutra::PERIODICAL);

      // new empty queues for time-to-trigger
      std::list<PendingTrigger_t> s;
      m_enteringTriggerQueue[it->measId] = s;
      m_leavingTriggerQueue[it->measId] = s;
    }

  if (mc.haveMeasGapConfig)
    {
      NS_FATAL_ERROR ("measurement gaps are currently not supported");
    }

  if (mc.haveSmeasure)
    {
      NS_FATAL_ERROR ("s-measure is currently not supported");
    }

  if (mc.haveSpeedStatePars)
    {
      NS_FATAL_ERROR ("SpeedStatePars are currently not supported");
    }
}

void
LteUeRrc::SaveUeMeasurements (uint16_t cellId, double rsrp, double rsrq,
                              bool useLayer3Filtering)
{
  NS_LOG_FUNCTION (this << cellId << rsrp << rsrq << useLayer3Filtering);

  std::map<uint16_t, MeasValues>::iterator storedMeasIt = m_storedMeasValues.find (cellId);

  if (storedMeasIt != m_storedMeasValues.end ())
    {
      if (useLayer3Filtering)
        {
          // F_n = (1-a) F_{n-1} + a M_n
          storedMeasIt->second.rsrp = (1 - m_varMeasConfig.aRsrp) * storedMeasIt->second.rsrp
            + m_varMeasConfig.aRsrp * rsrp;

          if (std::isnan (storedMeasIt->second.rsrq))
            {
              // the previous RSRQ measurements provided UE PHY are invalid
              storedMeasIt->second.rsrq = rsrq; // replace it with unfiltered value
            }
          else
            {
              storedMeasIt->second.rsrq = (1 - m_varMeasConfig.aRsrq) * storedMeasIt->second.rsrq
                + m_varMeasConfig.aRsrq * rsrq;
            }
        }
      else
        {
          storedMeasIt->second.rsrp = rsrp;
          storedMeasIt->second.rsrq = rsrq;
        }
    }
  else
    {
      // first value is always unfiltered
      MeasValues v;
      v.rsrp = rsrp;
      v.rsrq = rsrq;
      std::pair<uint16_t, MeasValues> val (cellId, v);
      std::pair<std::map<uint16_t, MeasValues>::iterator, bool>
        ret = m_storedMeasValues.insert (val);
      NS_ASSERT_MSG (ret.second == true, "element already existed");
      storedMeasIt = ret.first;
    }

  NS_LOG_DEBUG (this << " IMSI " << m_imsi << " state " << ToString (m_state)
                     << ", measured cell " << m_cellId
                     << ", new RSRP " << rsrp << " stored " << storedMeasIt->second.rsrp
                     << ", new RSRQ " << rsrq << " stored " << storedMeasIt->second.rsrq);
  storedMeasIt->second.timestamp = Simulator::Now ();

} // end of void SaveUeMeasurements

void
LteUeRrc::MeasurementReportTriggering (uint8_t measId)
{
  NS_LOG_FUNCTION (this << (uint16_t) measId);

  std::map<uint8_t, LteRrcSap::MeasIdToAddMod>::iterator measIdIt =
    m_varMeasConfig.measIdList.find (measId);
  NS_ASSERT (measIdIt != m_varMeasConfig.measIdList.end ());
  NS_ASSERT (measIdIt->first == measIdIt->second.measId);

  std::map<uint8_t, LteRrcSap::ReportConfigToAddMod>::iterator
    reportConfigIt = m_varMeasConfig.reportConfigList.find (measIdIt->second.reportConfigId);
  NS_ASSERT (reportConfigIt != m_varMeasConfig.reportConfigList.end ());
  LteRrcSap::ReportConfigEutra& reportConfigEutra = reportConfigIt->second.reportConfigEutra;

  std::map<uint8_t, LteRrcSap::MeasObjectToAddMod>::iterator
    measObjectIt = m_varMeasConfig.measObjectList.find (measIdIt->second.measObjectId);
  NS_ASSERT (measObjectIt != m_varMeasConfig.measObjectList.end ());
  LteRrcSap::MeasObjectEutra& measObjectEutra = measObjectIt->second.measObjectEutra;

  std::map<uint8_t, VarMeasReport>::iterator
    measReportIt = m_varMeasReportList.find (measId);
  bool isMeasIdInReportList = (measReportIt != m_varMeasReportList.end ());

  // we don't check the purpose field, as it is only included for
  // triggerType == periodical, which is not supported
  NS_ASSERT_MSG (reportConfigEutra.triggerType
                 == LteRrcSap::ReportConfigEutra::EVENT,
                 "only triggerType == event is supported");
  // only EUTRA is supported, no need to check for it

  NS_LOG_LOGIC (this << " considering measId " << (uint32_t) measId);
  bool eventEntryCondApplicable = false;
  bool eventLeavingCondApplicable = false;
  ConcernedCells_t concernedCellsEntry;
  ConcernedCells_t concernedCellsLeaving;

  switch (reportConfigEutra.eventId)
    {
    case LteRrcSap::ReportConfigEutra::EVENT_A1:
      {
        /*
         * Event A1 (Serving becomes better than threshold)
         * Please refer to 3GPP TS 36.331 Section 5.5.4.2
         */

        double ms; // Ms, the measurement result of the serving cell
        double thresh; // Thresh, the threshold parameter for this event
        // Hys, the hysteresis parameter for this event.
        double hys = EutranMeasurementMapping::IeValue2ActualHysteresis (reportConfigEutra.hysteresis);

        switch (reportConfigEutra.triggerQuantity)
          {
          case LteRrcSap::ReportConfigEutra::RSRP:
            ms = m_storedMeasValues[m_cellId].rsrp;
            NS_ASSERT (reportConfigEutra.threshold1.choice
                       == LteRrcSap::ThresholdEutra::THRESHOLD_RSRP);
            thresh = EutranMeasurementMapping::RsrpRange2Dbm (reportConfigEutra.threshold1.range);
            break;
          case LteRrcSap::ReportConfigEutra::RSRQ:
            ms = m_storedMeasValues[m_cellId].rsrq;
            NS_ASSERT (reportConfigEutra.threshold1.choice
                       == LteRrcSap::ThresholdEutra::THRESHOLD_RSRQ);
            thresh = EutranMeasurementMapping::RsrqRange2Db (reportConfigEutra.threshold1.range);
            break;
          default:
            NS_FATAL_ERROR ("unsupported triggerQuantity");
            break;
          }

        // Inequality A1-1 (Entering condition): Ms - Hys > Thresh
        bool entryCond = ms - hys > thresh;

        if (entryCond)
          {
            if (!isMeasIdInReportList)
              {
                concernedCellsEntry.push_back (m_cellId);
                eventEntryCondApplicable = true;
              }
            else
              {
                /*
                 * This is to check that the triggered cell recorded in the
                 * VarMeasReportList is the serving cell.
                 */
                NS_ASSERT (measReportIt->second.cellsTriggeredList.find (m_cellId)
                           != measReportIt->second.cellsTriggeredList.end ());
              }
          }
        else if (reportConfigEutra.timeToTrigger > 0)
          {
            CancelEnteringTrigger (measId);
          }

        // Inequality A1-2 (Leaving condition): Ms + Hys < Thresh
        bool leavingCond = ms + hys < thresh;

        if (leavingCond)
          {
            if (isMeasIdInReportList)
              {
                /*
                 * This is to check that the triggered cell recorded in the
                 * VarMeasReportList is the serving cell.
                 */
                NS_ASSERT (measReportIt->second.cellsTriggeredList.find (m_cellId)
                           != measReportIt->second.cellsTriggeredList.end ());
                concernedCellsLeaving.push_back (m_cellId);
                eventLeavingCondApplicable = true;
              }
          }
        else if (reportConfigEutra.timeToTrigger > 0)
          {
            CancelLeavingTrigger (measId);
          }

        NS_LOG_LOGIC (this << " event A1: serving cell " << m_cellId
                           << " ms=" << ms << " thresh=" << thresh
                           << " entryCond=" << entryCond
                           << " leavingCond=" << leavingCond);

      } // end of case LteRrcSap::ReportConfigEutra::EVENT_A1

      break;

    case LteRrcSap::ReportConfigEutra::EVENT_A2:
      {
        /*
         * Event A2 (Serving becomes worse than threshold)
         * Please refer to 3GPP TS 36.331 Section 5.5.4.3
         */

        double ms; // Ms, the measurement result of the serving cell
        double thresh; // Thresh, the threshold parameter for this event
        // Hys, the hysteresis parameter for this event.
        double hys = EutranMeasurementMapping::IeValue2ActualHysteresis (reportConfigEutra.hysteresis);

        switch (reportConfigEutra.triggerQuantity)
          {
          case LteRrcSap::ReportConfigEutra::RSRP:
            ms = m_storedMeasValues[m_cellId].rsrp;
            NS_ASSERT (reportConfigEutra.threshold1.choice
                       == LteRrcSap::ThresholdEutra::THRESHOLD_RSRP);
            thresh = EutranMeasurementMapping::RsrpRange2Dbm (reportConfigEutra.threshold1.range);
            break;
          case LteRrcSap::ReportConfigEutra::RSRQ:
            ms = m_storedMeasValues[m_cellId].rsrq;
            NS_ASSERT (reportConfigEutra.threshold1.choice
                       == LteRrcSap::ThresholdEutra::THRESHOLD_RSRQ);
            thresh = EutranMeasurementMapping::RsrqRange2Db (reportConfigEutra.threshold1.range);
            break;
          default:
            NS_FATAL_ERROR ("unsupported triggerQuantity");
            break;
          }

        // Inequality A2-1 (Entering condition): Ms + Hys < Thresh
        bool entryCond = ms + hys < thresh;

        if (entryCond)
          {
            if (!isMeasIdInReportList)
              {
                concernedCellsEntry.push_back (m_cellId);
                eventEntryCondApplicable = true;
              }
            else
              {
                /*
                 * This is to check that the triggered cell recorded in the
                 * VarMeasReportList is the serving cell.
                 */
                NS_ASSERT (measReportIt->second.cellsTriggeredList.find (m_cellId)
                           != measReportIt->second.cellsTriggeredList.end ());
              }
          }
        else if (reportConfigEutra.timeToTrigger > 0)
          {
            CancelEnteringTrigger (measId);
          }

        // Inequality A2-2 (Leaving condition): Ms - Hys > Thresh
        bool leavingCond = ms - hys > thresh;

        if (leavingCond)
          {
            if (isMeasIdInReportList)
              {
                /*
                 * This is to check that the triggered cell recorded in the
                 * VarMeasReportList is the serving cell.
                 */
                NS_ASSERT (measReportIt->second.cellsTriggeredList.find (m_cellId)
                           != measReportIt->second.cellsTriggeredList.end ());
                concernedCellsLeaving.push_back (m_cellId);
                eventLeavingCondApplicable = true;
              }
          }
        else if (reportConfigEutra.timeToTrigger > 0)
          {
            CancelLeavingTrigger (measId);
          }

        NS_LOG_LOGIC (this << " event A2: serving cell " << m_cellId
                           << " ms=" << ms << " thresh=" << thresh
                           << " entryCond=" << entryCond
                           << " leavingCond=" << leavingCond);

      } // end of case LteRrcSap::ReportConfigEutra::EVENT_A2

      break;

    case LteRrcSap::ReportConfigEutra::EVENT_A3:
      {
        /*
         * Event A3 (Neighbour becomes offset better than PCell)
         * Please refer to 3GPP TS 36.331 Section 5.5.4.4
         */

        double mn; // Mn, the measurement result of the neighbouring cell
        double ofn = measObjectEutra.offsetFreq; // Ofn, the frequency specific offset of the frequency of the
        double ocn = 0.0; // Ocn, the cell specific offset of the neighbour cell
        double mp; // Mp, the measurement result of the PCell
        double ofp = measObjectEutra.offsetFreq; // Ofp, the frequency specific offset of the primary frequency
        double ocp = 0.0; // Ocp, the cell specific offset of the PCell
        // Off, the offset parameter for this event.
        double off = EutranMeasurementMapping::IeValue2ActualA3Offset (reportConfigEutra.a3Offset);
        // Hys, the hysteresis parameter for this event.
        double hys = EutranMeasurementMapping::IeValue2ActualHysteresis (reportConfigEutra.hysteresis);

        switch (reportConfigEutra.triggerQuantity)
          {
          case LteRrcSap::ReportConfigEutra::RSRP:
            mp = m_storedMeasValues[m_cellId].rsrp;
            NS_ASSERT (reportConfigEutra.threshold1.choice
                       == LteRrcSap::ThresholdEutra::THRESHOLD_RSRP);
            break;
          case LteRrcSap::ReportConfigEutra::RSRQ:
            mp = m_storedMeasValues[m_cellId].rsrq;
            NS_ASSERT (reportConfigEutra.threshold1.choice
                       == LteRrcSap::ThresholdEutra::THRESHOLD_RSRQ);
            break;
          default:
            NS_FATAL_ERROR ("unsupported triggerQuantity");
            break;
          }

        for (std::map<uint16_t, MeasValues>::iterator storedMeasIt = m_storedMeasValues.begin ();
             storedMeasIt != m_storedMeasValues.end ();
             ++storedMeasIt)
          {
            uint16_t cellId = storedMeasIt->first;
            if (cellId == m_cellId)
              {
                continue;
              }

            switch (reportConfigEutra.triggerQuantity)
              {
              case LteRrcSap::ReportConfigEutra::RSRP:
                mn = storedMeasIt->second.rsrp;
                break;
              case LteRrcSap::ReportConfigEutra::RSRQ:
                mn = storedMeasIt->second.rsrq;
                break;
              default:
                NS_FATAL_ERROR ("unsupported triggerQuantity");
                break;
              }

            bool hasTriggered = isMeasIdInReportList
              && (measReportIt->second.cellsTriggeredList.find (cellId)
                  != measReportIt->second.cellsTriggeredList.end ());

            // Inequality A3-1 (Entering condition): Mn + Ofn + Ocn - Hys > Mp + Ofp + Ocp + Off
            bool entryCond = mn + ofn + ocn - hys > mp + ofp + ocp + off;

            if (entryCond)
              {
                if (!hasTriggered)
                  {
                    concernedCellsEntry.push_back (cellId);
                    eventEntryCondApplicable = true;
                  }
              }
            else if (reportConfigEutra.timeToTrigger > 0)
              {
                CancelEnteringTrigger (measId, cellId);
              }

            // Inequality A3-2 (Leaving condition): Mn + Ofn + Ocn + Hys < Mp + Ofp + Ocp + Off
            bool leavingCond = mn + ofn + ocn + hys < mp + ofp + ocp + off;

            if (leavingCond)
              {
                if (hasTriggered)
                  {
                    concernedCellsLeaving.push_back (cellId);
                    eventLeavingCondApplicable = true;
                  }
              }
            else if (reportConfigEutra.timeToTrigger > 0)
              {
                CancelLeavingTrigger (measId, cellId);
              }

            NS_LOG_LOGIC (this << " event A3: neighbor cell " << cellId
                               << " mn=" << mn << " mp=" << mp << " offset=" << off
                               << " entryCond=" << entryCond
                               << " leavingCond=" << leavingCond);

          } // end of for (storedMeasIt)

      } // end of case LteRrcSap::ReportConfigEutra::EVENT_A3

      break;

    case LteRrcSap::ReportConfigEutra::EVENT_A4:
      {
        /*
         * Event A4 (Neighbour becomes better than threshold)
         * Please refer to 3GPP TS 36.331 Section 5.5.4.5
         */

        double mn; // Mn, the measurement result of the neighbouring cell
        double ofn = measObjectEutra.offsetFreq; // Ofn, the frequency specific offset of the frequency of the
        double ocn = 0.0; // Ocn, the cell specific offset of the neighbour cell
        double thresh; // Thresh, the threshold parameter for this event
        // Hys, the hysteresis parameter for this event.
        double hys = EutranMeasurementMapping::IeValue2ActualHysteresis (reportConfigEutra.hysteresis);

        switch (reportConfigEutra.triggerQuantity)
          {
          case LteRrcSap::ReportConfigEutra::RSRP:
            NS_ASSERT (reportConfigEutra.threshold1.choice
                       == LteRrcSap::ThresholdEutra::THRESHOLD_RSRP);
            thresh = EutranMeasurementMapping::RsrpRange2Dbm (reportConfigEutra.threshold1.range);
            break;
          case LteRrcSap::ReportConfigEutra::RSRQ:
            NS_ASSERT (reportConfigEutra.threshold1.choice
                       == LteRrcSap::ThresholdEutra::THRESHOLD_RSRQ);
            thresh = EutranMeasurementMapping::RsrqRange2Db (reportConfigEutra.threshold1.range);
            break;
          default:
            NS_FATAL_ERROR ("unsupported triggerQuantity");
            break;
          }

        for (std::map<uint16_t, MeasValues>::iterator storedMeasIt = m_storedMeasValues.begin ();
             storedMeasIt != m_storedMeasValues.end ();
             ++storedMeasIt)
          {
            uint16_t cellId = storedMeasIt->first;
            if (cellId == m_cellId)
              {
                continue;
              }

            switch (reportConfigEutra.triggerQuantity)
              {
              case LteRrcSap::ReportConfigEutra::RSRP:
                mn = storedMeasIt->second.rsrp;
                break;
              case LteRrcSap::ReportConfigEutra::RSRQ:
                mn = storedMeasIt->second.rsrq;
                break;
              default:
                NS_FATAL_ERROR ("unsupported triggerQuantity");
                break;
              }

            bool hasTriggered = isMeasIdInReportList
              && (measReportIt->second.cellsTriggeredList.find (cellId)
                  != measReportIt->second.cellsTriggeredList.end ());

            // Inequality A4-1 (Entering condition): Mn + Ofn + Ocn - Hys > Thresh
            bool entryCond = mn + ofn + ocn - hys > thresh;

            if (entryCond)
              {
                if (!hasTriggered)
                  {
                    concernedCellsEntry.push_back (cellId);
                    eventEntryCondApplicable = true;
                  }
              }
            else if (reportConfigEutra.timeToTrigger > 0)
              {
                CancelEnteringTrigger (measId, cellId);
              }

            // Inequality A4-2 (Leaving condition): Mn + Ofn + Ocn + Hys < Thresh
            bool leavingCond = mn + ofn + ocn + hys < thresh;

            if (leavingCond)
              {
                if (hasTriggered)
                  {
                    concernedCellsLeaving.push_back (cellId);
                    eventLeavingCondApplicable = true;
                  }
              }
            else if (reportConfigEutra.timeToTrigger > 0)
              {
                CancelLeavingTrigger (measId, cellId);
              }

            NS_LOG_LOGIC (this << " event A4: neighbor cell " << cellId
                               << " mn=" << mn << " thresh=" << thresh
                               << " entryCond=" << entryCond
                               << " leavingCond=" << leavingCond);

          } // end of for (storedMeasIt)

      } // end of case LteRrcSap::ReportConfigEutra::EVENT_A4

      break;

    case LteRrcSap::ReportConfigEutra::EVENT_A5:
      {
        /*
         * Event A5 (PCell becomes worse than threshold1 and neighbour
         * becomes better than threshold2)
         * Please refer to 3GPP TS 36.331 Section 5.5.4.6
         */

        double mp; // Mp, the measurement result of the PCell
        double mn; // Mn, the measurement result of the neighbouring cell
        double ofn = measObjectEutra.offsetFreq; // Ofn, the frequency specific offset of the frequency of the
        double ocn = 0.0; // Ocn, the cell specific offset of the neighbour cell
        double thresh1; // Thresh1, the threshold parameter for this event
        double thresh2; // Thresh2, the threshold parameter for this event
        // Hys, the hysteresis parameter for this event.
        double hys = EutranMeasurementMapping::IeValue2ActualHysteresis (reportConfigEutra.hysteresis);

        switch (reportConfigEutra.triggerQuantity)
          {
          case LteRrcSap::ReportConfigEutra::RSRP:
            mp = m_storedMeasValues[m_cellId].rsrp;
            NS_ASSERT (reportConfigEutra.threshold1.choice
                       == LteRrcSap::ThresholdEutra::THRESHOLD_RSRP);
            NS_ASSERT (reportConfigEutra.threshold2.choice
                       == LteRrcSap::ThresholdEutra::THRESHOLD_RSRP);
            thresh1 = EutranMeasurementMapping::RsrpRange2Dbm (reportConfigEutra.threshold1.range);
            thresh2 = EutranMeasurementMapping::RsrpRange2Dbm (reportConfigEutra.threshold2.range);
            break;
          case LteRrcSap::ReportConfigEutra::RSRQ:
            mp = m_storedMeasValues[m_cellId].rsrq;
            NS_ASSERT (reportConfigEutra.threshold1.choice
                       == LteRrcSap::ThresholdEutra::THRESHOLD_RSRQ);
            NS_ASSERT (reportConfigEutra.threshold2.choice
                       == LteRrcSap::ThresholdEutra::THRESHOLD_RSRQ);
            thresh1 = EutranMeasurementMapping::RsrqRange2Db (reportConfigEutra.threshold1.range);
            thresh2 = EutranMeasurementMapping::RsrqRange2Db (reportConfigEutra.threshold2.range);
            break;
          default:
            NS_FATAL_ERROR ("unsupported triggerQuantity");
            break;
          }

        // Inequality A5-1 (Entering condition 1): Mp + Hys < Thresh1
        bool entryCond = mp + hys < thresh1;

        if (entryCond)
          {
            for (std::map<uint16_t, MeasValues>::iterator storedMeasIt = m_storedMeasValues.begin ();
                 storedMeasIt != m_storedMeasValues.end ();
                 ++storedMeasIt)
              {
                uint16_t cellId = storedMeasIt->first;
                if (cellId == m_cellId)
                  {
                    continue;
                  }

                switch (reportConfigEutra.triggerQuantity)
                  {
                  case LteRrcSap::ReportConfigEutra::RSRP:
                    mn = storedMeasIt->second.rsrp;
                    break;
                  case LteRrcSap::ReportConfigEutra::RSRQ:
                    mn = storedMeasIt->second.rsrq;
                    break;
                  default:
                    NS_FATAL_ERROR ("unsupported triggerQuantity");
                    break;
                  }

                bool hasTriggered = isMeasIdInReportList
                  && (measReportIt->second.cellsTriggeredList.find (cellId)
                      != measReportIt->second.cellsTriggeredList.end ());

                // Inequality A5-2 (Entering condition 2): Mn + Ofn + Ocn - Hys > Thresh2

                entryCond = mn + ofn + ocn - hys > thresh2;

                if (entryCond)
                  {
                    if (!hasTriggered)
                      {
                        concernedCellsEntry.push_back (cellId);
                        eventEntryCondApplicable = true;
                      }
                  }
                else if (reportConfigEutra.timeToTrigger > 0)
                  {
                    CancelEnteringTrigger (measId, cellId);
                  }

                NS_LOG_LOGIC (this << " event A5: neighbor cell " << cellId
                                   << " mn=" << mn << " mp=" << mp
                                   << " thresh2=" << thresh2
                                   << " thresh1=" << thresh1
                                   << " entryCond=" << entryCond);

              } // end of for (storedMeasIt)

          } // end of if (entryCond)
        else
          {
            NS_LOG_LOGIC (this << " event A5: serving cell " << m_cellId
                               << " mp=" << mp << " thresh1=" << thresh1
                               << " entryCond=" << entryCond);

            if (reportConfigEutra.timeToTrigger > 0)
              {
                CancelEnteringTrigger (measId);
              }
          }

        if (isMeasIdInReportList)
          {
            // Inequality A5-3 (Leaving condition 1): Mp - Hys > Thresh1
            bool leavingCond = mp - hys > thresh1;

            if (leavingCond)
              {
                if (reportConfigEutra.timeToTrigger == 0)
                  {
                    // leaving condition #2 does not have to be checked

                    for (std::map<uint16_t, MeasValues>::iterator storedMeasIt = m_storedMeasValues.begin ();
                         storedMeasIt != m_storedMeasValues.end ();
                         ++storedMeasIt)
                      {
                        uint16_t cellId = storedMeasIt->first;
                        if (cellId == m_cellId)
                          {
                            continue;
                          }

                        if (measReportIt->second.cellsTriggeredList.find (cellId)
                            != measReportIt->second.cellsTriggeredList.end ())
                          {
                            concernedCellsLeaving.push_back (cellId);
                            eventLeavingCondApplicable = true;
                          }
                      }
                  } // end of if (reportConfigEutra.timeToTrigger == 0)
                else
                  {
                    // leaving condition #2 has to be checked to cancel time-to-trigger

                    for (std::map<uint16_t, MeasValues>::iterator storedMeasIt = m_storedMeasValues.begin ();
                         storedMeasIt != m_storedMeasValues.end ();
                         ++storedMeasIt)
                      {
                        uint16_t cellId = storedMeasIt->first;
                        if (cellId == m_cellId)
                          {
                            continue;
                          }

                        if (measReportIt->second.cellsTriggeredList.find (cellId)
                            != measReportIt->second.cellsTriggeredList.end ())
                          {
                            switch (reportConfigEutra.triggerQuantity)
                              {
                              case LteRrcSap::ReportConfigEutra::RSRP:
                                mn = storedMeasIt->second.rsrp;
                                break;
                              case LteRrcSap::ReportConfigEutra::RSRQ:
                                mn = storedMeasIt->second.rsrq;
                                break;
                              default:
                                NS_FATAL_ERROR ("unsupported triggerQuantity");
                                break;
                              }

                            // Inequality A5-4 (Leaving condition 2): Mn + Ofn + Ocn + Hys < Thresh2

                            leavingCond = mn + ofn + ocn + hys < thresh2;

                            if (!leavingCond)
                              {
                                CancelLeavingTrigger (measId, cellId);
                              }

                            /*
                             * Whatever the result of leaving condition #2, this
                             * cell is still "in", because leaving condition #1
                             * is already true.
                             */
                            concernedCellsLeaving.push_back (cellId);
                            eventLeavingCondApplicable = true;

                            NS_LOG_LOGIC (this << " event A5: neighbor cell " << cellId
                                               << " mn=" << mn << " mp=" << mp
                                               << " thresh2=" << thresh2
                                               << " thresh1=" << thresh1
                                               << " leavingCond=" << leavingCond);

                          } // end of if (measReportIt->second.cellsTriggeredList.find (cellId)
                            //            != measReportIt->second.cellsTriggeredList.end ())

                      } // end of for (storedMeasIt)

                  } // end of else of if (reportConfigEutra.timeToTrigger == 0)

                NS_LOG_LOGIC (this << " event A5: serving cell " << m_cellId
                                   << " mp=" << mp << " thresh1=" << thresh1
                                   << " leavingCond=" << leavingCond);

              } // end of if (leavingCond)
            else
              {
                if (reportConfigEutra.timeToTrigger > 0)
                  {
                    CancelLeavingTrigger (measId);
                  }

                // check leaving condition #2

                for (std::map<uint16_t, MeasValues>::iterator storedMeasIt = m_storedMeasValues.begin ();
                     storedMeasIt != m_storedMeasValues.end ();
                     ++storedMeasIt)
                  {
                    uint16_t cellId = storedMeasIt->first;
                    if (cellId == m_cellId)
                      {
                        continue;
                      }

                    if (measReportIt->second.cellsTriggeredList.find (cellId)
                        != measReportIt->second.cellsTriggeredList.end ())
                      {
                        switch (reportConfigEutra.triggerQuantity)
                          {
                          case LteRrcSap::ReportConfigEutra::RSRP:
                            mn = storedMeasIt->second.rsrp;
                            break;
                          case LteRrcSap::ReportConfigEutra::RSRQ:
                            mn = storedMeasIt->second.rsrq;
                            break;
                          default:
                            NS_FATAL_ERROR ("unsupported triggerQuantity");
                            break;
                          }

                        // Inequality A5-4 (Leaving condition 2): Mn + Ofn + Ocn + Hys < Thresh2
                        leavingCond = mn + ofn + ocn + hys < thresh2;

                        if (leavingCond)
                          {
                            concernedCellsLeaving.push_back (cellId);
                            eventLeavingCondApplicable = true;
                          }

                        NS_LOG_LOGIC (this << " event A5: neighbor cell " << cellId
                                           << " mn=" << mn << " mp=" << mp
                                           << " thresh2=" << thresh2
                                           << " thresh1=" << thresh1
                                           << " leavingCond=" << leavingCond);

                      } // end of if (measReportIt->second.cellsTriggeredList.find (cellId)
                        //            != measReportIt->second.cellsTriggeredList.end ())

                  } // end of for (storedMeasIt)

              } // end of else of if (leavingCond)

          } // end of if (isMeasIdInReportList)

      } // end of case LteRrcSap::ReportConfigEutra::EVENT_A5

      break;

    default:
      NS_FATAL_ERROR ("unsupported eventId " << reportConfigEutra.eventId);
      break;

    } // switch (event type)

  NS_LOG_LOGIC (this << " eventEntryCondApplicable=" << eventEntryCondApplicable
                     << " eventLeavingCondApplicable=" << eventLeavingCondApplicable);

  if (eventEntryCondApplicable)
    {
      if (reportConfigEutra.timeToTrigger == 0)
        {
          VarMeasReportListAdd (measId, concernedCellsEntry);
        }
      else
        {
          PendingTrigger_t t;
          t.measId = measId;
          t.concernedCells = concernedCellsEntry;
          t.timer = Simulator::Schedule (MilliSeconds (reportConfigEutra.timeToTrigger),
                                         &LteUeRrc::VarMeasReportListAdd, this,
                                         measId, concernedCellsEntry);
          std::map<uint8_t, std::list<PendingTrigger_t> >::iterator
            enteringTriggerIt = m_enteringTriggerQueue.find (measId);
          NS_ASSERT (enteringTriggerIt != m_enteringTriggerQueue.end ());
          enteringTriggerIt->second.push_back (t);
        }
    }

  if (eventLeavingCondApplicable)
    {
      // reportOnLeave will only be set when eventId = eventA3
      bool reportOnLeave = (reportConfigEutra.eventId == LteRrcSap::ReportConfigEutra::EVENT_A3)
        && reportConfigEutra.reportOnLeave;

      if (reportConfigEutra.timeToTrigger == 0)
        {
          VarMeasReportListErase (measId, concernedCellsLeaving, reportOnLeave);
        }
      else
        {
          PendingTrigger_t t;
          t.measId = measId;
          t.concernedCells = concernedCellsLeaving;
          t.timer = Simulator::Schedule (MilliSeconds (reportConfigEutra.timeToTrigger),
                                         &LteUeRrc::VarMeasReportListErase, this,
                                         measId, concernedCellsLeaving, reportOnLeave);
          std::map<uint8_t, std::list<PendingTrigger_t> >::iterator
            leavingTriggerIt = m_leavingTriggerQueue.find (measId);
          NS_ASSERT (leavingTriggerIt != m_leavingTriggerQueue.end ());
          leavingTriggerIt->second.push_back (t);
        }
    }

} // end of void LteUeRrc::MeasurementReportTriggering (uint8_t measId)

void
LteUeRrc::CancelEnteringTrigger (uint8_t measId)
{
  NS_LOG_FUNCTION (this << (uint16_t) measId);

  std::map<uint8_t, std::list<PendingTrigger_t> >::iterator
    it1 = m_enteringTriggerQueue.find (measId);
  NS_ASSERT (it1 != m_enteringTriggerQueue.end ());

  if (!it1->second.empty ())
    {
      std::list<PendingTrigger_t>::iterator it2;
      for (it2 = it1->second.begin (); it2 != it1->second.end (); ++it2)
        {
          NS_ASSERT (it2->measId == measId);
          NS_LOG_LOGIC (this << " canceling entering time-to-trigger event at "
                             << Simulator::GetDelayLeft (it2->timer).GetSeconds ());
          Simulator::Cancel (it2->timer);
        }

      it1->second.clear ();
    }
}

void
LteUeRrc::CancelEnteringTrigger (uint8_t measId, uint16_t cellId)
{
  NS_LOG_FUNCTION (this << (uint16_t) measId << cellId);

  std::map<uint8_t, std::list<PendingTrigger_t> >::iterator
    it1 = m_enteringTriggerQueue.find (measId);
  NS_ASSERT (it1 != m_enteringTriggerQueue.end ());

  std::list<PendingTrigger_t>::iterator it2 = it1->second.begin ();
  while (it2 != it1->second.end ())
    {
      NS_ASSERT (it2->measId == measId);

      ConcernedCells_t::iterator it3;
      for (it3 = it2->concernedCells.begin ();
           it3 != it2->concernedCells.end (); ++it3)
        {
          if (*it3 == cellId)
            {
              it3 = it2->concernedCells.erase (it3);
            }
        }

      if (it2->concernedCells.empty ())
        {
          NS_LOG_LOGIC (this << " canceling entering time-to-trigger event at "
                             << Simulator::GetDelayLeft (it2->timer).GetSeconds ());
          Simulator::Cancel (it2->timer);
          it2 = it1->second.erase (it2);
        }
      else
        {
          it2++;
        }
    }
}

void
LteUeRrc::CancelLeavingTrigger (uint8_t measId)
{
  NS_LOG_FUNCTION (this << (uint16_t) measId);

  std::map<uint8_t, std::list<PendingTrigger_t> >::iterator
    it1 = m_leavingTriggerQueue.find (measId);
  NS_ASSERT (it1 != m_leavingTriggerQueue.end ());

  if (!it1->second.empty ())
    {
      std::list<PendingTrigger_t>::iterator it2;
      for (it2 = it1->second.begin (); it2 != it1->second.end (); ++it2)
        {
          NS_ASSERT (it2->measId == measId);
          NS_LOG_LOGIC (this << " canceling leaving time-to-trigger event at "
                             << Simulator::GetDelayLeft (it2->timer).GetSeconds ());
          Simulator::Cancel (it2->timer);
        }

      it1->second.clear ();
    }
}

void
LteUeRrc::CancelLeavingTrigger (uint8_t measId, uint16_t cellId)
{
  NS_LOG_FUNCTION (this << (uint16_t) measId << cellId);

  std::map<uint8_t, std::list<PendingTrigger_t> >::iterator
    it1 = m_leavingTriggerQueue.find (measId);
  NS_ASSERT (it1 != m_leavingTriggerQueue.end ());

  std::list<PendingTrigger_t>::iterator it2 = it1->second.begin ();
  while (it2 != it1->second.end ())
    {
      NS_ASSERT (it2->measId == measId);

      ConcernedCells_t::iterator it3;
      for (it3 = it2->concernedCells.begin ();
           it3 != it2->concernedCells.end (); ++it3)
        {
          if (*it3 == cellId)
            {
              it3 = it2->concernedCells.erase (it3);
            }
        }

      if (it2->concernedCells.empty ())
        {
          NS_LOG_LOGIC (this << " canceling leaving time-to-trigger event at "
                             << Simulator::GetDelayLeft (it2->timer).GetSeconds ());
          Simulator::Cancel (it2->timer);
          it2 = it1->second.erase (it2);
        }
      else
        {
          it2++;
        }
    }
}

void
LteUeRrc::VarMeasReportListAdd (uint8_t measId, ConcernedCells_t enteringCells)
{
  NS_LOG_FUNCTION (this << (uint16_t) measId);
  NS_ASSERT (!enteringCells.empty ());

  std::map<uint8_t, VarMeasReport>::iterator
    measReportIt = m_varMeasReportList.find (measId);

  if (measReportIt == m_varMeasReportList.end ())
    {
      VarMeasReport r;
      r.measId = measId;
      std::pair<uint8_t, VarMeasReport> val (measId, r);
      std::pair<std::map<uint8_t, VarMeasReport>::iterator, bool>
        ret = m_varMeasReportList.insert (val);
      NS_ASSERT_MSG (ret.second == true, "element already existed");
      measReportIt = ret.first;
    }

  NS_ASSERT (measReportIt != m_varMeasReportList.end ());

  for (ConcernedCells_t::const_iterator it = enteringCells.begin ();
       it != enteringCells.end ();
       ++it)
    {
      measReportIt->second.cellsTriggeredList.insert (*it);
    }

  NS_ASSERT (!measReportIt->second.cellsTriggeredList.empty ());
  measReportIt->second.numberOfReportsSent = 0;
  measReportIt->second.periodicReportTimer
    = Simulator::Schedule (UE_MEASUREMENT_REPORT_DELAY,
                           &LteUeRrc::SendMeasurementReport,
                           this, measId);

  std::map<uint8_t, std::list<PendingTrigger_t> >::iterator
    enteringTriggerIt = m_enteringTriggerQueue.find (measId);
  NS_ASSERT (enteringTriggerIt != m_enteringTriggerQueue.end ());
  if (!enteringTriggerIt->second.empty ())
    {
      /*
       * Assumptions at this point:
       *  - the call to this function was delayed by time-to-trigger;
       *  - the time-to-trigger delay is fixed (not adaptive/dynamic); and
       *  - the first element in the list is associated with this function call.
       */
      enteringTriggerIt->second.pop_front ();

      if (!enteringTriggerIt->second.empty ())
        {
          /*
           * To prevent the same set of cells triggering again in the future,
           * we clean up the time-to-trigger queue. This case might occur when
           * time-to-trigger > 200 ms.
           */
          for (ConcernedCells_t::const_iterator it = enteringCells.begin ();
               it != enteringCells.end (); ++it)
            {
              CancelEnteringTrigger (measId, *it);
            }
        }

    } // end of if (!enteringTriggerIt->second.empty ())

} // end of LteUeRrc::VarMeasReportListAdd

void
LteUeRrc::VarMeasReportListErase (uint8_t measId, ConcernedCells_t leavingCells,
                                  bool reportOnLeave)
{
  NS_LOG_FUNCTION (this << (uint16_t) measId);
  NS_ASSERT (!leavingCells.empty ());

  std::map<uint8_t, VarMeasReport>::iterator
    measReportIt = m_varMeasReportList.find (measId);
  NS_ASSERT (measReportIt != m_varMeasReportList.end ());

  for (ConcernedCells_t::const_iterator it = leavingCells.begin ();
       it != leavingCells.end ();
       ++it)
    {
      measReportIt->second.cellsTriggeredList.erase (*it);
    }

  if (reportOnLeave)
    {
      // runs immediately without UE_MEASUREMENT_REPORT_DELAY
      SendMeasurementReport (measId);
    }

  if (measReportIt->second.cellsTriggeredList.empty ())
    {
      measReportIt->second.periodicReportTimer.Cancel ();
      m_varMeasReportList.erase (measReportIt);
    }

  std::map<uint8_t, std::list<PendingTrigger_t> >::iterator
    leavingTriggerIt = m_leavingTriggerQueue.find (measId);
  NS_ASSERT (leavingTriggerIt != m_leavingTriggerQueue.end ());
  if (!leavingTriggerIt->second.empty ())
    {
      /*
       * Assumptions at this point:
       *  - the call to this function was delayed by time-to-trigger; and
       *  - the time-to-trigger delay is fixed (not adaptive/dynamic); and
       *  - the first element in the list is associated with this function call.
       */
      leavingTriggerIt->second.pop_front ();

      if (!leavingTriggerIt->second.empty ())
        {
          /*
           * To prevent the same set of cells triggering again in the future,
           * we clean up the time-to-trigger queue. This case might occur when
           * time-to-trigger > 200 ms.
           */
          for (ConcernedCells_t::const_iterator it = leavingCells.begin ();
               it != leavingCells.end (); ++it)
            {
              CancelLeavingTrigger (measId, *it);
            }
        }

    } // end of if (!leavingTriggerIt->second.empty ())

} // end of LteUeRrc::VarMeasReportListErase

void
LteUeRrc::VarMeasReportListClear (uint8_t measId)
{
  NS_LOG_FUNCTION (this << (uint16_t) measId);

  // remove the measurement reporting entry for this measId from the VarMeasReportList
  std::map<uint8_t, VarMeasReport>::iterator
    measReportIt = m_varMeasReportList.find (measId);
  if (measReportIt != m_varMeasReportList.end ())
    {
      NS_LOG_LOGIC (this << " deleting existing report for measId " << (uint16_t) measId);
      measReportIt->second.periodicReportTimer.Cancel ();
      m_varMeasReportList.erase (measReportIt);
    }

  CancelEnteringTrigger (measId);
  CancelLeavingTrigger (measId);
}

void 
LteUeRrc::SendMeasurementReport (uint8_t measId)
{
  NS_LOG_FUNCTION (this << (uint16_t) measId);
  //  3GPP TS 36.331 section 5.5.5 Measurement reporting

  std::map<uint8_t, LteRrcSap::MeasIdToAddMod>::iterator 
    measIdIt = m_varMeasConfig.measIdList.find (measId);
  NS_ASSERT (measIdIt != m_varMeasConfig.measIdList.end ());

  std::map<uint8_t, LteRrcSap::ReportConfigToAddMod>::iterator 
    reportConfigIt = m_varMeasConfig.reportConfigList.find (measIdIt->second.reportConfigId);
  NS_ASSERT (reportConfigIt != m_varMeasConfig.reportConfigList.end ());
  LteRrcSap::ReportConfigEutra& reportConfigEutra = reportConfigIt->second.reportConfigEutra;

  LteRrcSap::MeasurementReport measurementReport;
  LteRrcSap::MeasResults& measResults = measurementReport.measResults;
  measResults.measId = measId;

  std::map<uint16_t, MeasValues>::iterator servingMeasIt = m_storedMeasValues.find (m_cellId);
  NS_ASSERT (servingMeasIt != m_storedMeasValues.end ());
  measResults.rsrpResult = EutranMeasurementMapping::Dbm2RsrpRange (servingMeasIt->second.rsrp);
  measResults.rsrqResult = EutranMeasurementMapping::Db2RsrqRange (servingMeasIt->second.rsrq);
  NS_LOG_INFO (this << " reporting serving cell "
               "RSRP " << (uint32_t) measResults.rsrpResult << " (" << servingMeasIt->second.rsrp << " dBm) "
               "RSRQ " << (uint32_t) measResults.rsrqResult << " (" << servingMeasIt->second.rsrq << " dB)");
  measResults.haveMeasResultNeighCells = false;
  std::map<uint8_t, VarMeasReport>::iterator measReportIt = m_varMeasReportList.find (measId);
  if (measReportIt == m_varMeasReportList.end ())
    {
      NS_LOG_ERROR ("no entry found in m_varMeasReportList for measId " << (uint32_t) measId);
    }
  else
    {
      if (!(measReportIt->second.cellsTriggeredList.empty ()))
        {
          std::multimap<double, uint16_t> sortedNeighCells;
          for (std::set<uint16_t>::iterator cellsTriggeredIt = measReportIt->second.cellsTriggeredList.begin ();
               cellsTriggeredIt != measReportIt->second.cellsTriggeredList.end ();
               ++cellsTriggeredIt)
            {
              uint16_t cellId = *cellsTriggeredIt;
              if (cellId != m_cellId)
                {
                  std::map<uint16_t, MeasValues>::iterator neighborMeasIt = m_storedMeasValues.find (cellId);
                  double triggerValue;
                  switch (reportConfigEutra.triggerQuantity)
                    {
                    case LteRrcSap::ReportConfigEutra::RSRP:
                      triggerValue = neighborMeasIt->second.rsrp;
                      break;
                    case LteRrcSap::ReportConfigEutra::RSRQ:
                      triggerValue = neighborMeasIt->second.rsrq;
                      break;
                    default:
                      NS_FATAL_ERROR ("unsupported triggerQuantity");
                      break;
                    }
                  sortedNeighCells.insert (std::pair<double, uint16_t> (triggerValue, cellId));
                }
            }

          std::multimap<double, uint16_t>::reverse_iterator sortedNeighCellsIt;
          uint32_t count;
          for (sortedNeighCellsIt = sortedNeighCells.rbegin (), count = 0;
               sortedNeighCellsIt != sortedNeighCells.rend () && count < reportConfigEutra.maxReportCells;
               ++sortedNeighCellsIt, ++count)
            {
              uint16_t cellId = sortedNeighCellsIt->second;
              std::map<uint16_t, MeasValues>::iterator neighborMeasIt = m_storedMeasValues.find (cellId);
              NS_ASSERT (neighborMeasIt != m_storedMeasValues.end ());
              LteRrcSap::MeasResultEutra measResultEutra;
              measResultEutra.physCellId = cellId;
              measResultEutra.haveCgiInfo = false;
              measResultEutra.haveRsrpResult = true;
              measResultEutra.rsrpResult = EutranMeasurementMapping::Dbm2RsrpRange (neighborMeasIt->second.rsrp);
              measResultEutra.haveRsrqResult = true;
              measResultEutra.rsrqResult = EutranMeasurementMapping::Db2RsrqRange (neighborMeasIt->second.rsrq);
              NS_LOG_INFO (this << " reporting neighbor cell " << (uint32_t) measResultEutra.physCellId 
                                << " RSRP " << (uint32_t) measResultEutra.rsrpResult
                                << " (" << neighborMeasIt->second.rsrp << " dBm)"
                                << " RSRQ " << (uint32_t) measResultEutra.rsrqResult
                                << " (" << neighborMeasIt->second.rsrq << " dB)");
              measResults.measResultListEutra.push_back (measResultEutra);
              measResults.haveMeasResultNeighCells = true;
            }
        }
      else
        {
          NS_LOG_WARN (this << " cellsTriggeredList is empty");
        }

      measResults.haveScellsMeas = false;
      std::map<uint16_t, MeasValues>::iterator sCellsMeasIt =  m_storedScellMeasValues.begin ();
      if (sCellsMeasIt != m_storedScellMeasValues.end ())
        {
          measResults.haveScellsMeas = true;
          measResults.measScellResultList.haveMeasurementResultsServingSCells = true;
          measResults.measScellResultList.haveMeasurementResultsNeighCell = false;


          for ( sCellsMeasIt = m_storedScellMeasValues.begin (); 
                sCellsMeasIt != m_storedScellMeasValues.end (); ++sCellsMeasIt)
            {
              LteRrcSap::MeasResultScell measResultScell;
              measResultScell.servFreqId = sCellsMeasIt->first;
              measResultScell.haveRsrpResult =  true;
              measResultScell.haveRsrqResult =  true;
              measResultScell.rsrpResult = EutranMeasurementMapping::Dbm2RsrpRange (sCellsMeasIt->second.rsrp);
              measResultScell.rsrqResult = EutranMeasurementMapping::Db2RsrqRange (sCellsMeasIt->second.rsrq); 
              measResults.measScellResultList.measResultScell.push_back (measResultScell);
            }
        }

      /*
       * The current LteRrcSap implementation is broken in that it does not
       * allow for infinite values of reportAmount, which is probably the most
       * reasonable setting. So we just always assume infinite reportAmount.
       */
      measReportIt->second.numberOfReportsSent++;
      measReportIt->second.periodicReportTimer.Cancel ();

      Time reportInterval;
      switch (reportConfigEutra.reportInterval)
        {
        case LteRrcSap::ReportConfigEutra::MS120:
          reportInterval = MilliSeconds (120);
          break;
        case LteRrcSap::ReportConfigEutra::MS240:
          reportInterval = MilliSeconds (240);
          break;
        case LteRrcSap::ReportConfigEutra::MS480:
          reportInterval = MilliSeconds (480);
          break;
        case LteRrcSap::ReportConfigEutra::MS640:
          reportInterval = MilliSeconds (640);
          break;
        case LteRrcSap::ReportConfigEutra::MS1024:
          reportInterval = MilliSeconds (1024);
          break;
        case LteRrcSap::ReportConfigEutra::MS2048:
          reportInterval = MilliSeconds (2048);
          break;
        case LteRrcSap::ReportConfigEutra::MS5120:
          reportInterval = MilliSeconds (5120);
          break;
        case LteRrcSap::ReportConfigEutra::MS10240:
          reportInterval = MilliSeconds (10240);
          break;
        case LteRrcSap::ReportConfigEutra::MIN1:
          reportInterval = Seconds (60);
          break;
        case LteRrcSap::ReportConfigEutra::MIN6:
          reportInterval = Seconds (360);
          break;
        case LteRrcSap::ReportConfigEutra::MIN12:
          reportInterval = Seconds (720);
          break;
        case LteRrcSap::ReportConfigEutra::MIN30:
          reportInterval = Seconds (1800);
          break;
        case LteRrcSap::ReportConfigEutra::MIN60:
          reportInterval = Seconds (3600);
          break;
        default:
          NS_FATAL_ERROR ("Unsupported reportInterval " << (uint16_t) reportConfigEutra.reportInterval);
          break;
        }

      // schedule the next measurement reporting
      measReportIt->second.periodicReportTimer 
        = Simulator::Schedule (reportInterval,
                               &LteUeRrc::SendMeasurementReport,
                               this, measId);

      // send the measurement report to eNodeB
      m_rrcSapUser->SendMeasurementReport (measurementReport);
    } 
}

void 
LteUeRrc::StartConnection ()
{
  NS_LOG_FUNCTION (this << m_imsi);
  NS_ASSERT (m_hasReceivedMib);
  NS_ASSERT (m_hasReceivedSib2);
  m_connectionPending = false; // reset the flag
  m_requirepagingflag = false; // reset the flag
  SwitchToState (IDLE_RANDOM_ACCESS);
  m_cmacSapProvider.at (0)->StartContentionBasedRandomAccessProcedure ();
}

void 
LteUeRrc::LeaveConnectedMode ()
{
  NS_LOG_FUNCTION (this << m_imsi);
  m_asSapUser->NotifyConnectionReleased ();
  m_cmacSapProvider.at (0)->RemoveLc (1);
  std::map<uint8_t, Ptr<LteDataRadioBearerInfo> >::iterator it;
  for (it = m_drbMap.begin (); it != m_drbMap.end (); ++it)
    {
      m_cmacSapProvider.at (0)->RemoveLc (it->second->m_logicalChannelIdentity);
    }
  m_drbMap.clear ();
  m_bid2DrbidMap.clear ();
  m_srb1 = 0;
  SwitchToState (IDLE_CAMPED_NORMALLY);
}

void
LteUeRrc::ConnectionTimeout ()
{
  NS_LOG_FUNCTION (this << m_imsi);
  m_cmacSapProvider.at (0)->Reset ();       // reset the MAC
  m_hasReceivedSib2 = false;         // invalidate the previously received SIB2
  SwitchToState (IDLE_CAMPED_NORMALLY);
  m_connectionTimeoutTrace (m_imsi, m_cellId, m_rnti);
  m_asSapUser->NotifyConnectionFailed ();  // inform upper layer
}

void
LteUeRrc::DisposeOldSrb1 ()
{
  NS_LOG_FUNCTION (this);
  m_srb1Old = 0;
}

uint8_t 
LteUeRrc::Bid2Drbid (uint8_t bid)
{
  std::map<uint8_t, uint8_t>::iterator it = m_bid2DrbidMap.find (bid);
  //NS_ASSERT_MSG (it != m_bid2DrbidMap.end (), "could not find BID " << bid);
  if (it == m_bid2DrbidMap.end ())
    {
      return 0;
    }
  else
    {
  return it->second;
    }
}

void 
LteUeRrc::SwitchToState (State newState)
{
  NS_LOG_FUNCTION (this << ToString (newState));
  std::cout<< Simulator::Now().GetSeconds()<<" "<<GetRnti()<<" LteUeRrc::SwitchToState from "<<g_ueRrcStateName[m_state]<<" --> "<<g_ueRrcStateName[newState]<<std::endl;
  State oldState = m_state;
  m_state = newState;
  NS_LOG_INFO (this << " IMSI " << m_imsi << " RNTI " << m_rnti << " UeRrc "
                    << ToString (oldState) << " --> " << ToString (newState));
  m_stateTransitionTrace (m_imsi, m_cellId, m_rnti, oldState, newState);
  double idleTime;
  //int nt  =  10;
  Callback<void, double> two;


  switch (oldState)
  {
      case IDLE_START:
      case IDLE_CELL_SEARCH:
      case IDLE_WAIT_MIB_SIB1:
      case IDLE_WAIT_MIB:
      case IDLE_WAIT_SIB1:
      case IDLE_PAGING:
      case IDLE_CAMPED_NORMALLY:
      case IDLE_WAIT_SIB2:
      case IDLE_RANDOM_ACCESS:
      case IDLE_CONNECTING:
          break;
      case CONNECTED_NORMALLY:
          //TODO: Update from rrc reconfiguration
          m_t3324 = m_t3324_d;
          m_t3412 = m_t3412_d;
          m_edrx_cycle   =  ((m_edrx_cycle_d));   // ptw ranges from 2.56 to 40.96 (2.56/16)

          two = MakeCallback(&EnergyModuleLte::Only_idle_decrease, &LEM);
          idleTime= ((Simulator::Now()-m_lastUpdateTime).GetNanoSeconds());
          m_lastUpdateTime=Simulator::Now();
          std::cout<<Simulator::Now().GetSeconds()<<" "<<GetRnti() <<" ";
          two(idleTime);

          break;
      case CONNECTED_HANDOVER:
      case CONNECTED_PHY_PROBLEM:
      case CONNECTED_REESTABLISHING:
          break;
      case SUSPEND_PAGING:
          two = MakeCallback(&EnergyModuleLte::Only_paging_decrease, &LEM);
          idleTime= ((Simulator::Now()-m_lastUpdateTime).GetNanoSeconds());
          m_lastUpdateTime=Simulator::Now();
          std::cout<<Simulator::Now().GetSeconds()<<" "<<GetRnti() <<" ";
          two(idleTime);
          break;

      case IDLE_SUSPEND:
          two = MakeCallback(&EnergyModuleLte::Only_drx_decrease, &LEM);
          idleTime= ((Simulator::Now()-m_lastUpdateTime).GetNanoSeconds());
          m_lastUpdateTime=Simulator::Now();
          std::cout<<Simulator::Now().GetSeconds()<<" "<<GetRnti() <<" ";
          two(idleTime);
          break;

      case IDLE_PSM:
          two = MakeCallback(&EnergyModuleLte::Only_psm_decrease, &LEM);
          idleTime= ((Simulator::Now()-m_lastUpdateTime).GetNanoSeconds());
          m_lastUpdateTime=Simulator::Now();
          std::cout<<Simulator::Now().GetSeconds()<<" "<<GetRnti() <<" ";
          two(idleTime);
          //TODO: Update from rrc reconfiguration
          m_t3324 = m_t3324_d;
          m_t3412 = m_t3412_d;
          m_edrx_cycle   = m_edrx_cycle_d;   // ptw ranges from 2.56 to 40.96 (2.56/16)
          break;

      default:
          break;
  }

  switch (newState)
    {
    case IDLE_START:
      NS_FATAL_ERROR ("cannot switch to an initial state");
      break;

      case IDLE_CELL_SEARCH:
      case IDLE_WAIT_MIB_SIB1:
      case IDLE_WAIT_MIB:
      case IDLE_WAIT_SIB1:
      case IDLE_PAGING:
          break;

    case IDLE_CAMPED_NORMALLY:
      if (m_connectionPending)
        {
          SwitchToState (IDLE_WAIT_SIB2);
        }
      break;

      case IDLE_WAIT_SIB2:
          if (m_hasReceivedSib2)
          {
              NS_ASSERT (m_connectionPending);
              //StartConnection ();
          }
          break;

      case IDLE_RANDOM_ACCESS:
      case IDLE_CONNECTING:
      case CONNECTED_NORMALLY:
      case CONNECTED_HANDOVER:
      case CONNECTED_PHY_PROBLEM:
      case CONNECTED_REESTABLISHING:
           break;

      case IDLE_SUSPEND:

           break;
      case SUSPEND_PAGING:
          if(true == m_hasReceivedPaging)
          {
              LteRrcSap::RrcConnectionRequest msg;
              msg.ueIdentity = m_imsi;
              m_rrcSapUser->SendRrcConnectionRequest (msg);
              m_connectionTimeout = Simulator::Schedule (m_t300,
                                                         &LteUeRrc::ConnectionTimeout,
                                                         this);
          }
          else if(m_t3324 <= 0 && false == m_hasReceivedPaging )
          {
              SwitchToState(IDLE_PSM);
          }
          else if (m_t3324>0)
          {
              SwitchToState(IDLE_SUSPEND);
          }
          break;
      case IDLE_PSM:

           break;

      default:
           break;
  }
}

void
LteUeRrc::DoComponentCarrierEnabling (std::vector<uint8_t> res)
  {
    NS_LOG_INFO (this);
  }

void
LteUeRrc::SaveScellUeMeasurements (uint16_t sCellId, double rsrp, double rsrq,
                                   bool useLayer3Filtering, uint16_t componentCarrierId)
{
  NS_LOG_FUNCTION (this << sCellId << componentCarrierId << rsrp << rsrq << useLayer3Filtering);
  if (sCellId == m_cellId)
    {

      std::map<uint16_t, MeasValues>::iterator storedMeasIt = m_storedScellMeasValues.find (componentCarrierId);

      if (storedMeasIt != m_storedScellMeasValues.end ())
        {
          if (useLayer3Filtering)
            {
              // F_n = (1-a) F_{n-1} + a M_n
              storedMeasIt->second.rsrp = (1 - m_varMeasConfig.aRsrp) * storedMeasIt->second.rsrp
                + m_varMeasConfig.aRsrp * rsrp;

              if (std::isnan (storedMeasIt->second.rsrq))
                {
                  // the previous RSRQ measurements provided UE PHY are invalid
                  storedMeasIt->second.rsrq = rsrq;   // replace it with unfiltered value
                }
              else
                {
                  storedMeasIt->second.rsrq = (1 - m_varMeasConfig.aRsrq) * storedMeasIt->second.rsrq
                    + m_varMeasConfig.aRsrq * rsrq;
                }
            }
          else
            {
              storedMeasIt->second.rsrp = rsrp;
              storedMeasIt->second.rsrq = rsrq;
            }
        }
      else
        {
          // first value is always unfiltered
          MeasValues v;
          v.rsrp = rsrp;
          v.rsrq = rsrq;
          std::pair<uint16_t, MeasValues> val (componentCarrierId, v);
          std::pair<std::map<uint16_t, MeasValues>::iterator, bool>
            ret = m_storedScellMeasValues.insert (val);
          NS_ASSERT_MSG (ret.second == true, "element already existed");
          storedMeasIt = ret.first;
        }

      NS_LOG_DEBUG (this << " IMSI " << m_imsi << " state " << ToString (m_state)
                    << ", measured cell " << sCellId
                    << ", carrier component Id " << componentCarrierId
                    << ", new RSRP " << rsrp << " stored " << storedMeasIt->second.rsrp
                    << ", new RSRQ " << rsrq << " stored " << storedMeasIt->second.rsrq);
      storedMeasIt->second.timestamp = Simulator::Now ();
    }
  else 
    {
      NS_LOG_DEBUG (this << " IMSI " << m_imsi << "measurement on SCC from not serving cell ");
    }

}   // end of void SaveUeMeasurements


} // namespace ns3

