/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 TELEMATICS LAB, DEE - Politecnico di Bari
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
 * Author: Giuseppe Piro  <g.piro@poliba.it>
 *         Marco Miozzo <mmiozzo@cttc.es>
 */

#include <ns3/object-factory.h>
#include <ns3/log.h>
#include <cfloat>
#include <cmath>
#include <ns3/simulator.h>
#include <ns3/attribute-accessor-helper.h>
#include <ns3/double.h>


#include "lte-enb-phy.h"
#include "lte-ue-phy.h"
#include "lte-net-device.h"
#include "lte-spectrum-value-helper.h"
#include "lte-control-messages.h"
#include "lte-enb-net-device.h"
#include "lte-ue-rrc.h"
#include "lte-enb-mac.h"
#include <ns3/lte-common.h>
#include <ns3/lte-vendor-specific-parameters.h>

// WILD HACK for the inizialization of direct eNB-UE ctrl messaging
#include <ns3/node-list.h>
#include <ns3/node.h>
#include <ns3/lte-ue-net-device.h>
#include <ns3/pointer.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LteEnbPhy");

NS_OBJECT_ENSURE_REGISTERED (LteEnbPhy);

/**
 * Duration of the data portion of a DL subframe.
 * Equals to "TTI length * (11/14) - margin".
 * Data portion is fixed to 11 symbols out of the available 14 symbols.
 * 1 nanosecond margin is added to avoid overlapping simulator events.
 */
static const Time DL_DATA_DURATION = NanoSeconds (785714 -1);

/**
 * Delay from the start of a DL subframe to transmission of the data portion.
 * Equals to "TTI length * (3/14)".
 * Control portion is fixed to 3 symbols out of the available 14 symbols.
 */
static const Time DL_CTRL_DELAY_FROM_SUBFRAME_START = NanoSeconds (214286);

////////////////////////////////////////
// member SAP forwarders
////////////////////////////////////////

/// \todo SetBandwidth() and SetCellId() can be removed.
class EnbMemberLteEnbPhySapProvider : public LteEnbPhySapProvider
{
public:
  /**
   * Constructor
   *
   * \param phy the ENB Phy
   */
  EnbMemberLteEnbPhySapProvider (LteEnbPhy* phy);

  // inherited from LteEnbPhySapProvider
  virtual void SendMacPdu (Ptr<Packet> p);
  virtual void SendLteControlMessage (Ptr<LteControlMessage> msg);
  virtual uint8_t GetMacChTtiDelay ();
  /**
   * Set bandwidth function
   *
   * \param ulBandwidth the UL bandwidth
   * \param dlBandwidth the DL bandwidth
   */
  virtual void SetBandwidth (uint8_t ulBandwidth, uint8_t dlBandwidth);
  /**
   * Set Cell ID function
   *
   * \param cellId the cell ID
   */
  virtual void SetCellId (uint16_t cellId);


private:
  LteEnbPhy* m_phy; ///< the ENB Phy
};

EnbMemberLteEnbPhySapProvider::EnbMemberLteEnbPhySapProvider (LteEnbPhy* phy) : m_phy (phy)
{

}

void
EnbMemberLteEnbPhySapProvider::SendMacPdu (Ptr<Packet> p)
{
  m_phy->DoSendMacPdu (p);
}

void
EnbMemberLteEnbPhySapProvider::SetBandwidth (uint8_t ulBandwidth, uint8_t dlBandwidth)
{
  m_phy->DoSetBandwidth (ulBandwidth, dlBandwidth);
}

void
EnbMemberLteEnbPhySapProvider::SetCellId (uint16_t cellId)
{
  m_phy->DoSetCellId (cellId);
}

void
EnbMemberLteEnbPhySapProvider::SendLteControlMessage (Ptr<LteControlMessage> msg)
{
  m_phy->DoSendLteControlMessage (msg);
}

uint8_t
EnbMemberLteEnbPhySapProvider::GetMacChTtiDelay ()
{
  return (m_phy->DoGetMacChTtiDelay ());
}


////////////////////////////////////////
// generic LteEnbPhy methods
////////////////////////////////////////



LteEnbPhy::LteEnbPhy ()
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("This constructor should not be called");
}

LteEnbPhy::LteEnbPhy (Ptr<LteSpectrumPhy> dlPhy, Ptr<LteSpectrumPhy> ulPhy)
  : LtePhy (dlPhy, ulPhy),
    m_enbPhySapUser (0),
    m_enbCphySapUser (0),
    m_nrFrames (0),
    m_nrSubFrames (0),
    m_srsPeriodicity (0),
    m_srsStartTime (Seconds (0)),
    m_currentSrsOffset (0),
    m_interferenceSampleCounter (0),
    m_mibNbPartCounter(0), // Used by NB-IoT standard to trace the current MIB-NB part.
    m_schedulingInfoSib1NbGenerator(0),  // Used by NB-IoT standard to generate properly the SIB1-NB scheduling info.
    m_sib1NbRepetitions(0),  // Used by NB-IoT to trace the number of repetitions of SIB1-NB.
    m_T(128),
    m_nb(128),
    m_pf(0),
    m_po(0)
{
  m_enbPhySapProvider = new EnbMemberLteEnbPhySapProvider (this);
  m_enbCphySapProvider = new MemberLteEnbCphySapProvider<LteEnbPhy> (this);
  m_harqPhyModule = Create <LteHarqPhy> ();
  m_downlinkSpectrumPhy->SetHarqPhyModule (m_harqPhyModule);
  m_uplinkSpectrumPhy->SetHarqPhyModule (m_harqPhyModule);
}

TypeId
LteEnbPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LteEnbPhy")
    .SetParent<LtePhy> ()
    .SetGroupName("Lte")
    .AddConstructor<LteEnbPhy> ()
    .AddAttribute ("TxPower",
                   "Transmission power in dBm",
                   DoubleValue (30.0),
                   MakeDoubleAccessor (&LteEnbPhy::SetTxPower, 
                                       &LteEnbPhy::GetTxPower),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("NoiseFigure",
                   "Loss (dB) in the Signal-to-Noise-Ratio due to "
                   "non-idealities in the receiver.  According to Wikipedia "
                   "(http://en.wikipedia.org/wiki/Noise_figure), this is "
                   "\"the difference in decibels (dB) between"
                   " the noise output of the actual receiver to "
                   "the noise output of an ideal receiver with "
                   "the same overall gain and bandwidth when the receivers "
                   "are connected to sources at the standard noise "
                   "temperature T0.\"  In this model, we consider T0 = 290K.",
                   DoubleValue (5.0),
                   MakeDoubleAccessor (&LteEnbPhy::SetNoiseFigure, 
                                       &LteEnbPhy::GetNoiseFigure),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("MacToChannelDelay",
                   "The delay in TTI units that occurs between "
                   "a scheduling decision in the MAC and the actual "
                   "start of the transmission by the PHY. This is "
                   "intended to be used to model the latency of real PHY "
                   "and MAC implementations.",
                   UintegerValue (2),
                   MakeUintegerAccessor (&LteEnbPhy::SetMacChDelay, 
                                         &LteEnbPhy::GetMacChDelay),
                   MakeUintegerChecker<uint8_t> ())
    .AddTraceSource ("ReportUeSinr",
                     "Report UEs' averaged linear SINR",
                     MakeTraceSourceAccessor (&LteEnbPhy::m_reportUeSinr),
                     "ns3::LteEnbPhy::ReportUeSinrTracedCallback")
    .AddAttribute ("UeSinrSamplePeriod",
                   "The sampling period for reporting UEs' SINR stats.",
                   UintegerValue (1),  /// \todo In what unit is this?
                   MakeUintegerAccessor (&LteEnbPhy::m_srsSamplePeriod),
                   MakeUintegerChecker<uint16_t> ())
    .AddTraceSource ("ReportInterference",
                     "Report linear interference power per PHY RB",
                     MakeTraceSourceAccessor (&LteEnbPhy::m_reportInterferenceTrace),
                     "ns3::LteEnbPhy::ReportInterferenceTracedCallback")
    .AddAttribute ("InterferenceSamplePeriod",
                   "The sampling period for reporting interference stats",
                   UintegerValue (1),  /// \todo In what unit is this?
                   MakeUintegerAccessor (&LteEnbPhy::m_interferenceSamplePeriod),
                   MakeUintegerChecker<uint16_t> ())
    .AddTraceSource ("DlPhyTransmission",
                     "DL transmission PHY layer statistics.",
                     MakeTraceSourceAccessor (&LteEnbPhy::m_dlPhyTransmission),
                     "ns3::PhyTransmissionStatParameters::TracedCallback")
    .AddAttribute ("DlSpectrumPhy",
                   "The downlink LteSpectrumPhy associated to this LtePhy",
                   TypeId::ATTR_GET,
                   PointerValue (),
                   MakePointerAccessor (&LteEnbPhy::GetDlSpectrumPhy),
                   MakePointerChecker <LteSpectrumPhy> ())
    .AddAttribute ("UlSpectrumPhy",
                   "The uplink LteSpectrumPhy associated to this LtePhy",
                   TypeId::ATTR_GET,
                   PointerValue (),
                   MakePointerAccessor (&LteEnbPhy::GetUlSpectrumPhy),
                   MakePointerChecker <LteSpectrumPhy> ())
  ;
  return tid;
}


LteEnbPhy::~LteEnbPhy ()
{
}

void
LteEnbPhy::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_ueAttached.clear ();
  m_srsUeOffset.clear ();
  delete m_enbPhySapProvider;
  delete m_enbCphySapProvider;
  LtePhy::DoDispose ();
}

