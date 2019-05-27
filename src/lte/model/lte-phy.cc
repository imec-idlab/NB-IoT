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

#include <ns3/waveform-generator.h>
#include <ns3/object-factory.h>
#include <ns3/log.h>
#include <cmath>
#include <ns3/simulator.h>
#include "ns3/spectrum-error-model.h"
#include "lte-phy.h"
#include "lte-net-device.h"
#include <iostream>
#include <string>
using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LtePhy");

NS_OBJECT_ENSURE_REGISTERED (LtePhy);

//#define DEBUG_CONTROL_DATA_QUEUE


LtePhy::LtePhy ()
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("This constructor should not be called");
}

LtePhy::LtePhy (Ptr<LteSpectrumPhy> dlPhy, Ptr<LteSpectrumPhy> ulPhy)
  : m_downlinkSpectrumPhy (dlPhy),
    m_uplinkSpectrumPhy (ulPhy),
    m_tti (0.001),
    m_ulBandwidth (0),
    m_dlBandwidth (0),
    m_rbgSize (0),
    m_macChTtiDelay (0),
    m_macChRxTtiDelay (0),
    m_cellId (0),
    m_componentCarrierId(0)
{
  NS_LOG_FUNCTION (this);
}


TypeId
LtePhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LtePhy")
    .SetParent<Object> ()
    .SetGroupName("Lte")
  ;
  return tid;
}


LtePhy::~LtePhy ()
{
  NS_LOG_FUNCTION (this);
}

void
LtePhy::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_packetBurstQueue.clear ();
  m_controlMessagesQueue.clear ();
  m_dlControlMessagesQueue.clear();
  m_downlinkSpectrumPhy->Dispose ();
  m_downlinkSpectrumPhy = 0;
  m_uplinkSpectrumPhy->Dispose ();
  m_uplinkSpectrumPhy = 0;
  m_netDevice = 0;
  Object::DoDispose ();
}

void
LtePhy::SetDevice (Ptr<LteNetDevice> d)
{
  NS_LOG_FUNCTION (this << d);
  m_netDevice = d;
}


Ptr<LteNetDevice>
LtePhy::GetDevice () const
{
  NS_LOG_FUNCTION (this);
  return m_netDevice;
}

Ptr<LteSpectrumPhy> 
LtePhy::GetDownlinkSpectrumPhy ()
{
  return m_downlinkSpectrumPhy;
}

Ptr<LteSpectrumPhy> 
LtePhy::GetUplinkSpectrumPhy ()
{
  return m_uplinkSpectrumPhy;
}


void
LtePhy::SetDownlinkChannel (Ptr<SpectrumChannel> c)
{
  NS_LOG_FUNCTION (this << c);
  m_downlinkSpectrumPhy->SetChannel (c);
}

void
LtePhy::SetUplinkChannel (Ptr<SpectrumChannel> c)
{
  NS_LOG_FUNCTION (this << c);
  m_uplinkSpectrumPhy->SetChannel (c);
}

void
LtePhy::SetTti (double tti)
{
  NS_LOG_FUNCTION (this << tti);
  m_tti = tti;
}


double
LtePhy::GetTti (void) const
{
  NS_LOG_FUNCTION (this << m_tti);
  return m_tti;
}


uint16_t
LtePhy::GetSrsPeriodicity (uint16_t srcCi) const
{
  // from 3GPP TS 36.213 table 8.2-1 UE Specific SRS Periodicity
  //uint16_t SrsPeriodicity[9] = {0, 2, 5, 10, 20, 40, 80, 160, 320};
  uint16_t SrsPeriodicity[10] = {0, 2, 5, 10, 20, 40, 80, 160, 320, 640};//, 1280, 2560, 5120, 10240, 20480, 40960, 81920};
  //uint16_t SrsCiLow[9] = {0, 0, 2, 7, 17, 37, 77, 157, 317};
  uint16_t SrsCiLow[10] = {0, 0, 2, 7, 17, 37, 77, 157, 317, 637};//, 1277, 2557, 5117, 10237, 20477, 40957, 81917};
  //uint16_t SrsCiHigh[9] = {0, 1, 6, 16, 36, 76, 156, 316, 636};
  uint16_t SrsCiHigh[10] = {0, 1, 6, 16, 36, 76, 156, 316, 636, 1276};//, 2556, 5116, 10236, 20476, 40956, 81916, 163836};
  uint8_t i;
  for (i = 9; i > 0; i --)
    {
      if ((srcCi>=SrsCiLow[i])&&(srcCi<=SrsCiHigh[i]))
        {
          break;
        }
    }
  return SrsPeriodicity[i];
}