void
LteEnbPhy::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  bool haveNodeId = false;
  uint32_t nodeId = 0;
  if (m_netDevice != 0)
    {
      Ptr<Node> node = m_netDevice->GetNode ();
      if (node != 0)
        {
          nodeId = node->GetId ();
          haveNodeId = true;
        }
    }
  if (haveNodeId)
    {
      Simulator::ScheduleWithContext (nodeId, Seconds (0), &LteEnbPhy::StartFrame, this);
    }
  else
    {
      Simulator::ScheduleNow (&LteEnbPhy::StartFrame, this);
    }
  Ptr<SpectrumValue> noisePsd = LteSpectrumValueHelper::CreateNoisePowerSpectralDensity (m_ulEarfcn, m_ulBandwidth, m_noiseFigure);
  m_uplinkSpectrumPhy->SetNoisePowerSpectralDensity (noisePsd);
  LtePhy::DoInitialize ();
}


/*
 * \todo
 *
 * Implementation of the EnableNbIoTMode method needed to enable the NB-IoT mode.
 * It is a temporary solution. To get more information about
 * see the declaration of the method.
 */
void
LteEnbPhy::EnableNbIotMode(){
  m_EnabledNbIot = true;
}

void
LteEnbPhy::SetLteEnbPhySapUser (LteEnbPhySapUser* s)
{
  m_enbPhySapUser = s;
}

LteEnbPhySapProvider*
LteEnbPhy::GetLteEnbPhySapProvider ()
{
  return (m_enbPhySapProvider);
}

void
LteEnbPhy::SetLteEnbCphySapUser (LteEnbCphySapUser* s)
{
  NS_LOG_FUNCTION (this);
  m_enbCphySapUser = s;
}

LteEnbCphySapProvider*
LteEnbPhy::GetLteEnbCphySapProvider ()
{
  NS_LOG_FUNCTION (this);
  return (m_enbCphySapProvider);
}

void
LteEnbPhy::SetTxPower (double pow)
{
  NS_LOG_FUNCTION (this << pow);
  m_txPower = pow;
}

double
LteEnbPhy::GetTxPower () const
{
  NS_LOG_FUNCTION (this);
  return m_txPower;
}

int8_t
LteEnbPhy::DoGetReferenceSignalPower () const
{
  NS_LOG_FUNCTION (this);
  return m_txPower;
}

void
LteEnbPhy::SetNoiseFigure (double nf)
{
  NS_LOG_FUNCTION (this << nf);
  m_noiseFigure = nf;
}

double
LteEnbPhy::GetNoiseFigure () const
{
  NS_LOG_FUNCTION (this);
  return m_noiseFigure;
}

void
LteEnbPhy::SetMacChDelay (uint8_t delay)
{
  NS_LOG_FUNCTION (this << (int)delay);
  m_macChTtiDelay = delay;
#ifdef DEBUG_CONTROL_DATA_QUEUE
  std::cout << "m_packetBurstQueue.size():" << m_packetBurstQueue.size() << " m_controlMessagesQueue.size():" << m_controlMessagesQueue.size() << " m_dlControlMessagesQueue.size() "  << m_dlControlMessagesQueue.size() << "\n";
#endif
  for (int i = 0; i < m_macChTtiDelay; i++)
    {
      Ptr<PacketBurst> pb = CreateObject <PacketBurst> ();
      m_packetBurstQueue.push_back (pb);
      std::list<Ptr<LteControlMessage> > l;
      m_controlMessagesQueue.push_back (l);
      m_dlControlMessagesQueue.push_back (l);
      std::list<UlDciLteControlMessage> l1;
      m_ulDciQueue.push_back (l1);
    }
#ifdef DEBUG_CONTROL_DATA_QUEUE
  std::cout << "m_packetBurstQueue.size():" << m_packetBurstQueue.size() << " m_controlMessagesQueue.size():" << m_controlMessagesQueue.size() << " m_dlControlMessagesQueue.size() "  << m_dlControlMessagesQueue.size() << "\n";
#endif  
  for (int i = 0; i < UL_PUSCH_TTIS_DELAY; i++)
    {
      std::list<UlDciLteControlMessage> l1;
      m_ulDciQueue.push_back (l1);
    }
  for (int i = 0; i < DL_PDSCH_TTIS_DELAY; i++)
    {
      std::list<DlDciLteControlMessage> l1;
      m_dlDciQueue.push_back (l1);

      /*Ptr<PacketBurst> pb = CreateObject <PacketBurst> ();
      m_packetBurstQueue.push_back (pb);*/

      /*std::list<Ptr<LteControlMessage> > l;
      m_controlMessagesQueue.push_back (l);*/
    }
#ifdef DEBUG_CONTROL_DATA_QUEUE
  std::cout << "m_packetBurstQueue.size():" << m_packetBurstQueue.size() << " m_controlMessagesQueue.size():" << m_controlMessagesQueue.size() << " m_dlControlMessagesQueue.size() "  << m_dlControlMessagesQueue.size() << "\n\n";
#endif  
}

uint8_t
LteEnbPhy::GetMacChDelay (void) const
{
  return (m_macChTtiDelay);
}

Ptr<LteSpectrumPhy>
LteEnbPhy::GetDlSpectrumPhy () const
{
  return m_downlinkSpectrumPhy;
}

Ptr<LteSpectrumPhy>
LteEnbPhy::GetUlSpectrumPhy () const
{
  return m_uplinkSpectrumPhy;
}

bool
LteEnbPhy::AddUePhy (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << rnti);
  std::set <uint16_t>::iterator it;
  it = m_ueAttached.find (rnti);
  if (it == m_ueAttached.end ())
    {
      m_ueAttached.insert (rnti);
      return (true);
    }
  else
    {
      NS_LOG_ERROR ("UE already attached");
      return (false);
    }
}

bool
LteEnbPhy::DeleteUePhy (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << rnti);
  std::set <uint16_t>::iterator it;
  it = m_ueAttached.find (rnti);
  if (it == m_ueAttached.end ())
    {
      NS_LOG_ERROR ("UE not attached");
      return (false);
    }
  else
    {
      m_ueAttached.erase (it);
      return (true);
    }
}



void
LteEnbPhy::DoSendMacPdu (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this);
  //std::cout<< Simulator::Now().GetSeconds()<<" LteEnbPhy::DoSendMacPdu Packet_size: "<<p->GetSize()<<std::endl;
  SetMacPdu (p);
}

uint8_t
LteEnbPhy::DoGetMacChTtiDelay ()
{
  return (m_macChTtiDelay);
}


void
LteEnbPhy::PhyPduReceived (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this);
  m_enbPhySapUser->ReceivePhyPdu (p);
}

void
LteEnbPhy::SetDownlinkSubChannels (std::vector<int> mask)
{
  NS_LOG_FUNCTION (this);
  m_listOfDownlinkSubchannel = mask;
  Ptr<SpectrumValue> txPsd = CreateTxPowerSpectralDensity ();
  m_downlinkSpectrumPhy->SetTxPowerSpectralDensity (txPsd);
}

void
LteEnbPhy::SetDownlinkSubChannelsWithPowerAllocation (std::vector<int> mask)
{
  NS_LOG_FUNCTION (this);
  m_listOfDownlinkSubchannel = mask;
  Ptr<SpectrumValue> txPsd = CreateTxPowerSpectralDensityWithPowerAllocation ();
  m_downlinkSpectrumPhy->SetTxPowerSpectralDensity (txPsd);
}

std::vector<int>
LteEnbPhy::GetDownlinkSubChannels (void)
{
  NS_LOG_FUNCTION (this);
  return m_listOfDownlinkSubchannel;
}

void
LteEnbPhy::GeneratePowerAllocationMap (uint16_t rnti, int rbId)
{
  //NS_LOG_FUNCTION (this);
  NS_LOG_FUNCTION (this << rnti);
  double rbgTxPower = m_txPower;

  std::map<uint16_t, double>::iterator it = m_paMap.find (rnti);
  if (it != m_paMap.end ())
    {
      rbgTxPower = m_txPower + it->second;
    }

  m_dlPowerAllocationMap.insert (std::pair<int, double> (rbId, rbgTxPower));
}

Ptr<SpectrumValue>
LteEnbPhy::CreateTxPowerSpectralDensity ()
{
  NS_LOG_FUNCTION (this);

  Ptr<SpectrumValue> psd = LteSpectrumValueHelper::CreateTxPowerSpectralDensity (m_dlEarfcn, m_dlBandwidth, m_txPower, GetDownlinkSubChannels ());

  return psd;
}

Ptr<SpectrumValue>
LteEnbPhy::CreateTxPowerSpectralDensityWithPowerAllocation ()
{
  NS_LOG_FUNCTION (this);

  Ptr<SpectrumValue> psd = LteSpectrumValueHelper::CreateTxPowerSpectralDensity (m_dlEarfcn, m_dlBandwidth, m_txPower, m_dlPowerAllocationMap, GetDownlinkSubChannels ());

  return psd;
}


void
LteEnbPhy::CalcChannelQualityForUe (std::vector <double> sinr, Ptr<LteSpectrumPhy> ue)
{
  NS_LOG_FUNCTION (this);
}


void
LteEnbPhy::DoSendLteControlMessage (Ptr<LteControlMessage> msg)
{
  NS_LOG_FUNCTION (this << msg);
  // queues the message (wait for MAC-PHY delay)
  SetControlMessages (msg);
}



void
LteEnbPhy::ReceiveLteControlMessage (Ptr<LteControlMessage> msg)
{
  NS_FATAL_ERROR ("Obsolete function");
  NS_LOG_FUNCTION (this << msg);
  m_enbPhySapUser->ReceiveLteControlMessage (msg);
}

void
LteEnbPhy::ReceiveLteControlMessageList (std::list<Ptr<LteControlMessage> > msgList)
{
  NS_LOG_FUNCTION (this);
  std::list<Ptr<LteControlMessage> >::iterator it;
  for (it = msgList.begin (); it != msgList.end (); it++)
    {
      switch ((*it)->GetMessageType ())
        {
        case LteControlMessage::RACH_PREAMBLE:
          {
            Ptr<RachPreambleLteControlMessage> rachPreamble = DynamicCast<RachPreambleLteControlMessage> (*it);
            m_enbPhySapUser->ReceiveRachPreamble (rachPreamble->GetRapId ());
          }
          break;
        case LteControlMessage::DL_CQI:
          {
            Ptr<DlCqiLteControlMessage> dlcqiMsg = DynamicCast<DlCqiLteControlMessage> (*it);
            CqiListElement_s dlcqi = dlcqiMsg->GetDlCqi ();
            // check whether the UE is connected
            if (m_ueAttached.find (dlcqi.m_rnti) != m_ueAttached.end ())
              {
                m_enbPhySapUser->ReceiveLteControlMessage (*it);
              }
          }
          break;
        case LteControlMessage::BSR:
          {
            Ptr<BsrLteControlMessage> bsrMsg = DynamicCast<BsrLteControlMessage> (*it);
            MacCeListElement_s bsr = bsrMsg->GetBsr ();
            // check whether the UE is connected
            if (m_ueAttached.find (bsr.m_rnti) != m_ueAttached.end ())
              {
                m_enbPhySapUser->ReceiveLteControlMessage (*it);
              }
          }
          break;
        case LteControlMessage::DL_HARQ:
          {
            Ptr<DlHarqFeedbackLteControlMessage> dlharqMsg = DynamicCast<DlHarqFeedbackLteControlMessage> (*it);
            DlInfoListElement_s dlharq = dlharqMsg->GetDlHarqFeedback ();
            // check whether the UE is connected
            if (m_ueAttached.find (dlharq.m_rnti) != m_ueAttached.end ())
              {
                m_enbPhySapUser->ReceiveLteControlMessage (*it);
              }
          }
          break;
        default:
          NS_FATAL_ERROR ("Unexpected LteControlMessage type");
          break;
        }
    }
}

void
LteEnbPhy::CalculatePagingInfo (void)
{
    m_T   = 128;
    m_nb  = 1*m_T;
    uint16_t ue_id = 1;//m_imsi%1024;
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

    //m_pf =  RHS;
    /*if (m_requirepagingflag)
    {
        m_pf = (m_pf) + m_T;
    }*/
    m_pf = 3;

    NS_LOG_FUNCTION (this << "pf" << m_pf <<"po" << m_po);


}