uint16_t
LtePhy::GetSrsSubframeOffset (uint16_t srcCi) const
{
  // from 3GPP TS 36.213 table 8.2-1 UE Specific SRS Periodicity
  /*uint16_t SrsSubframeOffset[9] = {0, 0, 2, 7, 17, 37, 77, 157, 317};
  uint16_t SrsCiLow[9] = {0, 0, 2, 7, 17, 37, 77, 157, 317};
  uint16_t SrsCiHigh[9] = {0, 1, 6, 16, 36, 76, 156, 316, 636};*/

  uint16_t SrsSubframeOffset[10] = {0, 0, 2, 7, 17, 37, 77, 157, 317, 637};
  uint16_t SrsCiLow[10] = {0, 0, 2, 7, 17, 37, 77, 157, 317, 637};
  uint16_t SrsCiHigh[10] = {0, 1, 6, 16, 36, 76, 156, 316, 636, 1276};
  uint8_t i;
  for (i = 9; i > 0; i --)
    {
      if ((srcCi>=SrsCiLow[i])&&(srcCi<=SrsCiHigh[i]))
        {
          break;
        }
    }
  return (srcCi - SrsSubframeOffset[i]);
}

uint8_t
LtePhy::GetRbgSize (void) const
{
  return m_rbgSize;
}

void
LtePhy::SetMacPdu (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this);
#ifdef DEBUG_CONTROL_DATA_QUEUE
  std::cout << "m_packetBurstQueue.size ():" << m_packetBurstQueue.size () << " m_packetBurstQueue.at (m_packetBurstQueue.size () - 1)->GetSize (): "  << m_packetBurstQueue.at (m_packetBurstQueue.size () - 1)->GetSize () << "\n";
#endif  
  m_packetBurstQueue.at (m_packetBurstQueue.size () - 1)->AddPacket (p);
#ifdef DEBUG_CONTROL_DATA_QUEUE
  std::cout << "m_packetBurstQueue.size ():" << m_packetBurstQueue.size () << " m_packetBurstQueue.at (m_packetBurstQueue.size () - 1)->GetSize (): "  << m_packetBurstQueue.at (m_packetBurstQueue.size () - 1)->GetSize () << "\n";
#endif  
}

Ptr<PacketBurst>
LtePhy::GetPacketBurst (void)
{
  NS_LOG_FUNCTION (this);
#ifdef DEBUG_CONTROL_DATA_QUEUE
  std::cout << "m_packetBurstQueue.size ():" << m_packetBurstQueue.size () << " m_packetBurstQueue.at (0)->GetSize (): "  << m_packetBurstQueue.at (0)->GetSize () << "\n";
#endif
  if (m_packetBurstQueue.at (0)->GetSize () > 0)
    {
      Ptr<PacketBurst> ret = m_packetBurstQueue.at (0)->Copy ();
      m_packetBurstQueue.erase (m_packetBurstQueue.begin ());
      m_packetBurstQueue.push_back (CreateObject <PacketBurst> ());
#ifdef DEBUG_CONTROL_DATA_QUEUE
      std::cout << "m_packetBurstQueue.size ():" << m_packetBurstQueue.size () << " m_packetBurstQueue.at (0)->GetSize (): "  << m_packetBurstQueue.at (0)->GetSize () << "\n";
#endif      
      return (ret);
    }
  else
    {
      m_packetBurstQueue.erase (m_packetBurstQueue.begin ());
      m_packetBurstQueue.push_back (CreateObject <PacketBurst> ());
#ifdef DEBUG_CONTROL_DATA_QUEUE
      std::cout << "m_packetBurstQueue.size ():" << m_packetBurstQueue.size () << " m_packetBurstQueue.at (0)->GetSize (): "  << m_packetBurstQueue.at (0)->GetSize () << " return (0);\n";
#endif
      return (0);
    }
}