void
LteEnbPhy::StartFrame (void)
{
  NS_LOG_FUNCTION (this);

  ++m_nrFrames;
  NS_LOG_INFO ("-----frame " << m_nrFrames << "-----");
  m_nrSubFrames = 0;

  //CalculatePagingInfo();
  if(m_EnabledNbIot==false) // Legacy LTE condition
    {
      // send MIB at beginning of every frame
      m_mib.systemFrameNumber = m_nrSubFrames;
      Ptr<MibLteControlMessage> mibMsg = Create<MibLteControlMessage> ();
      mibMsg->SetMib (m_mib);
      m_controlMessagesQueue.at (0).push_back (mibMsg);

    }
  else  // NB-IoT condition (enabled)
    {
      // send MIB-NB message part or a new one if it is the correct turn in the scheduling.

          NS_LOG_FUNCTION (this << "NB-IoT");
      // NOTE: The Subframes number is 0, so we have to control the Frame number only.
      if((m_nrFrames % 64) == 0)
        {
          // Send a new MIB-NB message

          // This is first part of the new MIB-NB message, so we have to initialize the counter again.
          m_mibNbPartCounter = 0;


          // Initialize the generator of the schedulingInfoSib1Generator.
          m_schedulingInfoSib1NbGenerator++;
          m_schedulingInfoSib1NbGenerator = (m_schedulingInfoSib1NbGenerator) % 12;


          /*
           * This is a simplification of the information present in the Most Significant Byte
           * of the System Frame Number. A single MIB-NB is divided into 8 parts. Each part has
           * to be sent at the beginning of every subframe. A MIB-NB message is transmitted 8 times
           * before to change his value.
           */
          m_mibNb.systemFrameNumberMsb = m_mibNbPartCounter;


        }
      else
        {
          /*
           * We must set properly the content of the subframe number zero on a radio frame that is
           * under the MIB-NB period (SFN mod 64 != 0).
           */

          //Increment the counter of MIB-NB part.
          m_mibNbPartCounter++;

          if(m_mibNbPartCounter == 8)
            {
              //We must repeat the last MIB-NB message sent.
              m_mibNbPartCounter = 0;

              // This is the only thing we need to change.
            }

          m_mibNb.systemFrameNumberMsb = m_mibNbPartCounter;


        }

      // We suppose the UE will not wait for a System Information Block Type 14. So it is not needed to enable AB.
      m_mibNb.abEnabled = false;


     /*
      * We suppose to use Inband Same PCI mode, cause it is equal to legacy LTE.
      *
      * \todo
      * Till now, it is not present a method to gave some information about the
      * CRS (Cell Specific Reference Signal).
      * So, we will set up an insignificant value.
      *
      * Anyway, in future, if CRS will be passed to this module it will be used.
      * Otherwise, we could substitute the OperationMode structure with an enumetation.
      *
      */
     m_mibNb.operationMode.inbandSamePci.eutraCrsSequenceInfo = 1;


     /*
      * We generate the schedulingInfoSib1 in a controlled way from 0 to 11.
      * Values from 12 to 15 are reserved.
      *
      * In future this assignment will vary.
      *
      * To get more information about this parameter see the document 36.213, Table 16.4.1.3-3.
      */
      m_mibNb.schedulingInfoSib1 = (m_schedulingInfoSib1NbGenerator + m_mibNbPartCounter) % 12;


      /*
       * Number of the radio frame. This is a simplification cause we have to load only the 2
       * LSB of the system frame number.
       */
      m_mibNb.hyperSfnLsb = m_nrFrames;


      Ptr<MibNbLteControlMessage> mibMsg = Create<MibNbLteControlMessage>();

      mibMsg->SetMibNb(m_mibNb);

      m_controlMessagesQueue.at (0).push_back (mibMsg);

      if((m_nrFrames % 256) == 0) // Set a new SIB1-NB period
        {
          m_sib1NbPeriod=true;

          switch(m_mibNb.schedulingInfoSib1)
            {
              case 0:
              case 3:
              case 6:
              case 9:
                m_sib1NbRepetitions = 4;
                break;

              case 1:
              case 4:
              case 7:
              case 10:
                m_sib1NbRepetitions = 8;
                break;

              default:
                m_sib1NbRepetitions = 16;
                break;
            }
        }

    }

  // Once MIB or MIB-NB are enqueued we can start the composition of an another subframe.
  StartSubFrame ();
}

// Looks like a work around function
void LteEnbPhy::DoUpdatePagingParam(void)
{
    NS_LOG_FUNCTION (this<<m_nrFrames<<m_nrSubFrames);
    if (m_nrSubFrames >=0 && m_nrSubFrames+1<=10)
     {
          m_pf = m_nrFrames;
          (m_po) = m_nrSubFrames + 1;
     }
    else if(m_nrSubFrames+1>10)
     {
          m_pf = m_nrFrames+1;
          (m_po) = 1;
     }
}
void
LteEnbPhy::StartPaging (void)
{
    //std::cout<<m_pf<<"="<<m_nrFrames<<" "<<m_po<<"="<<m_nrSubFrames<<std::endl;
    if(m_pf == m_nrFrames && (m_po) == m_nrSubFrames)
    {
        NS_LOG_FUNCTION (this << m_pf<<m_nrSubFrames);
        Ptr<PagingLteControlMessage> msg = Create<PagingLteControlMessage> ();
        /* TODO: Set paging values */
        //msg->m_count = ;

        msg->SetPaging (m_paging);
        m_controlMessagesQueue.at (0).push_back (msg);
    }
}

void
LteEnbPhy::StartSubFrame (void)
{
  NS_LOG_FUNCTION (this);

  ++m_nrSubFrames;

  int temp_nrSubFrames = m_nrSubFrames;

  StartPaging();

  if(m_EnabledNbIot==false) //Legacy LTE condition
    {
      /*
       * Send SIB1 at 6th subframe of every odd-numbered radio frame. This is
       * equivalent with Section 5.2.1.2 of 3GPP TS 36.331, where it is specified
       * "repetitions are scheduled in subframe #5 of all other radio frames for
       * which SFN mod 2 = 0," except that 3GPP counts frames and subframes starting
       * from 0, while ns-3 counts starting from 1.
       */
      if ((m_nrSubFrames == 6) && ((m_nrFrames % 2) == 1))
        {
          Ptr<Sib1LteControlMessage> msg = Create<Sib1LteControlMessage> ();
          msg->SetSib1 (m_sib1);
          m_controlMessagesQueue.at (0).push_back (msg);
        }
    }
  else  //NB-IoT enabled
    {
      /*
       * Send SIB1-NB if needed.
       * SIB1-NB is transmitted over the NPDSCH. Its has a period of 256 radio frames and is
       * repeated 4, 8 or 16 times. The transport block size and the number of repetitions is
       * indicated in the MIB-NB. 4, 8 or 16 repetitions are possible, and 4 transport block sizes
       * of 208, 328, 440 and 680 bits are defined, according to the  systemInformationBlockType1
       * sent by the MIB-NB. The radio frame on which the SIB1-NB starts is determined by the
       * number of repetitions and the NCellID.
       * The SIB1-NB is transmitted in subframe #4 of every other frame in max 16 continuous frames.
       */
      if((m_nrSubFrames == 4)&&(m_sib1NbPeriod==true)&&(m_sib1NbRepetitions>0))
        {

          m_sib1NbRepetitions--;

          /*
           * This is a simplification of the effective implementation.
           * Because we have to send only a part of the System Radio Frame Number.
           * To get this value constant under the repetition we used the sum of
           * the Radio Frame Number and the remaining number of SIB1-NB repetitions.
           */
          m_sib1Nb.hyperSfnMsbR13 = m_nrFrames + m_sib1NbRepetitions;



          //Send SIB1-NB.
          Ptr<Sib1NbLteControlMessage> msg = Create<Sib1NbLteControlMessage> ();
          msg->SetSib1Nb(m_sib1Nb);
          m_controlMessagesQueue.at (0).push_back (msg);

        }

      //Set the end of the SIB1-NB period
      if(m_sib1NbRepetitions == 0)
        {
          m_sib1NbPeriod = false;
        }

    }

  if (m_srsPeriodicity>0)
    { 
      // might be 0 in case the eNB has no UEs attached
      NS_ASSERT_MSG (m_nrFrames > 1, "the SRS index check code assumes that frameNo starts at 1");
      NS_ASSERT_MSG (m_nrSubFrames > 0 && m_nrSubFrames <= 10, "the SRS index check code assumes that subframeNo starts at 1");
      m_currentSrsOffset = (((m_nrFrames-1)*10 + (m_nrSubFrames-1)) % m_srsPeriodicity);
    }
  NS_LOG_INFO ("-----sub frame " << m_nrSubFrames << "-----");
  m_harqPhyModule->SubframeIndication (m_nrFrames, m_nrSubFrames);

  //std::set <uint16_t> m_ueAttached;
  //for (std::set <uint16_t>::iterator it = m_ueAttached.begin(); it != m_ueAttached.end(); ++it)
  //  std::cout << *it << " ";
  //std::cout << "\n";


  // update info on TB to be received
  std::list<UlDciLteControlMessage> uldcilist = DequeueUlDci ();
  std::list<UlDciLteControlMessage>::iterator dciIt = uldcilist.begin ();
  NS_LOG_DEBUG (this << " eNB Expected TBs " << uldcilist.size ());
  for (dciIt = uldcilist.begin (); dciIt!=uldcilist.end (); dciIt++)
    {
      std::set <uint16_t>::iterator it2;
      it2 = m_ueAttached.find ((*dciIt).GetDci ().m_rnti);

      if (it2 == m_ueAttached.end ())
        {
          NS_LOG_ERROR ("UE not attached");
          std::cout << " UE not attached rnti:" <<  (*dciIt).GetDci ().m_rnti << "\n";
        }
      else
        {
          // send info of TB to LteSpectrumPhy 
          // translate to allocation map
          std::vector <int> rbMap;
          int N_rb_ul = m_ulBandwidth;
          int N_prb = (*dciIt).GetDci ().m_riv / N_rb_ul + 1;
          int RB_start = (*dciIt).GetDci ().m_riv % N_rb_ul;
#ifdef DCI_BW_DEBUG
          //std::cout << "[eNB] (AddExpectedTb) m_ulBandwidth " << (int)m_ulBandwidth  << " N_prb:" << (int)N_prb << " m_riv:" << (int)(*dciIt).GetDci ().m_riv << " m_rbStart:" << (int)(*dciIt).GetDci ().m_rbStart << " m_rbLen:" << (int)(*dciIt).GetDci ().m_rbLen << "\n";
#endif
          if(true/*(*dciIt).GetDci ().m_rar != 1*/)
          {
            for (int j = 0; j < N_prb; j++)
            {
              rbMap.push_back (RB_start + j);
            }
          }
          /*else
          {
            for (int i = (*dciIt).GetDci ().m_rbStart; i < (*dciIt).GetDci ().m_rbStart + (*dciIt).GetDci ().m_rbLen; i++)
              {
                rbMap.push_back (i);
              }                                     
          }*/
          m_uplinkSpectrumPhy->AddExpectedTb ((*dciIt).GetDci ().m_rnti, (*dciIt).GetDci ().m_ndi, (*dciIt).GetDci ().m_tbSize, (*dciIt).GetDci ().m_mcs, rbMap, 0 /* always SISO*/, 0 /* no HARQ proc id in UL*/, 0 /*evaluated by LteSpectrumPhy*/, false /* UL*/);
          if ((*dciIt).GetDci ().m_ndi==1)
            {
              NS_LOG_DEBUG (this << " RNTI " << (*dciIt).GetDci ().m_rnti << " NEW TB");
            }
          else
            {
              NS_LOG_DEBUG (this << " RNTI " << (*dciIt).GetDci ().m_rnti << " HARQ RETX");
            }
        }
    }
  // update info on TB to be send
#ifdef DEBUG_CONTROL_DATA_QUEUE
  std::cout << "m_dlDciQueue.size():" << m_dlDciQueue.size() << "\n";
#endif
  
  std::list<Ptr<LteControlMessage> > ctrlMsg = GetControlMessages (temp_nrSubFrames);
  // process the current burst of control messages
  NS_LOG_FUNCTION (this << " LteEnbPhy::process the current burst of control messages==================================");
  if (ctrlMsg.size () > 0)
    {
      std::list<Ptr<LteControlMessage> >::iterator it;
      if(m_dlDciQueue.size()!= 0 && (temp_nrSubFrames == 2 ))
      {
        m_dlDataRbMap.clear ();
        //m_dlCtrlRbMap.clear();
        m_dlPowerAllocationMap.clear ();
      }
      it = ctrlMsg.begin ();
      while (it != ctrlMsg.end ())
        {
          Ptr<LteControlMessage> msg = (*it);
          if (msg->GetMessageType () == LteControlMessage::DL_DCI)
            {
              NS_LOG_FUNCTION (this << " LteEnbPhy::LteControlMessage::DL_DCI==================================");
              Ptr<DlDciLteControlMessage> dci = DynamicCast<DlDciLteControlMessage> (msg);
              QueueDlDci (*dci);
              if(m_dlDciQueue.size()!= 0 && (temp_nrSubFrames == 2 ))
              {
                //m_dlDataRbMap.clear ();
                //m_dlPowerAllocationMap.clear ();
                std::list<DlDciLteControlMessage> dldcilist = DequeueDlDci ();
                std::list<DlDciLteControlMessage>::iterator dldciIt = dldcilist.begin ();
                NS_LOG_DEBUG (this << " eNB Expected TBs " << dldcilist.size ());
                for (dldciIt = dldcilist.begin (); dldciIt!=dldcilist.end (); dldciIt++)
                {
                  // get the tx power spectral density according to DL-DCI(s)
                  // translate the DCI to Spectrum framework
                  uint32_t mask = 0x1;
                  for (int i = 0; i < 32; i++)
                  {
                    if ((((*dldciIt).GetDci ().m_rbBitmap & mask) >> i) == 1)
                      {
                        for (int k = 0; k < GetRbgSize (); k++)
                          {
                            //m_dlDataRbMap.push_back ((i * GetRbgSize ()) + k);
#ifdef DCI_BW_DEBUG
                            NS_LOG_DEBUG(this << " [enb]DL-DCI allocated PRB " << (i*GetRbgSize()) + k);
                            std::cout << " [enb]DL-DCI allocated PRB " << (i*GetRbgSize()) + k << "\n";
#endif
                            GeneratePowerAllocationMap ((*dldciIt).GetDci ().m_rnti, (i * GetRbgSize ()) + k );
                          }
                      }
                    mask = (mask << 1);
                  }
                  //=================================================================================
                  int N_rb_dl = m_dlBandwidth;
#ifdef DCI_BW_DEBUG
                  std::cout << " m_dlBandwidth " << (int)m_dlBandwidth << "\n";
#endif
                  int N_prb = (*dldciIt).GetDci ().m_riv / N_rb_dl + 1;
                  int RB_start = (*dldciIt).GetDci ().m_riv % N_rb_dl;
                  for (int j = 0; j < N_prb; j++)
                  {
#ifdef DCI_BW_DEBUG 
                    std::cout << " m_dlDataRbMap.size() " << m_dlDataRbMap.size() << "\n";
#endif
                    m_dlDataRbMap.push_back (RB_start + j);
#ifdef DCI_BW_DEBUG
                    std::cout << " m_dlDataRbMap.size() " << m_dlDataRbMap.size() << "\n";
#endif
                  }
#ifdef DCI_BW_DEBUG
                  NS_LOG_DEBUG(this << " RNTI " << (*dldciIt).GetDci ().m_rnti << " [enb]DL-DCI allocated PRB " << (*dldciIt).GetDci ().m_riv % 100 << " m_repetitionNumber: " << (*dldciIt).GetDci ().m_repetitionNumber);
                  std::cout << " RNTI " << (*dldciIt).GetDci ().m_rnti << " [enb]DL-DCI allocated PRB " << (*dldciIt).GetDci ().m_riv % 100 << " m_repetitionNumber: " << (*dldciIt).GetDci ().m_repetitionNumber << "\n";
                  //m_dlDataRbMap.push_back ((*dldciIt)->GetDci ().m_riv % 100);
                  NS_LOG_DEBUG(this << " RB_start " << RB_start << " N_prb:" << N_prb);
                  std::cout << " RB_start " << RB_start << " N_prb:" << N_prb << "\n";
#endif
                  //=================================================================================

                  // fire trace of DL Tx PHY stats
                  for (uint8_t i = 0; i < (*dldciIt).GetDci ().m_mcs.size (); i++)
                    {
                      PhyTransmissionStatParameters params;
                      params.m_cellId = m_cellId;
                      params.m_imsi = 0; // it will be set by DlPhyTransmissionCallback in LteHelper
                      params.m_timestamp = Simulator::Now ().GetMilliSeconds () + DL_PDSCH_TTIS_DELAY;
                      params.m_rnti = (*dldciIt).GetDci ().m_rnti;
                      params.m_txMode = 0; // TBD
                      params.m_layer = i;
                      params.m_mcs = (*dldciIt).GetDci ().m_mcs.at (i);
                      params.m_size = (*dldciIt).GetDci ().m_tbsSize.at (i);
                      params.m_rv = (*dldciIt).GetDci ().m_rv.at (i);
                      params.m_ndi = (*dldciIt).GetDci ().m_ndi.at (i);
                      params.m_ccId = m_componentCarrierId;
                      m_dlPhyTransmission (params);
                    }
                }
              }
            }
          else if (msg->GetMessageType () == LteControlMessage::UL_DCI)
            {
              NS_LOG_FUNCTION (this << " LteEnbPhy::LteControlMessage::UL_DCI==================================(QueueUlDci)");
              //std::cout <<  this << " LteEnbPhy::LteControlMessage::UL_DCI==================================(QueueUlDci)" << "\n";
              Ptr<UlDciLteControlMessage> uldciMsg = DynamicCast<UlDciLteControlMessage> (msg);
              QueueUlDci (*uldciMsg);


              std::set <uint16_t>::iterator it2;
              it2 = m_ueAttached.find ((*uldciMsg).GetDci ().m_rnti);

              if (it2 == m_ueAttached.end ())
                {
                  NS_LOG_ERROR ("UE not attached");
                  std::cout << " UE not attached rnti:" <<  (*uldciMsg).GetDci ().m_rnti << "\n";
                }
              else
                {
                  // send info of TB to LteSpectrumPhy 
                  // translate to allocation map
                  std::vector <int> rbMap;
                  int N_rb_ul = m_ulBandwidth;
                  int N_prb = (*uldciMsg).GetDci ().m_riv / N_rb_ul + 1;
                  int RB_start = (*uldciMsg).GetDci ().m_riv % N_rb_ul;
                  //std::cout << "[eNB] (UL_DCI) m_ulBandwidth " << (int)m_ulBandwidth  << " N_prb:" << (int)N_prb << " m_riv:" << (int)(*uldciMsg).GetDci ().m_riv << " m_rbStart:" << (int)(*uldciMsg).GetDci ().m_rbStart << " m_rbLen:" << (int)(*dciIt).GetDci ().m_rbLen << "\n";
                  for (int j = 0; j < N_prb; j++)
                    {
                      rbMap.push_back (RB_start + j);
                      //m_dlCtrlRbMap.push_back (RB_start + j);
                    }
                }
                
            }
          else if (msg->GetMessageType () == LteControlMessage::RAR)
            {
              Ptr<RarLteControlMessage> rarMsg = DynamicCast<RarLteControlMessage> (msg);
              for (std::list<RarLteControlMessage::Rar>::const_iterator it = rarMsg->RarListBegin (); it != rarMsg->RarListEnd (); ++it)
                {
                  if (it->rarPayload.m_grant.m_ulDelay == true)
                    {
                      NS_FATAL_ERROR (" RAR delay is not yet implemented");
                    }
                  UlGrant_s ulGrant = it->rarPayload.m_grant;
                  // translate the UL grant in a standard UL-DCI and queue it
                  UlDciListElement_s dci;
                  dci.m_rnti = ulGrant.m_rnti;
                  dci.m_rbStart = ulGrant.m_rbStart;
                  //=============================================
#ifdef DCI_BW_DEBUG

                  int N_rb_ul = m_ulBandwidth;
                  int N_prb = ulGrant.m_riv / N_rb_ul + 1;
                  int RB_start = ulGrant.m_riv % N_rb_ul;
                  //std::cout << "[eNB] (ulGrant) m_ulBandwidth " << (int)m_ulBandwidth  << " N_prb:" << (int)N_prb << " m_riv:" << (int)ulGrant.m_riv << " m_rbStart:" << (int)ulGrant.m_rbStart << " m_rbLen:" << (int)ulGrant.m_rbLen << " RB_start:" << RB_start << "\n";
#endif
                  //=============================================
                  dci.m_rbLen = ulGrant.m_rbLen;
                  dci.m_tbSize = ulGrant.m_tbSize;
                  dci.m_mcs = ulGrant.m_mcs;
                  dci.m_hopping = ulGrant.m_hopping;
                  dci.m_tpc = ulGrant.m_tpc;
                  dci.m_cqiRequest = ulGrant.m_cqiRequest;
                  dci.m_ndi = 1;
                  dci.m_riv = ulGrant.m_riv;
                  dci.m_rar = 1;
                  dci.m_repetitionNumber = ulGrant.m_repetitionNumber;
                  UlDciLteControlMessage msg;
                  msg.SetDci (dci);
                  QueueUlDci (msg);
                }
            }
          it++;
        }
    }

  SendControlChannels (ctrlMsg);

  // trigger the MAC
  NS_LOG_FUNCTION (this << " LteEnbPhy::trigger the MAC==================================");
  m_enbPhySapUser->SubframeIndication (m_nrFrames, m_nrSubFrames);

  
  // send data frame
  //if(m_nrSubFrames % 2 != 0)

  //if(m_nrSubFrames % 2 != 0)
  if(temp_nrSubFrames == 7)
  {
    NS_LOG_FUNCTION (this << "-----sub frame (SEND DATA): " << m_nrSubFrames << "----------------------------------------------------------------------------------------------------------------------------------");
    Ptr<PacketBurst> pb = GetPacketBurst ();
    if (pb)
      {
        NS_LOG_FUNCTION (this << " LteEnbPhy::SendDataChannels==================================");
        Simulator::Schedule (DL_CTRL_DELAY_FROM_SUBFRAME_START, // ctrl frame fixed to 3 symbols
                             &LteEnbPhy::SendDataChannels,
                             this,pb);
      }
  }
  else
  {
    NS_LOG_FUNCTION (this << "-----sub frame (DONOT SEND DATA): " << temp_nrSubFrames << "----------------------------------------------------------------------------------------------------------------------------------");
  }


  NS_LOG_FUNCTION (this << " LteEnbPhy::Schedule EndSubFrame==================================");
  Simulator::Schedule (Seconds (GetTti ()),
                       &LteEnbPhy::EndSubFrame,
                       this);

}