void
LtePhy::SetControlMessages (Ptr<LteControlMessage> m)
{
  // In uplink the queue of control messages and packet are of different sizes
  // for avoiding TTI cancellation due to synchronization of subframe triggers
  //string messageType = "World";
  string messageType = (m->GetMessageType () == LteControlMessage::DL_DCI)?"DL_DCI":
  (m->GetMessageType () == LteControlMessage::UL_DCI)?"UL_DCI":
  (m->GetMessageType () == LteControlMessage::DL_CQI)?"DL_CQI":
  (m->GetMessageType () == LteControlMessage::UL_CQI)?"UL_CQI":"???";

  NS_LOG_FUNCTION (this << messageType);
#ifdef DEBUG_CONTROL_DATA_QUEUE
  if(m->GetMessageType () == LteControlMessage::UL_DCI)
    std::cout << "LtePhy::messageType:" << messageType << " SetControlMessages:" << m_controlMessagesQueue.size() << " m_controlMessagesQueue.at (0).size ():" << m_controlMessagesQueue.at (0).size () << " m_controlMessagesQueue.at (1).size ():" << m_controlMessagesQueue.at (1).size () << "\n";
  //std::cout << "LtePhy::SetControlMessages:" << m_dlControlMessagesQueue.size() << " m_dlControlMessagesQueue.at (0).size ():" << m_dlControlMessagesQueue.at (0).size () << " m_dlControlMessagesQueue.at (1).size ():" << m_dlControlMessagesQueue.at (1).size () << "\n";
#endif
  /*if(m->GetMessageType () == LteControlMessage::DL_DCI)
  {
    m_dlControlMessagesQueue.at (m_dlControlMessagesQueue.size () - 1).push_back (m);
    std::cout << "m_dlControlMessagesQueue.at (m_dlControlMessagesQueue.size () - 1).push_back (m)\n";
#ifdef DEBUG_CONTROL_DATA_QUEUE
    std::cout << "LtePhy::SetControlMessages:" << m_controlMessagesQueue.size() << " m_controlMessagesQueue.at (0).size ():" << m_controlMessagesQueue.at (0).size () << " m_controlMessagesQueue.at (1).size ():" << m_controlMessagesQueue.at (1).size () << "\n";
    std::cout << "LtePhy::SetControlMessages:" << m_dlControlMessagesQueue.size() << " m_dlControlMessagesQueue.at (0).size ():" << m_dlControlMessagesQueue.at (0).size () << " m_dlControlMessagesQueue.at (1).size ():" << m_dlControlMessagesQueue.at (1).size () << "\n";
#endif
  }
  else*/
  {
    m_controlMessagesQueue.at (m_controlMessagesQueue.size () - 1).push_back (m);
  }
#ifdef DEBUG_CONTROL_DATA_QUEUE
  std::cout << "LtePhy::SetControlMessages:" << m_controlMessagesQueue.size() << " m_controlMessagesQueue.at (0).size ():" << m_controlMessagesQueue.at (0).size () << " m_controlMessagesQueue.at (1).size ():" << m_controlMessagesQueue.at (1).size () << "\n";
  //std::cout << "LtePhy::SetControlMessages:" << m_dlControlMessagesQueue.size() << " m_dlControlMessagesQueue.at (0).size ():" << m_dlControlMessagesQueue.at (0).size () << " m_dlControlMessagesQueue.at (1).size ():" << m_dlControlMessagesQueue.at (1).size () << "\n\n";
#endif
}

std::list<Ptr<LteControlMessage> >
LtePhy::GetControlMessages (uint32_t nrSubFrames)
{
  NS_LOG_FUNCTION (this);
  bool dl_dci_found_at0 = false;
  int other_found = 0;
  bool dl_dci_found_at1 = false;
  std::list<Ptr<LteControlMessage> >::iterator other_it, dl_dci_it;
#ifdef DEBUG_CONTROL_DATA_QUEUE
  std::cout << "LtePhy::GetControlMessages.size():" << m_controlMessagesQueue.size() << " m_controlMessagesQueue.at (0).size (): "  << m_controlMessagesQueue.at (0).size () << " m_controlMessagesQueue.at (1).size (): "  << m_controlMessagesQueue.at (1).size () <<"\n";
  //std::cout << "LtePhy::GetControlMessages.size()::" << m_dlControlMessagesQueue.size() << " m_dlControlMessagesQueue.at (0).size ():" << m_dlControlMessagesQueue.at (0).size () << " m_dlControlMessagesQueue.at (1).size ():" << m_dlControlMessagesQueue.at (1).size () << "\n\n";
#endif 
  std::list<Ptr<LteControlMessage> > ret = m_controlMessagesQueue.at (0);
  if (ret.size () > 0)
    {
      std::list<Ptr<LteControlMessage> >::iterator it;
      it = ret.begin ();
      while (it != ret.end ())
        {
          Ptr<LteControlMessage> msg = (*it);
          string messageType = (msg->GetMessageType () == LteControlMessage::DL_DCI)?"DL_DCI":
          (msg->GetMessageType () == LteControlMessage::UL_DCI)?"UL_DCI":
          (msg->GetMessageType () == LteControlMessage::DL_CQI)?"DL_CQI":
          (msg->GetMessageType () == LteControlMessage::UL_CQI)?"UL_CQI":
          (msg->GetMessageType () == LteControlMessage::BSR)?"BSR":
          (msg->GetMessageType () == LteControlMessage::DL_HARQ)?"DL_HARQ":
          (msg->GetMessageType () == LteControlMessage::RACH_PREAMBLE)?"RACH_PREAMBLE":
          (msg->GetMessageType () == LteControlMessage::RAR)?"RAR":
          (msg->GetMessageType () == LteControlMessage::MIB)?"MIB":
          (msg->GetMessageType () == LteControlMessage::SIB1)?"SIB1":"???";

          NS_LOG_FUNCTION (this << messageType);
          if((msg->GetMessageType () == LteControlMessage::DL_DCI) && (nrSubFrames == 2))
          {
#ifdef DEBUG_CONTROL_DATA_QUEUE
            std::cout << "LteControlMessage::DL_DCI\n";
#endif
            //continue;
          }
          else if((msg->GetMessageType () == LteControlMessage::DL_DCI) && (nrSubFrames != 2))
          {
#ifdef DEBUG_CONTROL_DATA_QUEUE
            std::cout << "LteControlMessage::DL_DCI nrSubFrames != 2\n";
#endif
            dl_dci_it = it;
            dl_dci_found_at0 = true;
          }
          else if((msg->GetMessageType () != LteControlMessage::DL_DCI) && (nrSubFrames != 2))
          {
#ifdef DEBUG_CONTROL_DATA_QUEUE
            std::cout << "LteControlMessage OTHERS nrSubFrames != 2\n";
#endif
            other_it = it;
            other_found++;
          }
          else if (msg->GetMessageType () == LteControlMessage::DL_DCI)
          {
#ifdef DEBUG_CONTROL_DATA_QUEUE
            std::cout << "LteControlMessage ==== DL_DCI\n";
#endif
            NS_ASSERT(0);
          }
          it++;
        }
      if(dl_dci_found_at0 && other_found == 0)
      {
        std::list<Ptr<LteControlMessage> > emptylist;
        return (emptylist);
      }
      else if(dl_dci_found_at0 && other_found > 0)
      {
#ifdef DEBUG_CONTROL_DATA_QUEUE
        std::cout << "LteControlMessage::DL_DCI && OTHER Control messages\n";
#endif
        //continue;
      }
    }
  std::list<Ptr<LteControlMessage> > ret2 = m_controlMessagesQueue.at (1);
  if (ret2.size () > 0)
    {
      std::list<Ptr<LteControlMessage> >::iterator it;
      it = ret2.begin ();
      while (it != ret2.end ())
        {
          Ptr<LteControlMessage> msg = (*it);
          string messageType = (msg->GetMessageType () == LteControlMessage::DL_DCI)?"DL_DCI":
          (msg->GetMessageType () == LteControlMessage::UL_DCI)?"UL_DCI":
          (msg->GetMessageType () == LteControlMessage::DL_CQI)?"DL_CQI":
          (msg->GetMessageType () == LteControlMessage::UL_CQI)?"UL_CQI":
          (msg->GetMessageType () == LteControlMessage::BSR)?"BSR":
          (msg->GetMessageType () == LteControlMessage::DL_HARQ)?"DL_HARQ":
          (msg->GetMessageType () == LteControlMessage::RACH_PREAMBLE)?"RACH_PREAMBLE":
          (msg->GetMessageType () == LteControlMessage::RAR)?"RAR":
          (msg->GetMessageType () == LteControlMessage::MIB)?"MIB":
          (msg->GetMessageType () == LteControlMessage::SIB1)?"SIB1":"???";

          NS_LOG_FUNCTION (this << messageType);
          if((msg->GetMessageType () == LteControlMessage::DL_DCI) && (nrSubFrames != 2))
          {
#ifdef DEBUG_CONTROL_DATA_QUEUE
            std::cout << "LteControlMessage::DL_DCI m_controlMessagesQueue.at (1)\n";
#endif
            dl_dci_found_at1 = true;
          }
          it++;
        }
    }
#ifdef DEBUG_CONTROL_DATA_QUEUE
  std::cout << "LtePhy::GetControlMessages.size():" << m_controlMessagesQueue.size() << " m_controlMessagesQueue.at (0).size (): "  << m_controlMessagesQueue.at (0).size () << " m_controlMessagesQueue.at (1).size ():" << m_controlMessagesQueue.at (1).size () << "\n";
  //std::cout << "LtePhy::GetControlMessages.size()::" << m_dlControlMessagesQueue.size() << " m_dlControlMessagesQueue.at (0).size ():" << m_dlControlMessagesQueue.at (0).size () << " m_dlControlMessagesQueue.at (1).size ():" << m_dlControlMessagesQueue.at (1).size () << "\n\n";
#endif  
  if (m_controlMessagesQueue.at (0).size () > 0)
    {
      if(dl_dci_found_at0 && other_found > 0)
      {
        if(other_found > 1)
        {
          std::cout << "LteControlMessage::DL_DCI && OTHER Control messages >= 1\n";
          //NS_ASSERT(0);
        }
        std::list<Ptr<LteControlMessage> > newlist;
        newlist.push_back (*other_it);
#ifdef DEBUG_CONTROL_DATA_QUEUE
        std::cout << "LteControlMessage::DL_DCI && OTHER Control messages\n";
        std::cout << "LtePhy::GetControlMessages.size():" << m_controlMessagesQueue.size() << " m_controlMessagesQueue.at (0).size (): "  << m_controlMessagesQueue.at (0).size () << " m_controlMessagesQueue.at (1).size (): "  << m_controlMessagesQueue.at (1).size () <<"\n";
#endif        
        Ptr<LteControlMessage> msg = (*other_it);        
        string messageType = (msg->GetMessageType () == LteControlMessage::DL_DCI)?"DL_DCI":
        (msg->GetMessageType () == LteControlMessage::UL_DCI)?"UL_DCI":
        (msg->GetMessageType () == LteControlMessage::DL_CQI)?"DL_CQI":
        (msg->GetMessageType () == LteControlMessage::UL_CQI)?"UL_CQI":
        (msg->GetMessageType () == LteControlMessage::BSR)?"BSR":
        (msg->GetMessageType () == LteControlMessage::DL_HARQ)?"DL_HARQ":
        (msg->GetMessageType () == LteControlMessage::RACH_PREAMBLE)?"RACH_PREAMBLE":
        (msg->GetMessageType () == LteControlMessage::RAR)?"RAR":
        (msg->GetMessageType () == LteControlMessage::MIB)?"MIB":
        (msg->GetMessageType () == LteControlMessage::SIB1)?"SIB1":"???";
        NS_LOG_FUNCTION (this << messageType);
        msg = (*dl_dci_it);
        messageType = (msg->GetMessageType () == LteControlMessage::DL_DCI)?"DL_DCI":
        (msg->GetMessageType () == LteControlMessage::UL_DCI)?"UL_DCI":
        (msg->GetMessageType () == LteControlMessage::DL_CQI)?"DL_CQI":
        (msg->GetMessageType () == LteControlMessage::UL_CQI)?"UL_CQI":
        (msg->GetMessageType () == LteControlMessage::BSR)?"BSR":
        (msg->GetMessageType () == LteControlMessage::DL_HARQ)?"DL_HARQ":
        (msg->GetMessageType () == LteControlMessage::RACH_PREAMBLE)?"RACH_PREAMBLE":
        (msg->GetMessageType () == LteControlMessage::RAR)?"RAR":
        (msg->GetMessageType () == LteControlMessage::MIB)?"MIB":
        (msg->GetMessageType () == LteControlMessage::SIB1)?"SIB1":"???";
        NS_LOG_FUNCTION (this << messageType);
        m_controlMessagesQueue.at (0).remove (*other_it);
#ifdef DEBUG_CONTROL_DATA_QUEUE
        std::cout << "LtePhy::GetControlMessages.size():" << m_controlMessagesQueue.size() << " m_controlMessagesQueue.at (0).size (): "  << m_controlMessagesQueue.at (0).size () << " m_controlMessagesQueue.at (1).size (): "  << m_controlMessagesQueue.at (1).size () <<"\n";
#endif        
        return (newlist);
        
      } //don't push and let's keep dl-dci in its safe place
      else if(!dl_dci_found_at0 && dl_dci_found_at1)
      {
#ifdef DEBUG_CONTROL_DATA_QUEUE
        std::cout << "!dl_dci_found_at0 && dl_dci_found_at1)\n";
        std::cout << "LtePhy::GetControlMessages.size():" << m_controlMessagesQueue.size() << " m_controlMessagesQueue.at (0).size (): "  << m_controlMessagesQueue.at (0).size () << " m_controlMessagesQueue.at (1).size ():" << m_controlMessagesQueue.at (1).size () << "\n\n";
#endif        
        //std::list<Ptr<LteControlMessage> > ret = m_controlMessagesQueue.at (0);
        //m_controlMessagesQueue.erase (m_controlMessagesQueue.begin ());
        while (!m_controlMessagesQueue.at (0).empty())
        {
           m_controlMessagesQueue.at (0).pop_front();
        }
#ifdef DEBUG_CONTROL_DATA_QUEUE
        std::cout << "LtePhy::GetControlMessages.size():" << m_controlMessagesQueue.size() << " m_controlMessagesQueue.at (0).size (): "  << m_controlMessagesQueue.at (0).size () << " m_controlMessagesQueue.at (1).size ():" << m_controlMessagesQueue.at (1).size () << "\n\n";
#endif        
        return (ret);
      }
      else
      {
        std::list<Ptr<LteControlMessage> > ret = m_controlMessagesQueue.at (0);
        m_controlMessagesQueue.erase (m_controlMessagesQueue.begin ());
        std::list<Ptr<LteControlMessage> > newlist;
        m_controlMessagesQueue.push_back (newlist);
        return (ret);
      }
    }
  else
    {
      if(!dl_dci_found_at1)
      {
        m_controlMessagesQueue.erase (m_controlMessagesQueue.begin ());
        std::list<Ptr<LteControlMessage> > newlist;
        m_controlMessagesQueue.push_back (newlist);
#ifdef DEBUG_CONTROL_DATA_QUEUE
        std::cout << "LtePhy::GetControlMessages.size():" << m_controlMessagesQueue.size() << " m_controlMessagesQueue.at (0).size (): "  << m_controlMessagesQueue.at (0).size () << " m_controlMessagesQueue.at (1).size ():" << m_controlMessagesQueue.at (1).size () << "\n\n";
#endif      
      }
      else if(dl_dci_found_at1 && (nrSubFrames != 2))
      {
#ifdef DEBUG_CONTROL_DATA_QUEUE
        std::cout << "LtePhy::GetControlMessages.size():" << m_controlMessagesQueue.size() << " m_controlMessagesQueue.at (0).size (): "  << m_controlMessagesQueue.at (0).size () << " m_controlMessagesQueue.at (1).size ():" << m_controlMessagesQueue.at (1).size () << "\n\n";
#endif      
      }
      else if(dl_dci_found_at1 && (nrSubFrames == 2))
      {
        m_controlMessagesQueue.erase (m_controlMessagesQueue.begin ());
        std::list<Ptr<LteControlMessage> > newlist;
        m_controlMessagesQueue.push_back (newlist);
#ifdef DEBUG_CONTROL_DATA_QUEUE
        std::cout << "LtePhy::GetControlMessages.size():" << m_controlMessagesQueue.size() << " m_controlMessagesQueue.at (0).size (): "  << m_controlMessagesQueue.at (0).size () << " m_controlMessagesQueue.at (1).size ():" << m_controlMessagesQueue.at (1).size () << "\n\n";
#endif
      }
      std::list<Ptr<LteControlMessage> > emptylist;
      return (emptylist);
    }
}