void
LteEnbPhy::SendControlChannels (std::list<Ptr<LteControlMessage> > ctrlMsgList)
{
  NS_LOG_FUNCTION (this << " eNB " << m_cellId << " start tx ctrl frame");
  // set the current tx power spectral density (full bandwidth)
  std::vector <int> dlRb;
  for (uint8_t i = 0; i < m_dlBandwidth; i++)
    {
      dlRb.push_back (i);
    }
  SetDownlinkSubChannels (dlRb);
  NS_LOG_LOGIC (this << " eNB start TX CTRL");
  bool pss = false;
  if ((m_nrSubFrames == 1) || (m_nrSubFrames == 6))
    {
      pss = true;
    }
  m_downlinkSpectrumPhy->StartTxDlCtrlFrame (ctrlMsgList, pss);

}

void
LteEnbPhy::SendDataChannels (Ptr<PacketBurst> pb)
{
  NS_LOG_FUNCTION (this << " m_dlDataRbMap.size(): " << m_dlDataRbMap.size());
  // set the current tx power spectral density
  SetDownlinkSubChannelsWithPowerAllocation (m_dlDataRbMap);
  // send the current burts of packets
  NS_LOG_LOGIC (this << " eNB start TX DATA");
  std::list<Ptr<LteControlMessage> > ctrlMsgList;
  ctrlMsgList.clear ();
  m_downlinkSpectrumPhy->StartTxDataFrame (pb, ctrlMsgList, DL_DATA_DURATION);
}


void
LteEnbPhy::EndSubFrame (void)
{
  NS_LOG_FUNCTION (this << Simulator::Now ().GetSeconds ());
  if (m_nrSubFrames == 10)
    {
      Simulator::ScheduleNow (&LteEnbPhy::EndFrame, this);
    }
  else
    {
      Simulator::ScheduleNow (&LteEnbPhy::StartSubFrame, this);
    }
}


void
LteEnbPhy::EndFrame (void)
{
  NS_LOG_FUNCTION (this << Simulator::Now ().GetSeconds ());
  Simulator::ScheduleNow (&LteEnbPhy::StartFrame, this);
}


void 
LteEnbPhy::GenerateCtrlCqiReport (const SpectrumValue& sinr)
{
  NS_LOG_FUNCTION (this << sinr << Simulator::Now () << m_srsStartTime);
  //std::cout << this<< " sinr:" << sinr << " " << Simulator::Now () << " " << m_srsStartTime << "\n";
  // avoid processing SRSs sent with an old SRS configuration index
  if (Simulator::Now () > m_srsStartTime)
    {
      FfMacSchedSapProvider::SchedUlCqiInfoReqParameters ulcqi = CreateSrsCqiReport (sinr);
      m_enbPhySapUser->UlCqiReport (ulcqi);
    }
}

void
LteEnbPhy::GenerateDataCqiReport (const SpectrumValue& sinr)
{
  NS_LOG_FUNCTION (this << sinr);
  FfMacSchedSapProvider::SchedUlCqiInfoReqParameters ulcqi = CreatePuschCqiReport (sinr);
  m_enbPhySapUser->UlCqiReport (ulcqi);
}

void
LteEnbPhy::ReportInterference (const SpectrumValue& interf)
{
  NS_LOG_FUNCTION (this << interf);
  Ptr<SpectrumValue> interfCopy = Create<SpectrumValue> (interf);
  m_interferenceSampleCounter++;
  if (m_interferenceSampleCounter == m_interferenceSamplePeriod)
    {
      m_reportInterferenceTrace (m_cellId, interfCopy);
      m_interferenceSampleCounter = 0;
    }
}

void
LteEnbPhy::ReportRsReceivedPower (const SpectrumValue& power)
{
  // not used by eNB
}



FfMacSchedSapProvider::SchedUlCqiInfoReqParameters
LteEnbPhy::CreatePuschCqiReport (const SpectrumValue& sinr)
{
  NS_LOG_FUNCTION (this << sinr);
  Values::const_iterator it;
  FfMacSchedSapProvider::SchedUlCqiInfoReqParameters ulcqi;
  ulcqi.m_ulCqi.m_type = UlCqi_s::PUSCH;
  int i = 0;
  for (it = sinr.ConstValuesBegin (); it != sinr.ConstValuesEnd (); it++)
    {
      double sinrdb = 10 * std::log10 ((*it));
//       NS_LOG_DEBUG ("ULCQI RB " << i << " value " << sinrdb);
      // convert from double to fixed point notation Sxxxxxxxxxxx.xxx
      int16_t sinrFp = LteFfConverter::double2fpS11dot3 (sinrdb);
      ulcqi.m_ulCqi.m_sinr.push_back (sinrFp);
      i++;
    }
  return (ulcqi);
	
}


void
LteEnbPhy::DoSetBandwidth (uint8_t ulBandwidth, uint8_t dlBandwidth)
{
  NS_LOG_FUNCTION (this << (uint32_t) ulBandwidth << (uint32_t) dlBandwidth);
  m_ulBandwidth = ulBandwidth;
  m_dlBandwidth = dlBandwidth;

  static const int Type0AllocationRbg[4] = {
    10,     // RGB size 1
    26,     // RGB size 2
    63,     // RGB size 3
    110     // RGB size 4
  };  // see table 7.1.6.1-1 of 36.213
  for (int i = 0; i < 4; i++)
    {
      if (dlBandwidth < Type0AllocationRbg[i])
        {
          m_rbgSize = i + 1;
          break;
        }
    }
  m_rbgSize = 1;
}

void 
LteEnbPhy::DoSetEarfcn (uint32_t ulEarfcn, uint32_t dlEarfcn)
{
  NS_LOG_FUNCTION (this << ulEarfcn << dlEarfcn);
  m_ulEarfcn = ulEarfcn;
  m_dlEarfcn = dlEarfcn;
}


void 
LteEnbPhy::DoAddUe (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << rnti);
 
  bool success = AddUePhy (rnti);
  NS_ASSERT_MSG (success, "AddUePhy() failed");

  // add default P_A value
  DoSetPa (rnti, 0);
}

void 
LteEnbPhy::DoRemoveUe (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << rnti);
 
  bool success = DeleteUePhy (rnti);
  NS_ASSERT_MSG (success, "DeleteUePhy() failed");

  // remove also P_A value
  std::map<uint16_t, double>::iterator it = m_paMap.find (rnti);
  if (it != m_paMap.end ())
    {
      m_paMap.erase (it);
    }

}

void
LteEnbPhy::DoSetPa (uint16_t rnti, double pa)
{
  NS_LOG_FUNCTION (this << rnti);

  std::map<uint16_t, double>::iterator it = m_paMap.find (rnti);

  if (it == m_paMap.end ())
    {
      m_paMap.insert (std::pair<uint16_t, double> (rnti, pa));
    }
  else
    {
      it->second = pa;
    }

}

FfMacSchedSapProvider::SchedUlCqiInfoReqParameters
LteEnbPhy::CreateSrsCqiReport (const SpectrumValue& sinr)
{
  NS_LOG_FUNCTION (this << sinr);
  Values::const_iterator it;
  FfMacSchedSapProvider::SchedUlCqiInfoReqParameters ulcqi;
  ulcqi.m_ulCqi.m_type = UlCqi_s::SRS;
  int i = 0;
  double srsSum = 0.0;
  for (it = sinr.ConstValuesBegin (); it != sinr.ConstValuesEnd (); it++)
    {
      double sinrdb = 10 * log10 ((*it));
      //std::cout << "ULCQI RB " << i << " value " << sinrdb << "\n";
      // convert from double to fixed point notation Sxxxxxxxxxxx.xxx
      int16_t sinrFp = LteFfConverter::double2fpS11dot3 (sinrdb);
      srsSum += (*it);
      ulcqi.m_ulCqi.m_sinr.push_back (sinrFp);
      i++;
    }
  // Insert the user generated the srs as a vendor specific parameter
  NS_LOG_DEBUG (this << " ENB RX UL-CQI of " << m_srsUeOffset.at (m_currentSrsOffset));
  VendorSpecificListElement_s vsp;
  vsp.m_type = SRS_CQI_RNTI_VSP;
  vsp.m_length = sizeof(SrsCqiRntiVsp);
  Ptr<SrsCqiRntiVsp> rnti  = Create <SrsCqiRntiVsp> (m_srsUeOffset.at (m_currentSrsOffset));
  vsp.m_value = rnti;
  ulcqi.m_vendorSpecificList.push_back (vsp);
  // call SRS tracing method
  CreateSrsReport (m_srsUeOffset.at (m_currentSrsOffset),
                   (i > 0) ? (srsSum / i) : DBL_MAX);
  return (ulcqi);

}