std::list<Ptr<LteControlMessage> >
LtePhy::GetDlControlMessages (uint32_t nrSubFrames)
{
  NS_LOG_FUNCTION (this);
  std::list<Ptr<LteControlMessage> > ret = m_dlControlMessagesQueue.at (0);
  if (ret.size () > 0)
    {
      std::list<Ptr<LteControlMessage> >::iterator it;
      it = ret.begin ();
      while (it != ret.end ())
        {
          Ptr<LteControlMessage> msg = (*it);
          string messageType = (msg->GetMessageType () == LteControlMessage::DL_DCI)?"DL_DCI":
          (msg->GetMessageType () == LteControlMessage::UL_DCI)?"UL_DCI":
          (msg->GetMessageType () == LteControlMessage::DL_CQI)?"DL_CQI":
          (msg->GetMessageType () == LteControlMessage::UL_CQI)?"UL_CQI":"???";

          NS_LOG_FUNCTION (this << messageType);

          if((msg->GetMessageType () == LteControlMessage::DL_DCI) && (nrSubFrames == 2))
          {
#ifdef DEBUG_CONTROL_DATA_QUEUE
            std::cout << "LteControlMessage::DL_DCI\n";
#endif
          }
          else if((msg->GetMessageType () == LteControlMessage::DL_DCI) && (nrSubFrames != 2))
          {
            std::list<Ptr<LteControlMessage> > emptylist;
            return (emptylist);
          }
          else
          {
#ifdef DEBUG_CONTROL_DATA_QUEUE
            std::cout << "LteControlMessage NOT DL_DCI\n";
#endif
            NS_ASSERT(0);
          }
          it++;
        }
    }
#ifdef DEBUG_CONTROL_DATA_QUEUE
  std::cout << "LtePhy::GetControlMessages.size():" << m_dlControlMessagesQueue.size() << " m_dlControlMessagesQueue.at (0).size (): "  << m_dlControlMessagesQueue.at (0).size () << "\n\n";
#endif  
  if (m_dlControlMessagesQueue.at (0).size () > 0)
    {
      std::list<Ptr<LteControlMessage> > ret = m_dlControlMessagesQueue.at (0);
      m_dlControlMessagesQueue.erase (m_dlControlMessagesQueue.begin ());
      std::list<Ptr<LteControlMessage> > newlist;
      m_dlControlMessagesQueue.push_back (newlist);
      return (ret);
    }
  else
    {
      m_dlControlMessagesQueue.erase (m_dlControlMessagesQueue.begin ());
      std::list<Ptr<LteControlMessage> > newlist;
      m_dlControlMessagesQueue.push_back (newlist);
      std::list<Ptr<LteControlMessage> > emptylist;
      return (emptylist);
    }
}


void
LtePhy::DoSetCellId (uint16_t cellId)
{
  m_cellId = cellId;
  m_downlinkSpectrumPhy->SetCellId (cellId);
  m_uplinkSpectrumPhy->SetCellId (cellId);
}

void
LtePhy::SetComponentCarrierId (uint8_t index)
{
  m_componentCarrierId = index;
  m_downlinkSpectrumPhy->SetComponentCarrierId (index);
  m_uplinkSpectrumPhy->SetComponentCarrierId (index);
}

uint8_t
LtePhy::GetComponentCarrierId ()
{
  return m_componentCarrierId;
}

} // namespace ns3