void
LteEnbPhy::CreateSrsReport (uint16_t rnti, double srs)
{
  NS_LOG_FUNCTION (this << rnti << srs);
  std::map <uint16_t,uint16_t>::iterator it = m_srsSampleCounterMap.find (rnti);
  if (it==m_srsSampleCounterMap.end ())
    {
      // create new entry
      m_srsSampleCounterMap.insert (std::pair <uint16_t,uint16_t> (rnti, 0));
      it = m_srsSampleCounterMap.find (rnti);
    }
  (*it).second++;
  if ((*it).second == m_srsSamplePeriod)
    {
      m_reportUeSinr (m_cellId, rnti, srs, (uint16_t) m_componentCarrierId);
      (*it).second = 0;
    }
}

void
LteEnbPhy::DoSetTransmissionMode (uint16_t  rnti, uint8_t txMode)
{
  NS_LOG_FUNCTION (this << rnti << (uint16_t)txMode);
  // UL supports only SISO MODE
}

void
LteEnbPhy::QueueUlDci (UlDciLteControlMessage m)
{
  NS_LOG_FUNCTION (this);
  m_ulDciQueue.at (UL_PUSCH_TTIS_DELAY - 1).push_back (m);
}

std::list<UlDciLteControlMessage>
LteEnbPhy::DequeueUlDci (void)
{
  NS_LOG_FUNCTION (this);
#ifdef DEBUG_CONTROL_DATA_QUEUE
  std::cout << "m_ulDciQueue.size():" << m_ulDciQueue.size() << "\n";
#endif
  if (m_ulDciQueue.at (0).size ()>0)
    {
      std::list<UlDciLteControlMessage> ret = m_ulDciQueue.at (0);
      m_ulDciQueue.erase (m_ulDciQueue.begin ());
      std::list<UlDciLteControlMessage> l;
      m_ulDciQueue.push_back (l);
      return (ret);
    }
  else
    {
      m_ulDciQueue.erase (m_ulDciQueue.begin ());
      std::list<UlDciLteControlMessage> l;
      m_ulDciQueue.push_back (l);
      std::list<UlDciLteControlMessage> emptylist;
      return (emptylist);
    }
}


void
LteEnbPhy::QueueDlDci (DlDciLteControlMessage m)
{
  NS_LOG_FUNCTION (this);
#ifdef DEBUG_CONTROL_DATA_QUEUE
  std::cout << "m_dlDciQueue.size():" << m_dlDciQueue.size() << " m_dlDciQueue.at(0).size(): " << m_dlDciQueue.at (0).size() << "\n";
#endif  
  m_dlDciQueue.at (DL_PDSCH_TTIS_DELAY - 1).push_back (m);
#ifdef DEBUG_CONTROL_DATA_QUEUE
  std::cout << "m_dlDciQueue.size():" << m_dlDciQueue.size() << " m_dlDciQueue.at(0).size(): " << m_dlDciQueue.at (0).size() << "\n\n";
#endif  
}

std::list<DlDciLteControlMessage>
LteEnbPhy::DequeueDlDci (void)
{
  NS_LOG_FUNCTION (this);
#ifdef DEBUG_CONTROL_DATA_QUEUE
  std::cout << "m_dlDciQueue.size():" << m_dlDciQueue.size() << " m_dlDciQueue.at(0).size(): " << m_dlDciQueue.at (0).size() << "\n\n";
#endif  
  if (m_dlDciQueue.at (0).size ()>0)
    {
      std::list<DlDciLteControlMessage> ret = m_dlDciQueue.at (0);
      m_dlDciQueue.erase (m_dlDciQueue.begin ());
      std::list<DlDciLteControlMessage> l;
      m_dlDciQueue.push_back (l);
      return (ret);
    }
  else
    {
      m_dlDciQueue.erase (m_dlDciQueue.begin ());
      std::list<DlDciLteControlMessage> l;
      m_dlDciQueue.push_back (l);
      std::list<DlDciLteControlMessage> emptylist;
      return (emptylist);
    }
}


void
LteEnbPhy::DoSetSrsConfigurationIndex (uint16_t  rnti, uint16_t srcCi)
{
  NS_LOG_FUNCTION (this);
  uint16_t p = GetSrsPeriodicity (srcCi);
  if (p!=m_srsPeriodicity)
    {
      // resize the array of offset -> re-initialize variables
      m_srsUeOffset.clear ();
      m_srsUeOffset.resize (p, 0);
      m_srsPeriodicity = p;
      // inhibit SRS until RRC Connection Reconfiguration propagates
      // to UEs, otherwise we might be wrong in determining the UE who
      // actually sent the SRS (if the UE was using a stale SRS config)
      // if we use a static SRS configuration index, we can have a 0ms guard time
      m_srsStartTime = Simulator::Now () + MilliSeconds (m_macChTtiDelay) + MilliSeconds (0);
    }

  NS_LOG_DEBUG (this << " ENB SRS P " << m_srsPeriodicity << " RNTI " << rnti << " offset " << GetSrsSubframeOffset (srcCi) << " CI " << srcCi);
  std::map <uint16_t,uint16_t>::iterator it = m_srsCounter.find (rnti);
  if (it != m_srsCounter.end ())
    {
      (*it).second = GetSrsSubframeOffset (srcCi) + 1;
    }
  else
    {
      m_srsCounter.insert (std::pair<uint16_t, uint16_t> (rnti, GetSrsSubframeOffset (srcCi) + 1));
    }
  m_srsUeOffset.at (GetSrsSubframeOffset (srcCi)) = rnti;

}


void 
LteEnbPhy::DoSetMasterInformationBlock (LteRrcSap::MasterInformationBlock mib)
{
  NS_LOG_FUNCTION (this);
  m_mib = mib;
}


void
LteEnbPhy::DoSetSystemInformationBlockType1 (LteRrcSap::SystemInformationBlockType1 sib1)
{
  NS_LOG_FUNCTION (this);
  m_sib1 = sib1;
}

void
LteEnbPhy::DoSetMasterInformationBlockNb (NbLteRrcSap::MasterInformationBlockNb mibNb)	// Used by NB-IoT. 3GPP Release 13.
{
  NS_LOG_FUNCTION (this);
  m_mibNb = mibNb;
}

void
LteEnbPhy::DoSetSystemInformationBlockType1Nb (NbLteRrcSap::SystemInformationBlockType1Nb sib1Nb)  // Used by NB-IoT. 3GPP Release 13.
{
  NS_LOG_FUNCTION (this);
  m_sib1Nb = sib1Nb;
}


void
LteEnbPhy::SetHarqPhyModule (Ptr<LteHarqPhy> harq)
{
  m_harqPhyModule = harq;
}


void
LteEnbPhy::ReceiveLteUlHarqFeedback (UlInfoListElement_s mes)
{
  NS_LOG_FUNCTION (this);
  // forward to scheduler
  m_enbPhySapUser->UlInfoListElementHarqFeeback (mes);
}

};
