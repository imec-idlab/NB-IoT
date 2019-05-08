/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Marco Miozzo <marco.miozzo@cttc.es>
 */

#include <ns3/log.h>
#include <ns3/pointer.h>
#include <ns3/math.h>
#include <cfloat>
#include <set>
#include <climits>
#include <ns3/lte-mi-error-model.h>
#include <ns3/lte-amc.h>
#include <ns3/rr-ff-mac-scheduler.h>
#include <ns3/simulator.h>
#include <ns3/lte-common.h>
#include <ns3/lte-vendor-specific-parameters.h>
#include <ns3/boolean.h>
#include <ns3/lte-amc.h>
#include "lte-spectrum-value-helper.h"
#include <chrono>

#define DCI_BW_DEBUG 0
#define CQIBASED 0
#define CQITHRESHOLD 8
#define BLERBASED 0
#define TARGERBLER 0.05
#define MODNO 10




//STATIC VARIABLE TO HOLD NON ANCHOR RB FOR NEXT ITERATIONS.
static  std::vector<uint32_t> ExecTime(10000, 0);
static  std::vector<uint16_t> SRS(10000, 0);
static  std::vector<uint16_t> RBno(10000, 200);
// Subframes left for the RB
static  std::vector<uint16_t> sfleft(100, 0);
// Subcarriers left for the RB
static  std::vector<uint16_t> scleft(100, 12);
// Subcarrier start index
static  std::vector<uint16_t> scstart(100, 0);
static  std::vector<uint16_t> Rnti(100, 0);
namespace ns3 {

static const Time UL_SRS_DELAY_FROM_SUBFRAME_START = NanoSeconds (1e6 - 71429); 

NS_LOG_COMPONENT_DEFINE ("RrFfMacScheduler");

static const int Type0AllocationRbg[4] = {
  10,       // RGB size 1
  26,       // RGB size 2
  63,       // RGB size 3
  110       // RGB size 4
};  // see table 7.1.6.1-1 of 36.213




NS_OBJECT_ENSURE_REGISTERED (RrFfMacScheduler);


RrFfMacScheduler::RrFfMacScheduler ()
  :   m_cschedSapUser (0),
    m_schedSapUser (0),
    m_nextRntiDl (0),
    m_nextRntiUl (0)
{
  m_amc = CreateObject <LteAmc> ();
  m_cschedSapProvider = new MemberCschedSapProvider<RrFfMacScheduler> (this);
  m_schedSapProvider = new MemberSchedSapProvider<RrFfMacScheduler> (this);
  repetitionCounter = 0;
  m_ffrSapProvider = 0;
  m_ffrSapUser = new MemberLteFfrSapUser<RrFfMacScheduler> (this);
}

RrFfMacScheduler::~RrFfMacScheduler ()
{
  NS_LOG_FUNCTION (this);
}

void
RrFfMacScheduler::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_dlHarqProcessesDciBuffer.clear ();
  m_dlHarqProcessesTimer.clear ();
  m_dlHarqProcessesRlcPduListBuffer.clear ();
  m_dlInfoListBuffered.clear ();
  m_ulHarqCurrentProcessId.clear ();
  m_ulHarqProcessesStatus.clear ();
  m_ulHarqProcessesDciBuffer.clear ();
  delete m_cschedSapProvider;
  delete m_schedSapProvider;
  delete m_ffrSapUser;
}

int indexofSmallestElement(double array[], int size)
{
    int index = 0;

    for(int i = 1; i < size; i++)
    {
        if(array[i] < array[index])
            index = i;              
    }

    return index;
}

TypeId
RrFfMacScheduler::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RrFfMacScheduler")
    .SetParent<FfMacScheduler> ()
    .SetGroupName("Lte")
    .AddConstructor<RrFfMacScheduler> ()
    .AddAttribute ("CqiTimerThreshold",
                   "The number of TTIs a CQI is valid (default 1000 - 1 sec.)",
                   UintegerValue (1000),
                   MakeUintegerAccessor (&RrFfMacScheduler::m_cqiTimersThreshold),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("HarqEnabled",
                   "Activate/Deactivate the HARQ [by default is active].",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RrFfMacScheduler::m_harqOn),
                   MakeBooleanChecker ())
    .AddAttribute ("UlGrantMcs",
                   "The MCS of the UL grant, must be [0..15] (default 0)",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RrFfMacScheduler::m_ulGrantMcs),
                   MakeUintegerChecker<uint8_t> ())
  ;
  return tid;
}

void RrFfMacScheduler::setsrstime(uint16_t timesrs, uint16_t rnti)
{
  SRS.at(rnti) =timesrs;

}

void
RrFfMacScheduler::SetFfMacCschedSapUser (FfMacCschedSapUser* s)
{
  m_cschedSapUser = s;
}

void
RrFfMacScheduler::SetFfMacSchedSapUser (FfMacSchedSapUser* s)
{
  m_schedSapUser = s;
}

FfMacCschedSapProvider*
RrFfMacScheduler::GetFfMacCschedSapProvider ()
{
  return m_cschedSapProvider;
}

FfMacSchedSapProvider*
RrFfMacScheduler::GetFfMacSchedSapProvider ()
{
  return m_schedSapProvider;
}

void
RrFfMacScheduler::SetLteFfrSapProvider (LteFfrSapProvider* s)
{
  m_ffrSapProvider = s;
}

LteFfrSapUser*
RrFfMacScheduler::GetLteFfrSapUser ()
{
  return m_ffrSapUser;
}

void
RrFfMacScheduler::DoCschedCellConfigReq (const struct FfMacCschedSapProvider::CschedCellConfigReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  // Read the subset of parameters used
  m_cschedCellConfig = params;
  m_rachAllocationMap.resize (m_cschedCellConfig.m_ulBandwidth, 0);
  FfMacCschedSapUser::CschedUeConfigCnfParameters cnf;
  cnf.m_result = SUCCESS;
  m_cschedSapUser->CschedUeConfigCnf (cnf);
  return;
}

void
RrFfMacScheduler::DoCschedUeConfigReq (const struct FfMacCschedSapProvider::CschedUeConfigReqParameters& params)
{
  NS_LOG_FUNCTION (this << " RNTI " << params.m_rnti << " txMode " << (uint16_t)params.m_transmissionMode);
  std::map <uint16_t,uint8_t>::iterator it = m_uesTxMode.find (params.m_rnti);
  if (it == m_uesTxMode.end ())
    {
      m_uesTxMode.insert (std::pair <uint16_t, double> (params.m_rnti, params.m_transmissionMode));
      // generate HARQ buffers
      m_dlHarqCurrentProcessId.insert (std::pair <uint16_t,uint8_t > (params.m_rnti, 0));
      DlHarqProcessesStatus_t dlHarqPrcStatus;
      dlHarqPrcStatus.resize (8,0);
      m_dlHarqProcessesStatus.insert (std::pair <uint16_t, DlHarqProcessesStatus_t> (params.m_rnti, dlHarqPrcStatus));
      DlHarqProcessesTimer_t dlHarqProcessesTimer;
      dlHarqProcessesTimer.resize (8,0);
      m_dlHarqProcessesTimer.insert (std::pair <uint16_t, DlHarqProcessesTimer_t> (params.m_rnti, dlHarqProcessesTimer));
      DlHarqProcessesDciBuffer_t dlHarqdci;
      dlHarqdci.resize (8);
      m_dlHarqProcessesDciBuffer.insert (std::pair <uint16_t, DlHarqProcessesDciBuffer_t> (params.m_rnti, dlHarqdci));
      DlHarqRlcPduListBuffer_t dlHarqRlcPdu;
      dlHarqRlcPdu.resize (2);
      dlHarqRlcPdu.at (0).resize (8);
      dlHarqRlcPdu.at (1).resize (8);
      m_dlHarqProcessesRlcPduListBuffer.insert (std::pair <uint16_t, DlHarqRlcPduListBuffer_t> (params.m_rnti, dlHarqRlcPdu));
      m_ulHarqCurrentProcessId.insert (std::pair <uint16_t,uint8_t > (params.m_rnti, 0));
      UlHarqProcessesStatus_t ulHarqPrcStatus;
      ulHarqPrcStatus.resize (8,0);
      m_ulHarqProcessesStatus.insert (std::pair <uint16_t, UlHarqProcessesStatus_t> (params.m_rnti, ulHarqPrcStatus));
      UlHarqProcessesDciBuffer_t ulHarqdci;
      ulHarqdci.resize (8);
      m_ulHarqProcessesDciBuffer.insert (std::pair <uint16_t, UlHarqProcessesDciBuffer_t> (params.m_rnti, ulHarqdci));
    }
  else
    {
      (*it).second = params.m_transmissionMode;
    }
  return;
}

void
RrFfMacScheduler::DoCschedLcConfigReq (const struct FfMacCschedSapProvider::CschedLcConfigReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  // Not used at this stage (LCs updated by DoSchedDlRlcBufferReq)
  return;
}

void
RrFfMacScheduler::DoCschedLcReleaseReq (const struct FfMacCschedSapProvider::CschedLcReleaseReqParameters& params)
{
  NS_LOG_FUNCTION (this);
    for (uint16_t i = 0; i < params.m_logicalChannelIdentity.size (); i++)
    {
     std::list<FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it = m_rlcBufferReq.begin ();
      while (it!=m_rlcBufferReq.end ())
        {
          if (((*it).m_rnti == params.m_rnti)&&((*it).m_logicalChannelIdentity == params.m_logicalChannelIdentity.at (i)))
            {
              it = m_rlcBufferReq.erase (it);
            }
          else
            {
              it++;
            }
        }
    }
  return;
}

void
RrFfMacScheduler::DoCschedUeReleaseReq (const struct FfMacCschedSapProvider::CschedUeReleaseReqParameters& params)
{
  NS_LOG_FUNCTION (this << " Release RNTI " << params.m_rnti);
  
  m_uesTxMode.erase (params.m_rnti);
  m_dlHarqCurrentProcessId.erase (params.m_rnti);
  m_dlHarqProcessesStatus.erase  (params.m_rnti);
  m_dlHarqProcessesTimer.erase (params.m_rnti);
  m_dlHarqProcessesDciBuffer.erase  (params.m_rnti);
  m_dlHarqProcessesRlcPduListBuffer.erase  (params.m_rnti);
  m_ulHarqCurrentProcessId.erase  (params.m_rnti);
  m_ulHarqProcessesStatus.erase  (params.m_rnti);
  m_ulHarqProcessesDciBuffer.erase  (params.m_rnti);
  m_ceBsrRxed.erase (params.m_rnti);
  std::list<FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it = m_rlcBufferReq.begin ();
  while (it != m_rlcBufferReq.end ())
    {
      if ((*it).m_rnti == params.m_rnti)
        {
          NS_LOG_INFO (this << " Erase RNTI " << (*it).m_rnti << " LC " << (uint16_t)(*it).m_logicalChannelIdentity);
          it = m_rlcBufferReq.erase (it);
        }
      else
        {
          it++;
        }
    }
  if (m_nextRntiUl == params.m_rnti)
    {
      m_nextRntiUl = 0;
    }

  if (m_nextRntiDl == params.m_rnti)
    {
      m_nextRntiDl = 0;
    }
    
  return;
}


void
RrFfMacScheduler::DoSchedDlRlcBufferReq (const struct FfMacSchedSapProvider::SchedDlRlcBufferReqParameters& params)
{
  NS_LOG_FUNCTION (this << params.m_rnti << (uint32_t) params.m_logicalChannelIdentity);
  // API generated by RLC for updating RLC parameters on a LC (tx and retx queues)
  std::list<FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it = m_rlcBufferReq.begin ();
  bool newLc = true;
  while (it != m_rlcBufferReq.end ())
    {
      // remove old entries of this UE-LC
      if (((*it).m_rnti == params.m_rnti)&&((*it).m_logicalChannelIdentity == params.m_logicalChannelIdentity))
        {
          it = m_rlcBufferReq.erase (it);
          newLc = false;
        }
      else
        {
          ++it;
        }
    }
  // add the new parameters
  m_rlcBufferReq.insert (it, params);
  NS_LOG_INFO (this << " RNTI " << params.m_rnti << " LC " << (uint16_t)params.m_logicalChannelIdentity << " RLC tx size " << params.m_rlcTransmissionQueueHolDelay << " RLC retx size " << params.m_rlcRetransmissionQueueSize << " RLC stat size " <<  params.m_rlcStatusPduSize);
  // initialize statistics of the flow in case of new flows
  if (newLc == true)
    {
      m_p10CqiRxed.insert ( std::pair<uint16_t, uint8_t > (params.m_rnti, 1)); // only codeword 0 at this stage (SISO)
      // initialized to 1 (i.e., the lowest value for transmitting a signal)
      m_p10CqiTimers.insert ( std::pair<uint16_t, uint32_t > (params.m_rnti, m_cqiTimersThreshold));
    }

  return;
}

void
RrFfMacScheduler::DoSchedDlPagingBufferReq (const struct FfMacSchedSapProvider::SchedDlPagingBufferReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("method not implemented");
  return;
}

void
RrFfMacScheduler::DoSchedDlMacBufferReq (const struct FfMacSchedSapProvider::SchedDlMacBufferReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("method not implemented");
  return;
}

int
RrFfMacScheduler::GetRbgSize (int dlbandwidth)
{
  return (1);
  /*for (int i = 0; i < 4; i++)
    {
      if (dlbandwidth < Type0AllocationRbg[i])
        {
          return (i + 1);
        }
    }

  return (-1);*/
}

bool
RrFfMacScheduler::SortRlcBufferReq (FfMacSchedSapProvider::SchedDlRlcBufferReqParameters i,FfMacSchedSapProvider::SchedDlRlcBufferReqParameters j)
{
  return (i.m_rnti < j.m_rnti);
}


uint8_t
RrFfMacScheduler::HarqProcessAvailability (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << rnti);

  std::map <uint16_t, uint8_t>::iterator it = m_dlHarqCurrentProcessId.find (rnti);
  if (it == m_dlHarqCurrentProcessId.end ())
    {
      NS_FATAL_ERROR ("No Process Id found for this RNTI " << rnti);
    }
  std::map <uint16_t, DlHarqProcessesStatus_t>::iterator itStat = m_dlHarqProcessesStatus.find (rnti);
  if (itStat == m_dlHarqProcessesStatus.end ())
    {
      NS_FATAL_ERROR ("No Process Id Statusfound for this RNTI " << rnti);
    }
  uint8_t i = (*it).second;
  do
    {
      i = (i + 1) % HARQ_PROC_NUM;
    }
  while ( ((*itStat).second.at (i) != 0)&&(i != (*it).second));
  if ((*itStat).second.at (i) == 0)
    {
      return (true);
    }
  else
    {
      return (false); // return a not valid harq proc id
    }
}



uint8_t
RrFfMacScheduler::UpdateHarqProcessId (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << rnti);


  if (m_harqOn == false)
    {
      return (0);
    }

  std::map <uint16_t, uint8_t>::iterator it = m_dlHarqCurrentProcessId.find (rnti);
  if (it == m_dlHarqCurrentProcessId.end ())
    {
      NS_FATAL_ERROR ("No Process Id found for this RNTI " << rnti);
    }
  std::map <uint16_t, DlHarqProcessesStatus_t>::iterator itStat = m_dlHarqProcessesStatus.find (rnti);
  if (itStat == m_dlHarqProcessesStatus.end ())
    {
      NS_FATAL_ERROR ("No Process Id Statusfound for this RNTI " << rnti);
    }
  uint8_t i = (*it).second;
  do
    {
      i = (i + 1) % HARQ_PROC_NUM;
    }
  while ( ((*itStat).second.at (i) != 0)&&(i != (*it).second));
  if ((*itStat).second.at (i) == 0)
    {
      (*it).second = i;
      (*itStat).second.at (i) = 1;
    }
  else
    {
      return (9); // return a not valid harq proc id
    }

  return ((*it).second);
}


void
RrFfMacScheduler::RefreshHarqProcesses ()
{
  //NS_LOG_FUNCTION (this);

  std::map <uint16_t, DlHarqProcessesTimer_t>::iterator itTimers;
  for (itTimers = m_dlHarqProcessesTimer.begin (); itTimers != m_dlHarqProcessesTimer.end (); itTimers ++)
    {
      for (uint16_t i = 0; i < HARQ_PROC_NUM; i++)
        {
          if ((*itTimers).second.at (i) == HARQ_DL_TIMEOUT)
            {
              // reset HARQ process

              //NS_LOG_INFO (this << " Reset HARQ proc " << i << " for RNTI " << (*itTimers).first);
              std::map <uint16_t, DlHarqProcessesStatus_t>::iterator itStat = m_dlHarqProcessesStatus.find ((*itTimers).first);
              if (itStat == m_dlHarqProcessesStatus.end ())
                {
                  NS_FATAL_ERROR ("No Process Id Status found for this RNTI " << (*itTimers).first);
                }
              (*itStat).second.at (i) = 0;
              (*itTimers).second.at (i) = 0;
            }
          else
            {
              (*itTimers).second.at (i)++;
            }
        }
    }

}



void
RrFfMacScheduler::DoSchedDlTriggerReq (const struct FfMacSchedSapProvider::SchedDlTriggerReqParameters& params)
{
  uint32_t dlSchedSubframeNo = 0xF & params.m_sfnSf;
  //NS_LOG_FUNCTION (this << " DL Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf) << " return:" << (dlSchedSubframeNo % 3 == 0));
  
  // API generated by RLC for triggering the scheduling of a DL subframe

  RefreshDlCqiMaps ();
  int rbgSize = GetRbgSize (m_cschedCellConfig.m_dlBandwidth);
  int rbgNum = m_cschedCellConfig.m_dlBandwidth / rbgSize;
  FfMacSchedSapUser::SchedDlConfigIndParameters ret;

  // Generate RBGs map
  std::vector <bool> rbgMap;
  uint16_t rbgAllocatedNum = 0;
  std::set <uint16_t> rntiAllocated;
  rbgMap.resize (m_cschedCellConfig.m_dlBandwidth / rbgSize, false);
  rbgMap = m_ffrSapProvider->GetAvailableDlRbg ();
  for (std::vector<bool>::iterator it = rbgMap.begin (); it != rbgMap.end (); it++)
    {
      if ((*it) == true )
        {
          rbgAllocatedNum++;
        }
    }

  //   update UL HARQ proc id
  std::map <uint16_t, uint8_t>::iterator itProcId;
  for (itProcId = m_ulHarqCurrentProcessId.begin (); itProcId != m_ulHarqCurrentProcessId.end (); itProcId++)
    {
      (*itProcId).second = ((*itProcId).second + 1) % HARQ_PROC_NUM;
    }

  // RACH Allocation
  uint16_t rbAllocatedNum = 0;
  std::vector <bool> ulRbMap;
  m_rachAllocationMap.resize (m_cschedCellConfig.m_ulBandwidth, 0);
  ulRbMap.resize (m_cschedCellConfig.m_ulBandwidth, false);
  ulRbMap = m_ffrSapProvider->GetAvailableUlRbg ();
  uint8_t maxContinuousUlBandwidth = 0;
  uint8_t tmpMinBandwidth = 0;
  uint16_t ffrRbStartOffset = 0;
  uint16_t tmpFfrRbStartOffset = 0;
  uint16_t index = 0;
  for (uint16_t i = 0; i < m_cschedCellConfig.m_ulBandwidth; i++)
    {
      // Make sure that RB is not used, as it is assigned for next subframe as well.
      if (sfleft.at(i)>0)
        {
          ulRbMap.at (i) = true;
         }        
          
    }
  for (std::vector<bool>::iterator it = ulRbMap.begin (); it != ulRbMap.end (); it++)
    {
      if ((*it) == true )
        {
          rbAllocatedNum++;
          if (tmpMinBandwidth > maxContinuousUlBandwidth)
            {
              maxContinuousUlBandwidth = tmpMinBandwidth;
              ffrRbStartOffset = tmpFfrRbStartOffset;
            }
          tmpMinBandwidth = 0;
        }
      else
        {
          if (tmpMinBandwidth == 0)
            {
              tmpFfrRbStartOffset = index;
            }
          tmpMinBandwidth++;
        }
      index++;
    }
  if (tmpMinBandwidth > maxContinuousUlBandwidth)
    {
      maxContinuousUlBandwidth = tmpMinBandwidth;
      ffrRbStartOffset = tmpFfrRbStartOffset;
    }
  uint16_t rbStart = 0;
  rbStart = ffrRbStartOffset;
  //std::cout << " ffrRbStartOffset: " << ffrRbStartOffset << " maxContinuousUlBandwidth: " << (int)maxContinuousUlBandwidth << " tmpMinBandwidth:" << (int)tmpMinBandwidth << "\n";
  std::vector <struct RachListElement_s>::iterator itRach;
  for (itRach = m_rachList.begin (); itRach != m_rachList.end (); itRach++)
    {
      NS_ASSERT_MSG (m_amc->GetTbSizeFromMcs (m_ulGrantMcs, m_cschedCellConfig.m_ulBandwidth) > (*itRach).m_estimatedSize, " Default UL Grant MCS does not allow to send RACH messages");
      BuildRarListElement_s newRar;
      newRar.m_rnti = (*itRach).m_rnti;
      // DL-RACH Allocation
      // Ideal: no needs of configuring m_dci
      // UL-RACH Allocation
      newRar.m_grant.m_rnti = newRar.m_rnti;
      newRar.m_grant.m_mcs = m_ulGrantMcs;
      
      uint16_t rbLen = 1;
      uint16_t tbSizeBits = 0;
      // find lowest TB size that fits UL grant estimated size
      //std::cout << " tbSizeBits: " << tbSizeBits << " (*itRach).m_estimatedSize: " << (*itRach).m_estimatedSize << " rbStart:" << rbStart << " rbLen:" << rbLen<< " ffrRbStartOffset: " << ffrRbStartOffset << " maxContinuousUlBandwidth:" << maxContinuousUlBandwidth << "\n";
      while ((tbSizeBits < (*itRach).m_estimatedSize) && (rbStart + rbLen < (ffrRbStartOffset + maxContinuousUlBandwidth)))
        {
          rbLen++;
          tbSizeBits = m_amc->GetTbSizeFromMcs (m_ulGrantMcs, rbLen);
          //std::cout << " tbSizeBits: " << tbSizeBits << " < m_estimatedSize:" << (*itRach).m_estimatedSize << " rbLen:" << rbLen << "\n";
        }
      if (tbSizeBits < (*itRach).m_estimatedSize)
        {
          // no more allocation space: finish allocation
          //std::cout << " no more allocation space: finish allocation: " << tbSizeBits << " < " << (*itRach).m_estimatedSize << "\n";
          break;
        }
      //std::cout << " m_rbStart: " << rbStart << " rbLen: " << rbLen << " m_tbSize: " << tbSizeBits / 8 << "\n";
      newRar.m_grant.m_rbStart = rbStart;
      newRar.m_grant.m_rbLen = rbLen;
      newRar.m_grant.m_tbSize = tbSizeBits / 8;
      int N_rb_ul = m_cschedCellConfig.m_ulBandwidth;
      int N_prb = rbLen;
      //int RB_start = newRar.m_grant.m_riv % N_rb_ul;
      uint32_t riv = 0;
      if((N_prb-1) <= (N_rb_ul/2))
        {
          riv = N_rb_ul * (N_prb - 1) + rbStart;
        }
        else
        {
          riv = N_rb_ul * (N_rb_ul - N_prb+1) + (N_rb_ul - 1 - rbStart);
        }
      newRar.m_grant.m_riv = riv;
      //std::cout << "[RfFF:newRar]riv: " << riv << " N_prb:" << N_prb << " N_rb_ul:" << N_rb_ul << " rb_start:" << rbStart << " rbLen:" << rbLen << "\n";
      newRar.m_grant.m_hopping = false;
      newRar.m_grant.m_tpc = 0;
      newRar.m_grant.m_cqiRequest = false;
      newRar.m_grant.m_ulDelay = false;
      NS_LOG_INFO (this << " UL grant allocated to RNTI " << (*itRach).m_rnti << " rbStart " << rbStart << " rbLen " << rbLen << " MCS " << (uint16_t) m_ulGrantMcs << " tbSize " << newRar.m_grant.m_tbSize);
      for (uint16_t i = rbStart; i < rbStart + rbLen; i++)
        {
          m_rachAllocationMap.at (i) = (*itRach).m_rnti;
        }

      if (m_harqOn == true)
        {
          // generate UL-DCI for HARQ retransmissions
          UlDciListElement_s uldci;
          uldci.m_rnti = newRar.m_rnti;
          uldci.m_rbLen = rbLen;
          uldci.m_rbStart = rbStart;
          uldci.m_mcs = m_ulGrantMcs;
          uldci.m_tbSize = tbSizeBits / 8;
          uldci.m_ndi = 1;
          uldci.m_cceIndex = 0;
          uldci.m_aggrLevel = 1;
          uldci.m_ueTxAntennaSelection = 3; // antenna selection OFF
          uldci.m_hopping = false;
          uldci.m_n2Dmrs = 0;
          uldci.m_tpc = 0; // no power control
          uldci.m_cqiRequest = false; // only period CQI at this stage
          uldci.m_ulIndex = 0; // TDD parameter
          uldci.m_dai = 1; // TDD parameter
          uldci.m_freqHopping = 0;
          uldci.m_pdcchPowerOffset = 0; // not used

          uint8_t harqId = 0;
          std::map <uint16_t, uint8_t>::iterator itProcId;
          itProcId = m_ulHarqCurrentProcessId.find (uldci.m_rnti);
          if (itProcId == m_ulHarqCurrentProcessId.end ())
            {
              NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << uldci.m_rnti);
            }
          harqId = (*itProcId).second;
          std::map <uint16_t, UlHarqProcessesDciBuffer_t>::iterator itDci = m_ulHarqProcessesDciBuffer.find (uldci.m_rnti);
          if (itDci == m_ulHarqProcessesDciBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RNTI entry in UL DCI HARQ buffer for RNTI " << uldci.m_rnti);
            }
          (*itDci).second.at (harqId) = uldci;
        }

      rbStart = rbStart + rbLen;
      ret.m_buildRarList.push_back (newRar);
    }
  m_rachList.clear ();

  // Process DL HARQ feedback
  RefreshHarqProcesses ();
  // retrieve past HARQ retx buffered
  if (m_dlInfoListBuffered.size () > 0)
    {
      if (params.m_dlInfoList.size () > 0)
        {
          NS_LOG_INFO (this << " Received DL-HARQ feedback");
          m_dlInfoListBuffered.insert (m_dlInfoListBuffered.end (), params.m_dlInfoList.begin (), params.m_dlInfoList.end ());
        }
    }
  else
    {
      if (params.m_dlInfoList.size () > 0)
        {
          m_dlInfoListBuffered = params.m_dlInfoList;
        }
    }
  if (m_harqOn == false)
    {
      // Ignore HARQ feedback
      m_dlInfoListBuffered.clear ();
    }
  std::vector <struct DlInfoListElement_s> dlInfoListUntxed;
  for (uint16_t i = 0; i < m_dlInfoListBuffered.size (); i++)
    {
      std::set <uint16_t>::iterator itRnti = rntiAllocated.find (m_dlInfoListBuffered.at (i).m_rnti);
      if (itRnti != rntiAllocated.end ())
        {
          // RNTI already allocated for retx
          continue;
        }
      uint8_t nLayers = m_dlInfoListBuffered.at (i).m_harqStatus.size ();
      std::vector <bool> retx;
      NS_LOG_INFO (this << " Processing DLHARQ feedback");
      if (nLayers == 1)
        {
          retx.push_back (m_dlInfoListBuffered.at (i).m_harqStatus.at (0) == DlInfoListElement_s::NACK);
          retx.push_back (false);
        }
      else
        {
          retx.push_back (m_dlInfoListBuffered.at (i).m_harqStatus.at (0) == DlInfoListElement_s::NACK);
          retx.push_back (m_dlInfoListBuffered.at (i).m_harqStatus.at (1) == DlInfoListElement_s::NACK);
        }
      if (retx.at (0) || retx.at (1))
        {
          // retrieve HARQ process information
          uint16_t rnti = m_dlInfoListBuffered.at (i).m_rnti;
          uint8_t harqId = m_dlInfoListBuffered.at (i).m_harqProcessId;
          NS_LOG_INFO (this << " HARQ retx RNTI " << rnti << " harqId " << (uint16_t)harqId);
          std::map <uint16_t, DlHarqProcessesDciBuffer_t>::iterator itHarq = m_dlHarqProcessesDciBuffer.find (rnti);
          if (itHarq == m_dlHarqProcessesDciBuffer.end ())
            {
              NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << rnti);
            }

          DlDciListElement_s dci = (*itHarq).second.at (harqId);
          int rv = 0;
          if (dci.m_rv.size () == 1)
            {
              rv = dci.m_rv.at (0);
            }
          else
            {
              rv = (dci.m_rv.at (0) > dci.m_rv.at (1) ? dci.m_rv.at (0) : dci.m_rv.at (1));
            }

          if (rv == 3)
            {
              // maximum number of retx reached -> drop process
              NS_LOG_INFO ("Max number of retransmissions reached -> drop process");
              std::map <uint16_t, DlHarqProcessesStatus_t>::iterator it = m_dlHarqProcessesStatus.find (rnti);
              if (it == m_dlHarqProcessesStatus.end ())
                {
                  NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << m_dlInfoListBuffered.at (i).m_rnti);
                }
              (*it).second.at (harqId) = 0;
              std::map <uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =  m_dlHarqProcessesRlcPduListBuffer.find (rnti);
              if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end ())
                {
                  NS_FATAL_ERROR ("Unable to find RlcPdcList in HARQ buffer for RNTI " << m_dlInfoListBuffered.at (i).m_rnti);
                }
              for (uint16_t k = 0; k < (*itRlcPdu).second.size (); k++)
                {
                  (*itRlcPdu).second.at (k).at (harqId).clear ();
                }
              continue;
            }
          // check the feasibility of retransmitting on the same RBGs
          // translate the DCI to Spectrum framework
          std::vector <int> dciRbg;
          uint32_t mask = 0x1;
          NS_LOG_INFO ("Original RBGs " << dci.m_rbBitmap << " rnti " << dci.m_rnti  << " RIV " << dci.m_riv);
          for (int j = 0; j < 32; j++)
            {
              if (((dci.m_rbBitmap & mask) >> j) == 1)
                {
                  //dciRbg.push_back (j);
                  NS_LOG_INFO ("\t" << j);
                }
              mask = (mask << 1);
            }
          //=================================================================================
          int N_rb_dl = m_cschedCellConfig.m_dlBandwidth;
          int N_prb = dci.m_riv/N_rb_dl + 1;
          int RB_start = dci.m_riv % N_rb_dl;
          for (int j = 0; j < N_prb; j++)
          {
            dciRbg.push_back (RB_start + j);
          }
          //dciRbg.push_back (dci.m_riv % N_rb_dl);
          //=================================================================================
          bool free = true;
          for (uint8_t j = 0; j < dciRbg.size (); j++)
            {
              if (rbgMap.at (dciRbg.at (j)) == true)
                {
                  free = false;
                  break;
                }
            }
          if (free)
            {
              // use the same RBGs for the retx
              // reserve RBGs
              for (uint8_t j = 0; j < dciRbg.size (); j++)
                {
                  rbgMap.at (dciRbg.at (j)) = true;
                  NS_LOG_INFO ("RBG " << dciRbg.at (j) << " assigned");
                  rbgAllocatedNum++;
                }

              NS_LOG_INFO (this << " Send retx in the same RBGs");
            }
          else
            {
              // find RBGs for sending HARQ retx
              uint8_t j = 0;
              uint8_t counter = 0;
              uint8_t rbgId = (dciRbg.at (dciRbg.size () - 1) + 1) % rbgNum;
              uint8_t startRbg = dciRbg.at (dciRbg.size () - 1);
              std::vector <bool> rbgMapCopy = rbgMap;
              while ((j < dciRbg.size ())&&(startRbg != rbgId))
                {
                  if (rbgMapCopy.at (rbgId) == false)
                    {
                      rbgMapCopy.at (rbgId) = true;
                      dciRbg.at (j) = rbgId;
                      j++;
                    }
                  rbgId = (rbgId + 1) % rbgNum;
                }
              if (j == dciRbg.size ())
                {
                  // find new RBGs -> update DCI map
                  uint32_t rbgMask = 0;
                  uint32_t riv = 0;
                  for (uint16_t k = 0; k < dciRbg.size (); k++)
                    {
                      rbgMask = rbgMask + (0x1 << dciRbg.at (k));
                      NS_LOG_INFO (this << " New allocated RBG " << dciRbg.at (k));
                      rbgAllocatedNum++;
                      counter++;
                    }
                  dci.m_rbBitmap = rbgMask;
                  //=================================================================================
                  int rb_start = dciRbg.at (0);
                  int N_rb_dl = m_cschedCellConfig.m_dlBandwidth;
                  int N_prb = counter;

                  if((N_prb-1) <= (N_rb_dl/2))
                  {
                    riv = N_rb_dl*(N_prb-1) + rb_start;
#ifdef DCI_BW_DEBUG
                    std::cout << "[RfFF:dci]riv: " << riv << " N_prb:" << N_prb << " N_rb_dl:" << N_rb_dl << " rb_start" << rb_start << "\n";
#endif
                  }
                  else
                  {
                    riv = N_rb_dl*(N_rb_dl-N_prb+1) + (N_rb_dl - 1 - rb_start);
                  }
                  dci.m_riv = riv;
                  NS_ASSERT(0);
                  //=================================================================================
                  rbgMap = rbgMapCopy;
                }
              else
                {
                  // HARQ retx cannot be performed on this TTI -> store it
                  dlInfoListUntxed.push_back (m_dlInfoListBuffered.at (i));
                  NS_LOG_INFO (this << " No resource for this retx -> buffer it");
                }
            }
          // retrieve RLC PDU list for retx TBsize and update DCI
          BuildDataListElement_s newEl;
          std::map <uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =  m_dlHarqProcessesRlcPduListBuffer.find (rnti);
          if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RlcPdcList in HARQ buffer for RNTI " << rnti);
            }
          for (uint8_t j = 0; j < nLayers; j++)
            {
              if (retx.at (j))
                {
                  if (j >= dci.m_ndi.size ())
                    {
                      // for avoiding errors in MIMO transient phases
                      dci.m_ndi.push_back (0);
                      dci.m_rv.push_back (0);
                      dci.m_mcs.push_back (0);
                      dci.m_tbsSize.push_back (0);
                      NS_LOG_INFO (this << " layer " << (uint16_t)j << " no txed (MIMO transition)");

                    }
                  else
                    {
                      dci.m_ndi.at (j) = 0;
                      dci.m_rv.at (j)++;
                      (*itHarq).second.at (harqId).m_rv.at (j)++;
                      NS_LOG_INFO (this << " layer " << (uint16_t)j << " RV " << (uint16_t)dci.m_rv.at (j));
                    }
                }
              else
                {
                  // empty TB of layer j
                  dci.m_ndi.at (j) = 0;
                  dci.m_rv.at (j) = 0;
                  dci.m_mcs.at (j) = 0;
                  dci.m_tbsSize.at (j) = 0;
                  NS_LOG_INFO (this << " layer " << (uint16_t)j << " no retx");
                }
            }

          for (uint16_t k = 0; k < (*itRlcPdu).second.at (0).at (dci.m_harqProcess).size (); k++)
            {
              std::vector <struct RlcPduListElement_s> rlcPduListPerLc;
              for (uint8_t j = 0; j < nLayers; j++)
                {
                  if (retx.at (j))
                    {
                      if (j < dci.m_ndi.size ())
                        {
                          NS_LOG_INFO (" layer " << (uint16_t)j << " tb size " << dci.m_tbsSize.at (j));
                          rlcPduListPerLc.push_back ((*itRlcPdu).second.at (j).at (dci.m_harqProcess).at (k));
                        }
                    }
                  else
                    { // if no retx needed on layer j, push an RlcPduListElement_s object with m_size=0 to keep the size of rlcPduListPerLc vector = 2 in case of MIMO
                      NS_LOG_INFO (" layer " << (uint16_t)j << " tb size "<<dci.m_tbsSize.at (j));
                      RlcPduListElement_s emptyElement;
                      emptyElement.m_logicalChannelIdentity = (*itRlcPdu).second.at (j).at (dci.m_harqProcess).at (k).m_logicalChannelIdentity;
                      emptyElement.m_size = 0;
                      rlcPduListPerLc.push_back (emptyElement);
                    }
                }

              if (rlcPduListPerLc.size () > 0)
                {
                  newEl.m_rlcPduList.push_back (rlcPduListPerLc);
                }
            }
          newEl.m_rnti = rnti;
          newEl.m_dci = dci;
          (*itHarq).second.at (harqId).m_rv = dci.m_rv;
          // refresh timer
          std::map <uint16_t, DlHarqProcessesTimer_t>::iterator itHarqTimer = m_dlHarqProcessesTimer.find (rnti);
          if (itHarqTimer== m_dlHarqProcessesTimer.end ())
            {
              NS_FATAL_ERROR ("Unable to find HARQ timer for RNTI " << (uint16_t)rnti);
            }
          (*itHarqTimer).second.at (harqId) = 0;
          ret.m_buildDataList.push_back (newEl);
          rntiAllocated.insert (rnti);
        }
      else
        {
          // update HARQ process status
          NS_LOG_INFO (this << " HARQ ACK UE " << m_dlInfoListBuffered.at (i).m_rnti);
          std::map <uint16_t, DlHarqProcessesStatus_t>::iterator it = m_dlHarqProcessesStatus.find (m_dlInfoListBuffered.at (i).m_rnti);
          if (it == m_dlHarqProcessesStatus.end ())
            {
              NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << m_dlInfoListBuffered.at (i).m_rnti);
            }
          (*it).second.at (m_dlInfoListBuffered.at (i).m_harqProcessId) = 0;
          std::map <uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =  m_dlHarqProcessesRlcPduListBuffer.find (m_dlInfoListBuffered.at (i).m_rnti);
          if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RlcPdcList in HARQ buffer for RNTI " << m_dlInfoListBuffered.at (i).m_rnti);
            }
          for (uint16_t k = 0; k < (*itRlcPdu).second.size (); k++)
            {
              (*itRlcPdu).second.at (k).at (m_dlInfoListBuffered.at (i).m_harqProcessId).clear ();
            }
        }
    }
  m_dlInfoListBuffered.clear ();
  m_dlInfoListBuffered = dlInfoListUntxed;

  if (rbgAllocatedNum == rbgNum)
    {
      // all the RBGs are already allocated -> exit
      if ((ret.m_buildDataList.size () > 0) || (ret.m_buildRarList.size () > 0))
        {
          m_schedSapUser->SchedDlConfigInd (ret);
        }
      return;
    }

  // Get the actual active flows (queue!=0)
  std::list<FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it;
  m_rlcBufferReq.sort (SortRlcBufferReq);
  int nflows = 0;
  int nTbs = 0;
  std::map <uint16_t,uint8_t> lcActivesPerRnti; // tracks how many active LCs per RNTI there are
  std::map <uint16_t,uint8_t>::iterator itLcRnti;
  for (it = m_rlcBufferReq.begin (); it != m_rlcBufferReq.end (); it++)
    {
      // remove old entries of this UE-LC
      std::set <uint16_t>::iterator itRnti = rntiAllocated.find ((*it).m_rnti);
      if ( (((*it).m_rlcTransmissionQueueSize > 0)
            || ((*it).m_rlcRetransmissionQueueSize > 0)
            || ((*it).m_rlcStatusPduSize > 0))
           && (itRnti == rntiAllocated.end ())  // UE must not be allocated for HARQ retx
           && (HarqProcessAvailability ((*it).m_rnti))  ) // UE needs HARQ proc free

        {
          //NS_LOG_LOGIC (this << " User " << (*it).m_rnti << " LC " << (uint16_t)(*it).m_logicalChannelIdentity << " is active, status  " << (*it).m_rlcStatusPduSize << " retx " << (*it).m_rlcRetransmissionQueueSize << " tx " << (*it).m_rlcTransmissionQueueSize);
          std::map <uint16_t,uint8_t>::iterator itCqi = m_p10CqiRxed.find ((*it).m_rnti);
          uint8_t cqi = 0;
          if (itCqi != m_p10CqiRxed.end ())
            {
              cqi = (*itCqi).second;
            }
          else
            {
              cqi = 1; // lowest value fro trying a transmission
            }
          if (cqi != 0)
            {
              // CQI == 0 means "out of range" (see table 7.2.3-1 of 36.213)
              nflows++;
              itLcRnti = lcActivesPerRnti.find ((*it).m_rnti);
              if (itLcRnti != lcActivesPerRnti.end ())
                {
                  (*itLcRnti).second++;
                }
              else
                {
                  lcActivesPerRnti.insert (std::pair<uint16_t, uint8_t > ((*it).m_rnti, 1));
                  nTbs++;
                }

            }
        }
    }

  if (nflows == 0)
    {
      if ((ret.m_buildDataList.size () > 0) || (ret.m_buildRarList.size () > 0))
        {
          m_schedSapUser->SchedDlConfigInd (ret);
        }
      return;
    }


  if(dlSchedSubframeNo % MODNO != 0)
    return;
  // Divide the resource equally among the active users according to
  // Resource allocation type 0 (see sec 7.1.6.1 of 36.213)

  int rbgPerTb = (nTbs > 0) ? ((rbgNum - rbgAllocatedNum) / nTbs) : INT_MAX;
  if (rbgPerTb == 0)
    {
      rbgPerTb = 1;                // at least 1 rbg per TB (till available resource)
    }
  if (rbgPerTb > 1)
    {
      rbgPerTb = 1;                // at most 1 rbg per TB (till available resource)
    }
  NS_LOG_INFO (this << " nTbs " << nTbs << " rbgNum " << rbgNum << " rbgAllocatedNum " << rbgAllocatedNum);
  NS_LOG_INFO (this << " Flows to be transmitted " << nflows << " rbgPerTb " << rbgPerTb);

  int rbgAllocated = 0;

  // round robin assignment to all UEs registered starting from the subsequent of the one
  // served last scheduling trigger event
  if (m_nextRntiDl != 0)
    {
      NS_LOG_DEBUG ("Start from the successive of " << (uint16_t) m_nextRntiDl);
      for (it = m_rlcBufferReq.begin (); it != m_rlcBufferReq.end (); it++)
        {
          if ((*it).m_rnti == m_nextRntiDl)
            {
              // select the next RNTI to starting
              it++;
              if (it == m_rlcBufferReq.end ())
              {
                it = m_rlcBufferReq.begin ();
              }
              m_nextRntiDl = (*it).m_rnti;
              break;
            }
        }

      if (it == m_rlcBufferReq.end ())
        {
          NS_LOG_ERROR (this << " no user found");
        }
    }
  else
    {
      it = m_rlcBufferReq.begin ();
      m_nextRntiDl = (*it).m_rnti;
    }
  std::map <uint16_t,uint8_t>::iterator itTxMode;
  do
    {
      itLcRnti = lcActivesPerRnti.find ((*it).m_rnti);
      std::set <uint16_t>::iterator itRnti = rntiAllocated.find ((*it).m_rnti);
      if ((itLcRnti == lcActivesPerRnti.end ())||(itRnti != rntiAllocated.end ()))
        {
          // skip this RNTI (no active queue or yet allocated for HARQ)
          uint16_t rntiDiscared = (*it).m_rnti;
          while (it != m_rlcBufferReq.end ())
            {
              if ((*it).m_rnti != rntiDiscared)
                {
                  break;
                }
              it++;
            }
          if (it == m_rlcBufferReq.end ())
            {
              // restart from the first
              it = m_rlcBufferReq.begin ();
            }
          continue;
        }
      itTxMode = m_uesTxMode.find ((*it).m_rnti);
      if (itTxMode == m_uesTxMode.end ())
        {
          NS_FATAL_ERROR ("No Transmission Mode info on user " << (*it).m_rnti);
        }
      int nLayer = TransmissionModesLayers::TxMode2LayerNum ((*itTxMode).second);
      int lcNum = (*itLcRnti).second;
      // create new BuildDataListElement_s for this RNTI
      BuildDataListElement_s newEl;
      newEl.m_rnti = (*it).m_rnti;
      // create the DlDciListElement_s
      DlDciListElement_s newDci;
      newDci.m_rnti = (*it).m_rnti;
      newDci.m_harqProcess = UpdateHarqProcessId ((*it).m_rnti);
      newDci.m_resAlloc = 0;
      newDci.m_rbBitmap = 0;
      newDci.m_riv = 0;
      std::map <uint16_t,uint8_t>::iterator itCqi = m_p10CqiRxed.find (newEl.m_rnti);
      for (uint8_t i = 0; i < nLayer; i++)
        {
          if (itCqi == m_p10CqiRxed.end ())
            {
              newDci.m_mcs.push_back (0); // no info on this user -> lowest MCS
            }
          else
            {
              newDci.m_mcs.push_back ( m_amc->GetMcsFromCqi ((*itCqi).second) );
            }
        }
      int tbSize = (m_amc->GetTbSizeFromMcs (newDci.m_mcs.at (0), rbgPerTb * rbgSize) / 8);
      uint16_t rlcPduSize = tbSize / lcNum;
      while ((*it).m_rnti == newEl.m_rnti)
        {
          if ( ((*it).m_rlcTransmissionQueueSize > 0)
               || ((*it).m_rlcRetransmissionQueueSize > 0)
               || ((*it).m_rlcStatusPduSize > 0) )
            {
              std::vector <struct RlcPduListElement_s> newRlcPduLe;
              for (uint8_t j = 0; j < nLayer; j++)
                {
                  RlcPduListElement_s newRlcEl;
                  newRlcEl.m_logicalChannelIdentity = (*it).m_logicalChannelIdentity;
                  NS_LOG_INFO (this << "LCID " << (uint32_t) newRlcEl.m_logicalChannelIdentity << " size " << rlcPduSize << " ID " << (*it).m_rnti << " layer " << (uint16_t)j);
                  newRlcEl.m_size = rlcPduSize;
                  UpdateDlRlcBufferInfo ((*it).m_rnti, newRlcEl.m_logicalChannelIdentity, rlcPduSize);
                  newRlcPduLe.push_back (newRlcEl);

                  if (m_harqOn == true)
                    {
                      // store RLC PDU list for HARQ
                      std::map <uint16_t, DlHarqRlcPduListBuffer_t>::iterator itRlcPdu =  m_dlHarqProcessesRlcPduListBuffer.find ((*it).m_rnti);
                      if (itRlcPdu == m_dlHarqProcessesRlcPduListBuffer.end ())
                        {
                          NS_FATAL_ERROR ("Unable to find RlcPdcList in HARQ buffer for RNTI " << (*it).m_rnti);
                        }
                      (*itRlcPdu).second.at (j).at (newDci.m_harqProcess).push_back (newRlcEl);
                    }

                }
              newEl.m_rlcPduList.push_back (newRlcPduLe);
              lcNum--;
            }
          it++;
          if (it == m_rlcBufferReq.end ())
            {
              // restart from the first
              it = m_rlcBufferReq.begin ();
              break;
            }
        }
      uint32_t rbgMask = 0;
      uint32_t riv = 0;
      uint16_t i = 0;
      bool firstTime = false;
      int firstRbgAllocated = 0;
      NS_LOG_INFO (this << " DL - Allocate user " << newEl.m_rnti << " LCs " << (uint16_t)(*itLcRnti).second << " bytes " << tbSize << " mcs " << (uint16_t) newDci.m_mcs.at (0) << " harqId " << (uint16_t)newDci.m_harqProcess <<  " layers " << nLayer);
      NS_LOG_INFO ("RBG:");
      while (i < rbgPerTb)
        {
          if (rbgMap.at (rbgAllocated) == false)
            {
              rbgMask = rbgMask + (0x1 << rbgAllocated);
              //riv = rbgAllocated;
              if(!firstTime)
              {
                firstRbgAllocated = rbgAllocated;
                firstTime = true;
              }
              NS_LOG_INFO ("\t " << rbgAllocated);
              i++;
              rbgMap.at (rbgAllocated) = true;
              rbgAllocatedNum++;
            }
          rbgAllocated++;
        }
      newDci.m_rbBitmap = rbgMask; // (32 bit bitmap see 7.1.6 of 36.213)
      //=================================================================================
      int rb_start = firstRbgAllocated;
      int N_rb_dl = m_cschedCellConfig.m_dlBandwidth;
      int N_prb = rbgPerTb;
      newDci.m_repetitionNumber = 1;//repetitionCounter++;
      if((N_prb - 1) <= (N_rb_dl/2))
      {
        riv = N_rb_dl * (N_prb - 1) + rb_start;
#ifdef DCI_BW_DEBUG
        std::cout << "[RfFF:newDci]riv: " << riv << " N_prb:" << N_prb << " N_rb_dl:" << N_rb_dl << " rb_start:" << rb_start << " m_repetitionNumber " << newDci.m_repetitionNumber << "\n";
#endif
      }
      else
      {
        riv = N_rb_dl * (N_rb_dl - N_prb + 1) + (N_rb_dl - 1 - rb_start);
#ifdef DCI_BW_DEBUG
        std::cout << "[RfFF:newDci]riv: " << riv << "\n";
#endif
      }
      newDci.m_riv = riv;
      //=================================================================================

      for (int i = 0; i < nLayer; i++)
        {
          newDci.m_tbsSize.push_back (tbSize);
          newDci.m_ndi.push_back (1);
          newDci.m_rv.push_back (0);
        }

      newDci.m_tpc = 1; //1 is mapped to 0 in Accumulated Mode and to -1 in Absolute Mode

      newEl.m_dci = newDci;
      if (m_harqOn == true)
        {
          // store DCI for HARQ
          std::map <uint16_t, DlHarqProcessesDciBuffer_t>::iterator itDci = m_dlHarqProcessesDciBuffer.find (newEl.m_rnti);
          if (itDci == m_dlHarqProcessesDciBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RNTI entry in DCI HARQ buffer for RNTI " << newEl.m_rnti);
            }
          (*itDci).second.at (newDci.m_harqProcess) = newDci;
          // refresh timer
          std::map <uint16_t, DlHarqProcessesTimer_t>::iterator itHarqTimer =  m_dlHarqProcessesTimer.find (newEl.m_rnti);
          if (itHarqTimer== m_dlHarqProcessesTimer.end ())
            {
              NS_FATAL_ERROR ("Unable to find HARQ timer for RNTI " << (uint16_t)newEl.m_rnti);
            }
          (*itHarqTimer).second.at (newDci.m_harqProcess) = 0;
        }
      // ...more parameters -> ignored in this version

      ret.m_buildDataList.push_back (newEl);
      if (rbgAllocatedNum == rbgNum)
        {
          m_nextRntiDl = newEl.m_rnti; // store last RNTI served
          break;                       // no more RGB to be allocated
        }
    }
  while ((*it).m_rnti != m_nextRntiDl);

  ret.m_nrOfPdcchOfdmSymbols = 1;   /// \todo check correct value according the DCIs txed  

  m_schedSapUser->SchedDlConfigInd (ret);
  return;
}

void
RrFfMacScheduler::DoSchedDlRachInfoReq (const struct FfMacSchedSapProvider::SchedDlRachInfoReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  
  m_rachList = params.m_rachList;

  return;
}

void
RrFfMacScheduler::DoSchedDlCqiInfoReq (const struct FfMacSchedSapProvider::SchedDlCqiInfoReqParameters& params)
{
  //NS_LOG_FUNCTION (this);

  std::map <uint16_t,uint8_t>::iterator it;
  for (unsigned int i = 0; i < params.m_cqiList.size (); i++)
    {
      if ( params.m_cqiList.at (i).m_cqiType == CqiListElement_s::P10 )
        {
          //NS_LOG_LOGIC ("wideband CQI " <<  (uint32_t) params.m_cqiList.at (i).m_wbCqi.at (0) << " reported");
          std::map <uint16_t,uint8_t>::iterator it;
          uint16_t rnti = params.m_cqiList.at (i).m_rnti;
          it = m_p10CqiRxed.find (rnti);
          if (it == m_p10CqiRxed.end ())
            {
              // create the new entry
              m_p10CqiRxed.insert ( std::pair<uint16_t, uint8_t > (rnti, params.m_cqiList.at (i).m_wbCqi.at (0)) ); // only codeword 0 at this stage (SISO)
              // generate correspondent timer
              m_p10CqiTimers.insert ( std::pair<uint16_t, uint32_t > (rnti, m_cqiTimersThreshold));
            }
          else
            {
              // update the CQI value
              (*it).second = params.m_cqiList.at (i).m_wbCqi.at (0);
              //std::cout << "params.m_cqiList.at (i).m_wbCqi.at (0):" << (uint16_t)params.m_cqiList.at (i).m_wbCqi.at (0) << "\n";
              // update correspondent timer
              std::map <uint16_t,uint32_t>::iterator itTimers;
              itTimers = m_p10CqiTimers.find (rnti);
              (*itTimers).second = m_cqiTimersThreshold;
            }
        }
      else if ( params.m_cqiList.at (i).m_cqiType == CqiListElement_s::A30 )
        {
          // subband CQI reporting high layer configured
          // Not used by RR Scheduler
        }
      else
        {
          NS_LOG_ERROR (this << " CQI type unknown");
        }
    }

  return;
}

void
RrFfMacScheduler::DoSchedUlTriggerReq (const struct FfMacSchedSapProvider::SchedUlTriggerReqParameters& params)
{
  uint32_t ulSchedSubframeNo = 0xF & params.m_sfnSf;
  NS_LOG_FUNCTION (this << " UL - Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf) << " size " << params.m_ulInfoList.size ());

//======================DEBUG FRAME AND SUBFRAME AT SCHEDULER=========================================

  // Refer top of this file for new variables
  //std::cout << Now().GetSeconds () << "  UL - Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf) << "\n";
  RefreshUlCqiMaps ();

  // Generate RBs map
  FfMacSchedSapUser::SchedUlConfigIndParameters ret;
  std::vector <bool> rbMap;
  uint16_t rbAllocatedNum = 0;
  //uint16_t rbAllocated = 0;
  std::set <uint16_t> rntiAllocated;
  std::vector <uint16_t> rbgAllocationMap;
  // update with RACH allocation map
  rbgAllocationMap = m_rachAllocationMap;
  //rbgAllocationMap.resize (m_cschedCellConfig.m_ulBandwidth, 0);
  m_rachAllocationMap.clear ();
  m_rachAllocationMap.resize (m_cschedCellConfig.m_ulBandwidth, 0);

  rbMap.resize (m_cschedCellConfig.m_ulBandwidth, false);
  rbMap = m_ffrSapProvider->GetAvailableUlRbg ();
  for (std::vector<bool>::iterator it = rbMap.begin (); it != rbMap.end (); it++)
  {
    if ((*it) == true )
    {
      rbAllocatedNum++;        
    }
  } 
  // remove RACH allocation

  for (uint16_t i = 0; i < m_cschedCellConfig.m_ulBandwidth; i++)
  {
    if (rbgAllocationMap.at (i) != 0)
    {
      rbMap.at (i) = true;
      NS_LOG_LOGIC(this << " Allocated for RACH " << i);
      scleft.at(i)=0;

        //std::cout << this << " Allocated for RACH " << i << "\n";
    }

  //======================AVOID ALLOCATING MULTI TONE RB IN NEXT SUBFRAME (sfleft remembers the #subframes to wait===========================================================================

    else  if (sfleft.at(i)>0)
      {rbMap.at (i) = true;
       scleft.at(rbMap.at(i))=0;

            //std::cout << "EXTENDED " << i << " sfleft " << sfleft.at(i) << " rb allocated to " << rbgAllocationMap.at (i)  << "\n";
       sfleft.at(i)--;
     }
     else if (!REPEAT && i < m_cschedCellConfig.m_ulBandwidth)
     {
     	scleft.at(rbMap.at(i))=12;
      scstart.at(rbMap.at(i))=0;
    }

    //==========================================================================================================================================================================================

  }


 //======================ANCHOR CARRIER AVOIDANCE===============================================================================================================================================

    //Anchor PRB for different bandwidths. These are the RB's which cannot be used for Uplink data transmission and need to be skipped
  std::vector <uint16_t> BW100 =  {4, 9, 14, 19, 24, 29, 34,39, 44, 55, 60, 65, 70, 75,80, 85, 90, 95};
  std::vector <uint16_t> BW75  =  {2, 7, 12, 17, 22,27, 32, 42, 47,52, 57, 62, 67, 72};
  std::vector <uint16_t> BW50  =  {4, 9, 14, 19,30, 35, 40, 45};
  std::vector <uint16_t> BW25  =  {2, 7, 17,22};
  std::vector <uint16_t> BW15  =  {2, 12};

  std::vector<uint16_t> ::iterator start;
  std::vector<uint16_t> ::iterator  end;

  switch (m_cschedCellConfig.m_ulBandwidth)
  {
    case 15: start = BW15.begin (); end = BW15.end(); break;
    case 25: start = BW25.begin (); end = BW25.end(); break;
    case 50: start = BW50.begin (); end = BW50.end(); break;
    case 75: start = BW75.begin (); end = BW75.end(); break;
    case 100: start = BW100.begin (); end = BW100.end(); break;
    default : std::cout << "BW unsupported NB-IOT";break;
  }

  for (std::vector<uint16_t>::iterator it= start;it != end; it++ )
  {
    rbMap.at(*it-1) = true ;
    scleft.at(*it-1)=0;
    Rnti.at(*it-1)=60000;
  }

//================================================================================================================================================================================================

//======================HARQ=======================================================================================================================================================================
  if (m_harqOn == true)
  {
      //   Process UL HARQ feedback
 //std::cout << "Entered HARQ check\n";
    for (uint16_t i = 0; i < params.m_ulInfoList.size (); i++)
    {
      if (params.m_ulInfoList.at (i).m_receptionStatus == UlInfoListElement_s::NotOk)
      {

             //if(ulSchedSubframeNo % MODNO != 0)
              //  return;

              // retx correspondent block: retrieve the UL-DCI
        uint16_t rnti = params.m_ulInfoList.at (i).m_rnti;
        std::cout << Now().GetSeconds() << " SF " << ulSchedSubframeNo << " RNTI FOR HARQ " << params.m_ulInfoList.at (i).m_rnti << "\n";
        std::map <uint16_t, uint8_t>::iterator itProcId = m_ulHarqCurrentProcessId.find (rnti);
        if (itProcId == m_ulHarqCurrentProcessId.end ())
        {
          NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << rnti);
        }
        uint8_t harqId = (uint8_t)((*itProcId).second - HARQ_PERIOD) % HARQ_PROC_NUM;
        NS_LOG_INFO (this << " UL-HARQ retx RNTI " << rnti << " harqId " << (uint16_t)harqId);
        std::map <uint16_t, UlHarqProcessesDciBuffer_t>::iterator itHarq = m_ulHarqProcessesDciBuffer.find (rnti);
        if (itHarq == m_ulHarqProcessesDciBuffer.end ())
        {
          NS_LOG_ERROR ("No info find in UL-HARQ buffer for UE (might change eNB) " << rnti);
        }
        UlDciListElement_s dci = (*itHarq).second.at (harqId);
        std::map <uint16_t, UlHarqProcessesStatus_t>::iterator itStat = m_ulHarqProcessesStatus.find (rnti);
        if (itStat == m_ulHarqProcessesStatus.end ())
        {
          NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << rnti);
        }
        if ((*itStat).second.at (harqId) > 3)
        {
          NS_LOG_INFO ("Max number of retransmissions reached (UL)-> drop process");
          std::cout << "Cannot allocate retx due to RACH allocations for UE\n";
          continue;
        }
        bool free = true;
        dci.m_rnti=params.m_ulInfoList.at (i).m_rnti;
        dci.m_rbStart = RBno.at(dci.m_rnti);
        dci.m_rbLen=1;
        std::cout << "retx on the same RBs " << dci.m_rnti <<  " RB " << (int) RBno.at(dci.m_rnti) <<"\n";
        for (int j = RBno.at(dci.m_rnti); j < RBno.at(dci.m_rnti) + dci.m_rbLen; j++)
        {
          if (rbMap.at (j) == true)
          {
            free = false;
            NS_LOG_INFO (this << " BUSY " << j);
          }
        }
        if (free)
        {
                  // retx on the same RBs
          for (int j = RBno.at(dci.m_rnti); j < RBno.at(dci.m_rnti) + dci.m_rbLen; j++)
          {
            rbMap.at (j) = true;
            scleft.at(i) =0;
                      //sfleft.at(dci.m_rbStart)=8;
            Rnti.at(j)= params.m_ulInfoList.at (i).m_rnti;;
            rbgAllocationMap.at (j) = params.m_ulInfoList.at (i).m_rnti;
            NS_LOG_INFO ("\tRB " << j);
            rbAllocatedNum++;
          }
          NS_LOG_INFO (this << " Send retx in the same RBGs " << (uint16_t)dci.m_rbStart << " to " << dci.m_rbStart + dci.m_rbLen << " RV " << (*itStat).second.at (harqId) + 1);
        }
        else
        {
          NS_LOG_INFO ("Cannot allocate retx due to RACH allocations for UE " << rnti);
          std::cout << "Cannot allocate retx due to RACH allocations for UE\n";
          continue;
        }
              //=================================================================================
        int rb_start = RBno.at(dci.m_rnti);
        int N_rb_dl = m_cschedCellConfig.m_ulBandwidth;
        int N_prb = 1;
        uint32_t riv = 0;
              dci.m_repetitionNumber = 1;//repetitionCounter++;
              if((N_prb - 1) <= (N_rb_dl/2))
              {
                riv = N_rb_dl * (N_prb - 1) + rb_start;
              }
              else
              {
                riv = N_rb_dl * (N_rb_dl - N_prb + 1) + (N_rb_dl - 1 - rb_start);

              }
              dci.m_riv = riv;
              dci.m_rar = 0;
                // retx on the same RBs
              
              //NS_ASSERT(0);
              //=================================================================================
              dci.m_ndi = 0;

              dci.m_scindication = 18;
              // Update HARQ buffers with new HarqId
              (*itStat).second.at ((*itProcId).second) = (*itStat).second.at (harqId) + 1;
              (*itStat).second.at (harqId) = 0;
              (*itHarq).second.at ((*itProcId).second) = dci;
              ret.m_dciList.push_back (dci);

              rntiAllocated.insert (dci.m_rnti);

            }
          }
        }
//================================================================================================================================================================================================

//======================FIND FREE RB AND FIND NEXT RNTI===========================================================================================================================================
  std::map <uint16_t,uint32_t>::iterator it;
  int nflows = 0;

  for (it = m_ceBsrRxed.begin (); it != m_ceBsrRxed.end (); it++)
  {
    std::set <uint16_t>::iterator itRnti = rntiAllocated.find ((*it).first);
      // select UEs with queues not empty and not yet allocated for HARQ
      //if((*it).second != 0)
    NS_LOG_INFO (this << " UE " << (*it).first << " queue " << (*it).second);
    if (((*it).second > 0)&&(itRnti == rntiAllocated.end ()))
    {
      nflows++;
    }
  }

  if (nflows == 0)
  {
    if (ret.m_dciList.size () > 0)
    {
      m_allocationMaps.insert (std::pair <uint16_t, std::vector <uint16_t> > (params.m_sfnSf, rbgAllocationMap));
      m_schedSapUser->SchedUlConfigInd (ret);
    }
      return;  // no flows to be scheduled
  }

  if(ulSchedSubframeNo % MODNO!= 0)
  {
      if (ret.m_dciList.size () > 0)
      {
        m_allocationMaps.insert (std::pair <uint16_t, std::vector <uint16_t> > (params.m_sfnSf, rbgAllocationMap));
        m_schedSapUser->SchedUlConfigInd (ret);
      }
      return;
  }
  // Divide the remaining resources equally among the active users starting from the subsequent one served last scheduling trigger
    uint16_t rbPerFlow = (m_cschedCellConfig.m_ulBandwidth) / (nflows + rntiAllocated.size ());
  if (rbPerFlow < 3)
  {
      rbPerFlow = 1;//6;  // at least 3 rbg per flow (till available resource) to ensure TxOpportunity >= 7 bytes
  }
  else
  rbPerFlow = 1;//6

  uint16_t rbAllocated = 0;
  int findFirstFreeRb = 0;
  int i =0;
  for (std::vector<bool>::iterator it = rbMap.begin (); it != rbMap.end (); it++)
  {
    if ((*it) == false)
      break;
    else
      findFirstFreeRb++;

    i++;
  }

  for (std::vector<bool>::iterator it = rbMap.begin (); it != rbMap.end (); it++)
  {
   std::cout << *it << " ";
  }

 std::cout << "\n";


  // check availability
 rbAllocated = findFirstFreeRb;
 if (m_nextRntiUl != 0)
 {
  for (it = m_ceBsrRxed.begin (); it != m_ceBsrRxed.end (); it++)
  {
    if ((*it).first == m_nextRntiUl)
    {
      break;
    }
  }
  if (it == m_ceBsrRxed.end ())
  {
    NS_LOG_ERROR (this << " no user found");
  }
}
else
{
  it = m_ceBsrRxed.begin ();
  m_nextRntiUl = (*it).first;
}
uint32_t min = 10000;
     //NEXT RNTI UL - USEFUL TO TRACK ROUND ROBIN CYCLE - Next RNTI to start round robin from
    //std::cout << " m_nextRntiUl " << m_nextRntiUl << "\n\n";
NS_LOG_INFO (this << " NFlows " << nflows << " RB per Flow " << rbPerFlow);
do
{
 uint32_t n_times = 1;
 uint32_t n_times_tone = 1;
 uint32_t M=12;
 int tonesneeded;
 std::set <uint16_t>::iterator itRnti = rntiAllocated.find ((*it).first);
       // uint32_t m_srsSubframeOffset = SRS.at (((*it).first))
       //  if (( Now().GetMilliSeconds() % (640) )== m_srsSubframeOffset)
       //      {
              
       //        std::cout <<  Simulator::Now().GetSeconds() << " sending SRS scheduler\n";
             
       //      }
 

 if ((itRnti != rntiAllocated.end ())||((*it).second == 0))
 {
          // UE already allocated for UL-HARQ -> skip it
  if(*itRnti!=0)
    std::cout << "UE allocated for HARQ " << *itRnti << "\n";
  it++;
  if (it == m_ceBsrRxed.end ())
  {
   // restart from the first
    it = m_ceBsrRxed.begin ();
  }
  continue;
}
if (rbAllocated + rbPerFlow - 1 > m_cschedCellConfig.m_ulBandwidth)
{
  // limit to physical resources last resource assignment
  rbPerFlow = m_cschedCellConfig.m_ulBandwidth - rbAllocated;// at least 3 rbg per flow to ensure TxOpportunity >= 7 bytes
  if (rbPerFlow < 3)
  {
              // terminate allocation
    rbPerFlow = 1;
  }
  else
    rbPerFlow = 1;

}
NS_LOG_INFO (this << " try to allocate " << (*it).first);
      //std::cout << " try to allocate " << (*it).first <<"\n";
UlDciListElement_s uldci;
uldci.m_rnti = (*it).first;
uldci.m_rbLen = rbPerFlow;
bool allocated = false;
int firstRbgAllocated = 0;
bool firstTime = false;
uint32_t riv = 0;



//================================================================================================================================================================================================


//================================================HERE CHECK THE FIXED RB OF EACH RNTI============================================================================================================

if(RBno.at (uldci.m_rnti) != 200 )
{ 
 // std :: cout << "RB NO " << RBno.at (uldci.m_rnti) << "\n";
	//std :: cout << "RBMAP " << rbMap.at(RBno.at (uldci.m_rnti)) <<  " SCLEFT " << scleft.at(RBno.at (uldci.m_rnti)) <<  " RNTI USING: " << Rnti.at(RBno.at (uldci.m_rnti)) << " SF LEFT " << sfleft.at( RBno.at (uldci.m_rnti)) << " map " << rbMap.at(rbAllocated) << "\n";
	if ((rbMap.at(RBno.at (uldci.m_rnti))==true &&  scleft.at(RBno.at (uldci.m_rnti))==0 && Rnti.at(RBno.at (uldci.m_rnti))!=0 )|| (rbMap.at(RBno.at (uldci.m_rnti))==true && Rnti.at(RBno.at (uldci.m_rnti))==uldci.m_rnti))
	{

		std::cout << "RNTI " <<  (*it).first << " fixed RB " << RBno.at (uldci.m_rnti) << " in use by " << Rnti.at(RBno.at (uldci.m_rnti)) <<  "not available for " << uldci.m_rnti << "\n";
		it++;
		if (it == m_ceBsrRxed.end ())
		{
              // restart from the first
			it = m_ceBsrRxed.begin ();
		}
    
		if ((rbAllocated == m_cschedCellConfig.m_ulBandwidth) || (rbPerFlow == 0) || (rbAllocated + rbPerFlow > m_cschedCellConfig.m_ulBandwidth))
		{
	          // Stop allocation: no more PRBs
			m_nextRntiUl = (*it).first;
			break;
		} 

		continue;
	}
  else if (REPEAT && !EXHAUSTIVE && !ANALYTICAL)//|| rbMap.at(RBno.at (uldci.m_rnti))==false || Rnti.at(RBno.at (uldci.m_rnti))==0)//||Rnti.at(RBno.at (uldci.m_rnti))==0)
		{
        //Fixed RB is free, u can use it.               
			rbAllocated = RBno.at (uldci.m_rnti);
      scleft.at(rbAllocated)= 12;
			scstart.at(rbAllocated)=0;
			sfleft.at(rbAllocated)=0;

		}
    else
    {
      rbAllocated = RBno.at (uldci.m_rnti);
    }
	}

//================================================================================================================================================================================================




//==================================================================================SCHEDULING====================================================================================================

using namespace std::chrono;
  double minSinr=0 ; double minSinrLinear;
 
      //NS_LOG_INFO (this << " RB Allocated " << rbAllocated << " rbAllocatedNum " << rbAllocatedNum << " rbPerFlow " << rbPerFlow << " flows " << nflows << " m_ulBandwidth:" << (int)m_cschedCellConfig.m_ulBandwidth);
      //std::cout << this << " RB Allocated " << rbAllocated << " rbAllocatedNum " << rbAllocatedNum << " rbPerFlow " << rbPerFlow << " flows " << nflows << " m_ulBandwidth:" << (int)m_cschedCellConfig.m_ulBandwidth << "\n";
  while ((!allocated)&&((rbAllocated + rbPerFlow - m_cschedCellConfig.m_ulBandwidth) < 1) && (rbPerFlow != 0))
  {

    bool free = true;
    for (uint16_t j = rbAllocated; j < rbAllocated + rbPerFlow; j++)
    {
      if (rbMap.at (j) == true && scleft.at(j)==0)

      {

        free = false;
        break;
      }
    }
    if (free)
    {
      uldci.m_rbStart = rbAllocated;

      for (uint16_t j = rbAllocated; j < rbAllocated + rbPerFlow; j++)
      {
        rbMap.at (j) = true;
        if(!firstTime)
        {
          firstRbgAllocated = j;
          firstTime = true;
        }
      // store info on allocation for managing ul-cqi interpretation
        rbgAllocationMap.at (j) = (*it).first;
        NS_LOG_INFO ("\t " << j);
      }
      rbAllocated += rbPerFlow;
    //std::cout << this << " rbPerFlow, rbAllocated " << rbAllocated << "\n";
      allocated = true;
      break;
    }

    rbAllocated++;
          //std::cout << this << " ++ rbAllocated " << rbAllocated << "\n";
          //std::cout << " rbAllocated++" << " RB Allocated " << rbAllocated << " rbAllocatedNum " << rbAllocatedNum << " rbPerFlow " << rbPerFlow << " flows " << nflows << " m_ulBandwidth:" << (int)m_cschedCellConfig.m_ulBandwidth <<"\n";
    if (rbAllocated + rbPerFlow - 1 > m_cschedCellConfig.m_ulBandwidth)
    {
              // limit to physical resources last resource assignment
      rbPerFlow = m_cschedCellConfig.m_ulBandwidth - rbAllocated;
              // at least 3 rbg per flow to ensure TxOpportunity >= 7 bytes
      if (rbPerFlow < 3)
      {
                  // terminate allocation
        rbPerFlow = 1;                 
      }
      else
        rbPerFlow = 1;
    }
  }
  if (!allocated)
  {
          // unable to allocate new resource: finish scheduling

   if(min <  (*it).first )
    m_nextRntiUl=min;
  else
    m_nextRntiUl = (*it).first;


           //std::cout << "Not allocated " <<  uldci.m_rnti  << "\n";

  if (ret.m_dciList.size () > 0)
  {
    m_schedSapUser->SchedUlConfigInd (ret);
  }
  m_allocationMaps.insert (std::pair <uint16_t, std::vector <uint16_t> > (params.m_sfnSf, rbgAllocationMap));
  return;
}

std::map <uint16_t, std::vector <double> >::iterator itCqi = m_ueCqi.find ((*it).first);
int cqi = 0;
int rbStartTemp = 0;
std::vector <int> rbgMap;
rbgMap.push_back (0);
std::vector <bool> rbMapTemp;
rbMapTemp.resize (m_cschedCellConfig.m_ulBandwidth, false);
rbMapTemp = m_ffrSapProvider->GetAvailableUlRbg ();
for (std::vector<bool>::iterator it = rbMapTemp.begin (); it != rbMapTemp.end (); it++)
{
  if ((*it) == false )
  {
    break;
  }
  rbStartTemp++;
}
if (itCqi == m_ueCqi.end ())
{
          // no cqi info about this UE
          uldci.m_mcs = 0; // MCS 0 -> UL-AMC TBD
          NS_LOG_INFO (this << " UE does not have ULCQI " << (*it).first );
 }
 else
 {
      //===============SINR FROM SRS IS PASSED TO BLER AND COMPARED AGAINST BLERTHRESHOLD ================================

  TbStats_t tbStats;
  HarqProcessInfoList_t harqInfoList;        
  Ptr<SpectrumModel> sm = LteSpectrumValueHelper::GetSpectrumModel (21450, 100);
  Ptr<SpectrumValue> SINR100 = Create <SpectrumValue> (sm);
  for (int j=0 ;j< 100 ;j++)
      (*SINR100)[j]=0;       // take the lowest CQI value (worst RB)
  
   minSinr =  200; //(*itCqi).second.at (rbStartTemp/*uldci.m_rbStart*/);
      
  //for (uint16_t i = rbStartTemp/*uldci.m_rbStart*/; i < m_cschedCellConfig.m_ulBandwidth/*uldci.m_rbStart + uldci.m_rbLen*/; i++) - IF SRS IS USING WHOLE BW USE THIS
    if(RBno.at (uldci.m_rnti) != 200 )
      i=RBno.at (uldci.m_rnti);
    else
     i=23; //IF SRS IS USING ONLY 1 RB (HERE IT IS ANCHOR)
i=23;
  {
    if ((*itCqi).second.at (i) < minSinr)
    {
         // std::cout << " (*itCqi).second " << (*itCqi).second.at(i) << "\n";
      minSinr = (*itCqi).second.at (i);
    }
  }
  // double dummysinr[] = {0,-9, -8.75,-8.5,-8.25, -8 ,-7.75,-7.5, -7.25, -7 ,-6.75,-6.5,-6.25, -6, -5.75,-5.5, -5.25,-5 ,-4.75,-4.5, -4.25,-4 ,-3.75,-3.5};
   //minSinr = dummysinr[uldci.m_rnti];
  minSinrLinear =  std::pow (10, minSinr / 10 );
  (*SINR100)[0]=minSinrLinear;

  tbStats = LteMiErrorModel::GetTbDecodificationStats (*SINR100, rbgMap, m_amc->GetTbSizeFromMcs (12, 1) / 8, 12, harqInfoList);
  std::cout <<" RNTI " << uldci.m_rnti<< " minSinr  " << minSinr <<  " minSinrLinear "<< minSinrLinear<< " SIZE "<<  m_amc->GetTbSizeFromMcs (1, 1) / 8  <<  " BLER " << tbStats.tbler << "\n"; 
  
   // translate SINR -> cqi: WILD ACK: same as DL
  double s = log2 ( 1 + (
   std::pow (10, minSinr / 10 )  /
   ( (-std::log (5.0 * 0.00005 )) / 1.5) ));
  //minSinrLinear =  std::pow (10, minSinr / 10 );

  cqi = m_amc->GetCqiFromSpectralEfficiency (s);

  //============================================================================================
    using namespace std::chrono;
 
// Use auto keyword to avoid typing long
// type definitions to get the timepoint
// at this instant use function now()
 high_resolution_clock::time_point t1 = high_resolution_clock::now();
  if (EXHAUSTIVE)
{
int tonearray[]={1,2,4,12,48};
int timearray[]={1,2,4,8,32};
int Datalength=42*8;
 int TBS [] = {16,24,32,40,56,72,88,104,120,136,144,176,208};
 //double Threshold[]= {-4.1,-4.6,-4.1,-2.6,-2.1,-1.1,-0.6,0.9,1.9, 2.4 ,3.9, 4.4, 5.4};   
 double threshold[9]={0.62, 0.776247117, 0.87096359 , 1.230268771 ,1.548816619 ,1.737800829, 2.45470891, 2.754228703, 3.467368505};
double delay,sinr;
struct filtered
{
int mcs;
int repeat;
int tone;
double sinr;

};
filtered f[1000];
double filteredd[1000];
int k=0;

  for (int r=1; r<=128; r=r*2)
    for (int t=0;t<5;t++)
      for(int m=4;m<=12;m++)
      {
          delay = ((8.92857+r*timearray[t]*0.92857)*ceil(Datalength/(TBS[m])));
          sinr = minSinrLinear*r*tonearray[t];
          //*SINR100=sinr;
          //tbStats = LteMiErrorModel::GetTbDecodificationStats (*SINR100, rbgMap, m_amc->GetTbSizeFromMcs (m, 1) / 8, m, harqInfoList);
          if(sinr > threshold[m-4])
          {
            f[k].mcs=m;
            f[k].tone = tonearray[t];
            f[k].repeat = r;
            filteredd[k]=delay;
            f[k].sinr = sinr;
            k++;

          }

      }


int index= indexofSmallestElement(filteredd,k);
//std::cout << filteredd[index] << " mcs " <<  f[index].mcs << " tone  "<< f[index].tone<< "repeat " << f[index].repeat <<  " sinr" << f[index].sinr << "\n";

n_times_tone = f[index].tone;
n_times= f[index].repeat;
M= f[index].mcs;

}
if(ANALYTICAL)
{
int Datalength=42*8;
double delays[9];
int tones[9]={0};
double reps[9]={0};
int TBS [] = {56,72,88,104,120,136,144,176,208};//16,24,32,40,
double threshold[9]={0.62, 0.776247117, 0.87096359 , 1.230268771 ,1.548816619 ,1.737800829, 2.45470891, 2.754228703, 3.467368505};
for (int mm=0;mm<=8;mm++)
{
  int tone1=2;
  double sinr1= minSinrLinear*2.0;
  double r1= threshold[mm]/sinr1;
  if ( r1 <= 1)
        r1 =1;
else if (r1>1 && r1<=2) 
        r1=2;
else if (r1>2 && r1<=4)
        r1=4;
else if (r1>4 && r1<=8)
        r1=8;
else if (r1>8 && r1<=16)
        r1=16;    
else if (r1>16 && r1<=32)
          r1=32;
else if (r1>32 && r1<=64)
           r1=64;
else if(r1>64 && r1<=128)
        r1=128;
      else
         r1=999999;
  double delay1= ((8.92857+r1*tone1*0.92857)*ceil(Datalength/(TBS[mm])));


  int tone2=32;
  double sinr2= minSinrLinear*48.0;
  double r2= threshold[mm]/sinr2;
  if ( r2 <= 1)
        r2 =1;
else if (r2>1 && r2<=2) 
        r2=2;
else if (r2>2 && r2<=4)
        r2=4;
else if (r2>4 && r2<=8)
        r2=8;
else if (r2>8 && r2<=16)
        r2=16;    
else if (r2>16 && r2<=32)
          r2=32;
else if (r2>32 && r2<=64)
           r2=64;
else if(r2>64 && r2<=128)
        r2=128;
else
    r2=999999;

  double delay2= ((8.92857+r2*tone2*0.92857)*ceil(Datalength/(TBS[mm])));
    // if(r2>128)
    // delay2=999999.0;
  if(delay1<delay2)
  {
    delays[mm]=delay1;
    tones[mm]=tone1;
    reps[mm]=r1;
  }
  else
      {
    delays[mm]=delay2;
    tones[mm]=tone2;
    reps[mm]=r2;
  }


}
int index= indexofSmallestElement(delays,9);
//std::cout << " DELAYS " << delays [index] <<  " index " << index << "\n";
n_times_tone = tones[index];
double n_times1= reps[index];
M= index+4;


// if ( n_times1 <= 1)
//         n_times =1;
// else if (n_times1>1 && n_times1<=2) 
//         n_times=2;
// else if (n_times1>2 && n_times1<=4)
//         n_times=4;
// else if (n_times1>4 && n_times1<=8)
//         n_times=8;
// else if (n_times1>8 && n_times1<=16)
//         n_times=16;    
// else if (n_times1>16 && n_times1<=32)
//           n_times=32;
// else if (n_times1>32 && n_times1<=64)
//            n_times=64;
// else if(n_times1>64 )
//         n_times=128;
//std::cout << " RNTI " <<  uldci.m_rnti << "TONE " << tones[index]<< " REPS "<<reps[index]<<  " n times "<< n_times << " MCS "<< index+4;
}
   

using namespace std::chrono;
 
// After function call
    high_resolution_clock::time_point t2 = high_resolution_clock::now();

    auto duration = duration_cast<nanoseconds>( t2 - t1 ).count();
    //std::cout << uldci.m_rnti << " duration "<<  duration << "\n";
    ExecTime.at(uldci.m_rnti) = duration;


  //===============REPETITION ASSIGNED BY COMPARING WITH BLERTHRESHOLD ================================
  if(REPEAT && BLERBASED)
  {
    if (tbStats.tbler < TARGERBLER)
    {
      n_times = 1;

    }
    else
    {
     for(i=2; i<=128;i=i*2)
     {
      double minSinr_zero= minSinrLinear;
      minSinr_zero = minSinr_zero + (i-1)*( minSinr_zero);

      (*SINR100) = minSinr_zero;
      tbStats = LteMiErrorModel::GetTbDecodificationStats (*SINR100, rbgMap, m_amc->GetTbSizeFromMcs (12, 1) / 8, 12, harqInfoList);
      if(tbStats.tbler < TARGERBLER)
      {

        n_times = i;
            //std::cout << "BLER TARGET REACHED" << i << "sinr " << minSinr_zero << "BLER " << tbStats.tbler <<  "\n\n\n\n";                     
        break;
      }

    }
    if(i>128)
     n_times=128;


 }
}
//============================================================================================


  //===============TONE ASSIGNED BY COMPARING WITH BLERTHRESHOLD ================================
int i=1;
if(TONE && BLERBASED)
{
  if (tbStats.tbler < TARGERBLER)
  {
    n_times_tone = 1;

  }
  else
  {
   for(i=2; i < 49 ;i=i+2)
   {
    if (!(i==2||i==4||i==12||i==48))
    {
      continue;
    }
    double minSinr_zero= minSinrLinear;
    minSinr_zero = minSinr_zero + (i-1)*fabs( minSinr_zero);

    (*SINR100) = minSinr_zero;
    tbStats = LteMiErrorModel::GetTbDecodificationStats (*SINR100, rbgMap, m_amc->GetTbSizeFromMcs (12, 1) / 8, 12, harqInfoList);
    //std::cout << "BLER " << tbStats.tbler << " i " << i<<    "\n\n\n\n"; 
    if(tbStats.tbler < TARGERBLER)
      {   n_times_tone=i;
                              
        break;
      }

    }
    if(tbStats.tbler >= TARGERBLER)
     n_times_tone=48;
 }
std::cout << "RNTI " << uldci.m_rnti << "BLER TARGET REACHED" << n_times_tone << " SINR" << (*SINR100)[0]<<  "\n\n\n\n"; 
   }
//============================================================================================
if (cqi == 0 || m_amc->GetMcsFromCqi (cqi) <= 2)
{ 

//HERE CHECK IF CQI=0 CAN STILL BE ALLOCATED USING TONE OR REPETITION--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 if(REPEAT && CQIBASED)
 {

   for(i=2; i<=128;i=i*2)
   {
    double minSinr_zero= minSinrLinear;
    minSinr_zero = minSinr_zero + (i-1)*fabs( minSinr_zero);
    double s = log2 ( 1 + (
     minSinr_zero   /
     ( (-std::log (5.0 * 0.00005 )) / 1.5) ));


    cqi = m_amc->GetCqiFromSpectralEfficiency (s);
    if(cqi>0)
    {
      std::cout << "CQI changed due to 0 " << i << "sinr " << minSinr_zero << "\n\n\n\n";                  	  
      break;
    }

  }
  if(i>128)
   n_times=128;
 else
   n_times = i;
}

if(TONE && CQIBASED)
{
  std::cout << "minSinrLinear  " << minSinrLinear <<  "\n"; 
  for(i=2; i<=12 && i!=6;i=i+2)
  {
   double minSinr_zero= minSinrLinear;
   minSinr_zero = minSinr_zero + (i-1)*fabs( minSinr_zero);
   double s = log2 ( 1 + (
     minSinr_zero /
     ( (-std::log (5.0 * 0.00005 )) / 1.5) ));


   cqi = m_amc->GetCqiFromSpectralEfficiency (s);
   if(cqi>0)
   {
    std::cout << "CQI changed due to 0 " << i << "sinr " << minSinr_zero << "\n\n\n\n";                  	  
    break;
  }

}
if(i>12)
 n_times=12;
else
 n_times = i;
}

//HERE END CHECK IF CQI=0 CAN STILL BE ALLOCATED USING TONE OR REPETITION--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


 if(cqi==0 && MCS)
 {   
	
              it++;
              if (it == m_ceBsrRxed.end ())
                {
                  // restart from the first
                  it = m_ceBsrRxed.begin ();
                }
              NS_LOG_DEBUG (this << " UE discared for CQI=0, RNTI " << uldci.m_rnti);
              std::cout << " UE discared for CQI=0, RNTI " << uldci.m_rnti << " minSinr:" << minSinr << " uldci.m_rbStart:" << (int)uldci.m_rbStart << " uldci.m_rbLen:" << (int)uldci.m_rbLen <<"\n";
             
              // remove UE from allocation map
              for (uint16_t i = uldci.m_rbStart; i < uldci.m_rbStart + uldci.m_rbLen; i++)
                {
                  rbgAllocationMap.at (i) = 0;
                }
              rbAllocated -= rbPerFlow;
              rbMap.at (rbAllocated) = false;
              //std::cout << this << " -1-rbAllocated " << rbAllocated << " rbAllocatedNum " << rbAllocatedNum << " rbPerFlow " << rbPerFlow << " flows " << nflows << " m_ulBandwidth:" << (int)m_cschedCellConfig.m_ulBandwidth << "\n";
              continue; 
}         
              // CQI == 0 means "out of range" (see table 7.2.3-1 of 36.213)
}
if(cqi>0)
{
 //std::cout << "No of times inside cqi>0 "<< n_times <<"\n";

//HERE FIND NO OF REPETITIONS AND TONES BASED ON CQI-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

 if(REPEAT && CQIBASED)
 {
   int cqithreshold = CQITHRESHOLD;

   int cqi_test=cqi;
   int i=1;
   for( i=n_times; i<=128;i=i*2)
   {
    double minSinr_test= minSinrLinear;
    if(cqi_test>cqithreshold)
    {
      std::cout << "CQI target reached " << i << "\n";                  	  
      break;
    }
    if(minSinr_test>=0)
      minSinr_test = minSinr_test + (i-1)* fabs( minSinr_test);
    else
    {
      minSinr_test = minSinr_test/i;
      if(fabs(minSinr_test)<0.1)
        minSinr_test=0;

    }
    double s = log2 ( 1 + (
     minSinr_test   /
     ( (-std::log (5.0 * 0.00005 )) / 1.5) ));


    cqi_test = m_amc->GetCqiFromSpectralEfficiency (s);
    std::cout << "cqi_test " << cqi_test << " minSinr_test " << minSinr_test <<"\n";


  }
  if(cqi_test>cqithreshold)
  {
    if(i>128)
     n_times=128;
   else
     n_times =i;
 }
 else
  n_times=128;
}

if(TONE && CQIBASED)
{

  switch (cqi)
  {
    case 15:
    case 14:
    case 13:
    case 12: 
    case 11:tonesneeded=12; break;
    case 10: 
    case 9:
    case 8: tonesneeded =6; break;
    case 7: 
    case 6: tonesneeded=3; break;
    case 5: 
    case 4:
    case 3: 
    case 2: 
    case 1:
    case 0: tonesneeded=1; break;
  }

  if(scleft.at(uldci.m_rbStart)<tonesneeded)
  {
    it++;
    if (it == m_ceBsrRxed.end ())
    {
                  // restart from the first
      it = m_ceBsrRxed.begin ();
    }
                 //  for (uint16_t i = uldci.m_rbStart; i < uldci.m_rbStart + uldci.m_rbLen; i++)
                 // {
                 //  rbgAllocationMap.at (i) = 0;
                 // }
                 //    // rbAllocated -= rbPerFlow;
                     //rbMap.at (rbAllocated) = false;
                    //std::cout << this << " -1-rbAllocated " << rbAllocated << " rbAllocatedNum " << rbAllocatedNum << " rbPerFlow " << rbPerFlow << " flows " << nflows << " m_ulBandwidth:" << (int)m_cschedCellConfig.m_ulBandwidth << "\n";
    continue;
  }

}


//HERE END FIND NO OF REPETITIONS AND TONES BASED ON CQI-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
}
if(MCS)
uldci.m_mcs = m_amc->GetMcsFromCqi (cqi);
else
uldci.m_mcs = M;
        
}
if(TONE && BLERBASED || TONE && EXHAUSTIVE||TONE && ANALYTICAL)
tonesneeded = 12 / n_times_tone;

//=================================================================================
int rb_start = firstRbgAllocated;
int N_rb_dl = m_cschedCellConfig.m_ulBandwidth;
int N_prb = rbPerFlow;
      uldci.m_repetitionNumber = 1;//repetitionCounter++;
      if((N_prb - 1) <= (N_rb_dl/2))
      {
        riv = N_rb_dl * (N_prb - 1) + rb_start;

      }
      else
      {
        riv = N_rb_dl * (N_rb_dl - N_prb + 1) + (N_rb_dl - 1 - rb_start);
#ifdef DCI_BW_DEBU
        std::cout << "[RfFF:uldci]riv: " << riv << "\n";
#endif
      }
      uldci.m_riv = riv;
      // STORE FIXED RB OF EACH RNTI---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
      if(RBno.at (uldci.m_rnti) == 200 )
      {
        RBno.at (uldci.m_rnti)=riv;
      }
       // STORE FIXED RB OF EACH RNTI---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
      uldci.m_rar = 0;
      //================================================================================= 
      uldci.m_tbSize = (m_amc->GetTbSizeFromMcs (uldci.m_mcs, rbPerFlow) / 8); // MCS 0 -> UL-AMC TBD


      UpdateUlRlcBufferInfo (uldci.m_rnti, uldci.m_tbSize);
      uldci.m_ndi = 1;
      uldci.m_cceIndex = 0;
      uldci.m_aggrLevel = 1;
      uldci.m_ueTxAntennaSelection = 3; // antenna selection OFF
      uldci.m_hopping = false;
      uldci.m_n2Dmrs = 0;
      uldci.m_tpc = 0; // no power control
      uldci.m_cqiRequest = false; // only period CQI at this stage
      uldci.m_ulIndex = 0; // TDD parameter
      uldci.m_dai = 1; // TDD parameter
      uldci.m_freqHopping = 0;
      uldci.m_pdcchPowerOffset = 0; // not used


     //NEW PARAMETERS ADDED TO UL_DCI BASED ON NB-IOT (For now, we are assigning constants, later we can change dynamically)----------------------------------------------------------------------------------------------------------

      uldci.m_flag= 0; //0-> N0 format, 1-> N1 format
      uldci.m_numberofRU = 0; //No of Resource units 0 to 7. 0 means 1 RU
      uldci.m_scheddelay= 0; //Scheduling delay a number between 0 to 3, 0 means 8 ms delay
      uldci.m_rv = 0; //Redundancy version 1 or 0
      uldci.m_dcirepetitionNumber =0; //2 bits no repetition as of now for DCI


/* If the RB is completely free, allocate based on CQI*/

uldci.m_scindication = 18;
//============================================================================ALLOCATE TONES========================================================================================================
if(scstart.at(rb_start)>11)
  scstart.at(rb_start)=0;

if(TONE)
{
  //std::cout << "tonesneeded "<< tonesneeded << "rnti "<< uldci.m_rnti << "SC LEFT" << scleft.at(uldci.m_rbStart) << " RB "<< uldci.m_rbStart << " CQI "<< cqi << "\n";
  if(tonesneeded==0)
  {
    //tonesneeded=1;
      sfleft.at(rb_start)= 32;
      //std::cout << "SF LEFT AFTER TONE ASSIGNED " << sfleft.at(rb_start) << "\n";
      if(scleft.at(rb_start)>0) 
      scleft.at(rb_start) =  scleft.at(rb_start)-1;
      uldci.m_scindication = 19+scstart.at(rb_start); //Subcarrier indication between 0 to 64 6 bits; It is 18 for 12 tone 
      scstart.at(rb_start) =  scstart.at(rb_start)+1;
  }
  if(tonesneeded==1)
  {
      sfleft.at(rb_start)= 8;
      //std::cout << "SF LEFT AFTER TONE ASSIGNED " << sfleft.at(rb_start) << "\n";
      if(scleft.at(rb_start)>0) 
      scleft.at(rb_start) =  scleft.at(rb_start)-1;
      uldci.m_scindication = scstart.at(rb_start); //Subcarrier indication between 0 to 64 6 bits; It is 18 for 12 tone 
      scstart.at(rb_start) =  scstart.at(rb_start)+1;
  }
  else if (tonesneeded ==3)
  {
        sfleft.at(rb_start)= 4;
        if(scleft.at(rb_start)>0)   
         scleft.at(rb_start) =  scleft.at(rb_start)-3;
        uldci.m_scindication =  scstart.at(rb_start)/3 + 12;
           scstart.at(rb_start) = scstart.at(rb_start)+3;; 
  }
  else if (tonesneeded==6)
  {
    sfleft.at(rb_start)= 2;
     if(scleft.at(rb_start)>0) 
    scleft.at(rb_start) =  scleft.at(rb_start)-6;
    uldci.m_scindication =  scstart.at(rb_start)/6 + 16;
    scstart.at(rb_start) = scstart.at(rb_start)+6;
  }
  else if (tonesneeded==12)
  {
    sfleft.at(rb_start)= 0;
    uldci.m_scindication = 18; //Subcarrier indication between 0 to 64 6 bits; It is 18 for 12 tone
    scleft.at(rb_start) =  0;
  }

//========================================================================================================================================================================================================

}
if(REPEAT)
{

//============================================================================ALLOCATE REPETITIONS========================================================================================================

{
//std::cout << "n_times" << n_times <<"\n";
uldci.m_repetitionNumber = n_times ;

}
if(EXHAUSTIVE||ANALYTICAL)
sfleft.at(rb_start)=  sfleft.at(rb_start)*uldci.m_repetitionNumber;
else
sfleft.at(rb_start)= uldci.m_repetitionNumber-1;
uldci.m_rv = uldci.m_repetitionNumber-sfleft.at(rb_start);
uldci.m_ndi = 1;
//uldci.m_scindication = 18; //Subcarrier indication between 0 to 64 6 bits; It is 18 for 12 tone
scleft.at(rb_start) =0;
//========================================================================================================================================================================================================
}
if(!TONE && !REPEAT)
{
 //===========================================================================NO REPETITION OR TONE========================================================================================================
 
  sfleft.at(rb_start)= 0;
  uldci.m_scindication = 18; //Subcarrier indication between 0 to 64 6 bits; It is 18 for 12 tone
  scleft.at(rb_start) =  0;
}
//========================================================================================================================================================================================================= 

Rnti.at(rb_start)= uldci.m_rnti;


#ifdef DCI_BW_DEBUG
        std::cout << "[RfFF:uldci]riv: " << riv << " N_prb:" << N_prb << " N_rb_dl:" << N_rb_dl << " rb_start:" << rb_start << " m_repetitionNumber " << uldci.m_repetitionNumber <<  " RNTI " << uldci.m_rnti <<  " m_scindication " << uldci.m_scindication << "\n";
#endif


 //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

   ret.m_dciList.push_back (uldci);
      // store DCI for HARQ_PERIOD
      uint8_t harqId = 0;
      if (m_harqOn == true)
        {
          std::map <uint16_t, uint8_t>::iterator itProcId;
          itProcId = m_ulHarqCurrentProcessId.find (uldci.m_rnti);
          if (itProcId == m_ulHarqCurrentProcessId.end ())
            {
              NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << uldci.m_rnti);
            }
          harqId = (*itProcId).second;
          std::map <uint16_t, UlHarqProcessesDciBuffer_t>::iterator itDci = m_ulHarqProcessesDciBuffer.find (uldci.m_rnti);
          if (itDci == m_ulHarqProcessesDciBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RNTI entry in UL DCI HARQ buffer for RNTI " << uldci.m_rnti);
            }
          (*itDci).second.at (harqId) = uldci;
          // Update HARQ process status (RV 0)
          std::map <uint16_t, UlHarqProcessesStatus_t>::iterator itStat = m_ulHarqProcessesStatus.find (uldci.m_rnti);
          if (itStat == m_ulHarqProcessesStatus.end ())
            {
              NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << uldci.m_rnti);
            }
          (*itStat).second.at (harqId) = 0;
        }
        
      NS_LOG_INFO (this << " UL Allocation - UE " << (*it).first << " startPRB " << (uint32_t)uldci.m_rbStart << " nPRB " << (uint32_t)uldci.m_rbLen << " CQI " << cqi << " MCS " << (uint32_t)uldci.m_mcs << " TBsize " << uldci.m_tbSize << " harqId " << (uint16_t)harqId);

      it++;
      if (it == m_ceBsrRxed.end ())
        {
          // restart from the first
          it = m_ceBsrRxed.begin ();
        }
     
      if ((rbAllocated == m_cschedCellConfig.m_ulBandwidth) || (rbPerFlow == 0) || (rbAllocated + rbPerFlow > m_cschedCellConfig.m_ulBandwidth))
        {
           std::cout << "rbAllocated: " << rbAllocated << " m_ulBandwidth:" << m_cschedCellConfig.m_ulBandwidth << " rbPerFlow:" << rbPerFlow << "\n\n\n\n";

         m_nextRntiUl = (*it).first;
         break;
        }
    }
  while (((*it).first != m_nextRntiUl)&&(rbPerFlow!=0));

  m_allocationMaps.insert (std::pair <uint16_t, std::vector <uint16_t> > (params.m_sfnSf, rbgAllocationMap));
uint32_t sum=0;
for(i=0;i<100;i++)
  {
    sum = sum + ExecTime.at(i);
    std::cout << ExecTime.at(i) << " ";
}

std::cout << " sum " <<  sum;
std::cout << "\n";

  m_schedSapUser->SchedUlConfigInd (ret);
  return;
}
//================================================================================================================================================================================================


void
RrFfMacScheduler::DoSchedUlNoiseInterferenceReq (const struct FfMacSchedSapProvider::SchedUlNoiseInterferenceReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  return;
}

void
RrFfMacScheduler::DoSchedUlSrInfoReq (const struct FfMacSchedSapProvider::SchedUlSrInfoReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  return;
}

void
RrFfMacScheduler::DoSchedUlMacCtrlInfoReq (const struct FfMacSchedSapProvider::SchedUlMacCtrlInfoReqParameters& params)
{
  NS_LOG_FUNCTION (this);

  std::map <uint16_t,uint32_t>::iterator it;

  for (unsigned int i = 0; i < params.m_macCeList.size (); i++)
    {
      if ( params.m_macCeList.at (i).m_macCeType == MacCeListElement_s::BSR )
        {
          // buffer status report
          // note that this scheduler does not differentiate the
          // allocation according to which LCGs have more/less bytes
          // to send.
          // Hence the BSR of different LCGs are just summed up to get
          // a total queue size that is used for allocation purposes.

          uint32_t buffer = 0;
          for (uint8_t lcg = 0; lcg < 4; ++lcg)
            {
              uint8_t bsrId = params.m_macCeList.at (i).m_macCeValue.m_bufferStatus.at (lcg);
              buffer += BufferSizeLevelBsr::BsrId2BufferSize (bsrId);
            }

          uint16_t rnti = params.m_macCeList.at (i).m_rnti;
          it = m_ceBsrRxed.find (rnti);
          if (it == m_ceBsrRxed.end ())
            {
              // create the new entry
              m_ceBsrRxed.insert ( std::pair<uint16_t, uint32_t > (rnti, buffer));
              //if(buffer != 0)
              //  NS_LOG_INFO (this << " Insert RNTI " << rnti << " queue " << buffer);
            }
          else
            {
              // update the buffer size value
              (*it).second = buffer;
              //if(buffer != 0)
              //  NS_LOG_INFO (this << " Update RNTI " << rnti << " queue " << buffer);
            }
        }
    }

  return;
}

void
RrFfMacScheduler::DoSchedUlCqiInfoReq (const struct FfMacSchedSapProvider::SchedUlCqiInfoReqParameters& params)
{
  NS_LOG_FUNCTION (this);

  switch (m_ulCqiFilter)
    {
    case FfMacScheduler::SRS_UL_CQI:
      {
        // filter all the CQIs that are not SRS based
        if (params.m_ulCqi.m_type != UlCqi_s::SRS)
          {
            return;
          }
      }
      break;
    case FfMacScheduler::PUSCH_UL_CQI:
      {
        // filter all the CQIs that are not SRS based
        if (params.m_ulCqi.m_type != UlCqi_s::PUSCH)
          {
            return;
          }
      }
    case FfMacScheduler::ALL_UL_CQI:
      break;

    default:
      NS_FATAL_ERROR ("Unknown UL CQI type");
    }
  switch (params.m_ulCqi.m_type)
    {
    case UlCqi_s::PUSCH:
      {
        std::map <uint16_t, std::vector <uint16_t> >::iterator itMap;
        std::map <uint16_t, std::vector <double> >::iterator itCqi;
        itMap = m_allocationMaps.find (params.m_sfnSf);
        if (itMap == m_allocationMaps.end ())
          {
            NS_LOG_INFO (this << " Does not find info on allocation, size : " << m_allocationMaps.size ());
            return;
          }
        for (uint32_t i = 0; i < (*itMap).second.size (); i++)
          {
            // convert from fixed point notation Sxxxxxxxxxxx.xxx to double
            double sinr = LteFfConverter::fpS11dot3toDouble (params.m_ulCqi.m_sinr.at (i));
            itCqi = m_ueCqi.find ((*itMap).second.at (i));
            if (itCqi == m_ueCqi.end ())
              {
                // create a new entry
                std::vector <double> newCqi;
                for (uint32_t j = 0; j < m_cschedCellConfig.m_ulBandwidth; j++)
                  {
                    if (i == j)
                      {
                        newCqi.push_back (sinr);
                        //std::cout << "UlCqi_s::PUSCH1 " << sinr << " i == j = " << i  << " RNTI:" << (int)(*itMap).second.at (i) << "\n";
                      }
                    else
                      {
                        // initialize with NO_SINR value.
                        newCqi.push_back (30.0);
                        //std::cout << "UlCqi_s::PUSCH 30.0\n";
                      }

                  }
                m_ueCqi.insert (std::pair <uint16_t, std::vector <double> > ((*itMap).second.at (i), newCqi));

                //std::cout << "UlCqi_s::PUSCH1\n\n\n";
                // generate correspondent timer
                m_ueCqiTimers.insert (std::pair <uint16_t, uint32_t > ((*itMap).second.at (i), m_cqiTimersThreshold));
              }
            else
              {
                // update the value
                //(*itCqi).second.at (i) = sinr;
                //std::cout << "UlCqi_s::PUSCH2 " << sinr << " i = end =  " << i  << " RNTI:" << (int)(*itMap).second.at (i) << "\n";
                //std::cout << "UlCqi_s::PUSCH2\n\n\n";
                // update correspondent timer
                std::map <uint16_t, uint32_t>::iterator itTimers;
                itTimers = m_ueCqiTimers.find ((*itMap).second.at (i));
                (*itTimers).second = m_cqiTimersThreshold;

              }

          }
        // remove obsolete info on allocation
        m_allocationMaps.erase (itMap);
      }
      break;
    case UlCqi_s::SRS:
      {
        // get the RNTI from vendor specific parameters
        uint16_t rnti = 0;
        NS_ASSERT (params.m_vendorSpecificList.size () > 0);
        for (uint16_t i = 0; i < params.m_vendorSpecificList.size (); i++)
          {
            if (params.m_vendorSpecificList.at (i).m_type == SRS_CQI_RNTI_VSP)
              {
                Ptr<SrsCqiRntiVsp> vsp = DynamicCast<SrsCqiRntiVsp> (params.m_vendorSpecificList.at (i).m_value);
                rnti = vsp->GetRnti ();
              }
          }
        std::map <uint16_t, std::vector <double> >::iterator itCqi;
        itCqi = m_ueCqi.find (rnti);
        if (itCqi == m_ueCqi.end ())
          {
            // create a new entry
            std::vector <double> newCqi;
            for (uint32_t j = 0; j < m_cschedCellConfig.m_ulBandwidth; j++)
              {
                double sinr = LteFfConverter::fpS11dot3toDouble (params.m_ulCqi.m_sinr.at (j));
                newCqi.push_back (sinr);
                //NS_LOG_INFO (this << " RNTI " << rnti << " new SRS-CQI for RB  " << j << " value " << sinr);
                //std::cout << " RNTI " << rnti << " new SRS-CQI for RB  " << j << " value2: " << (double)sinr << " value1: " << params.m_ulCqi.m_sinr.at (j) << "\n";
              }
            m_ueCqi.insert (std::pair <uint16_t, std::vector <double> > (rnti, newCqi));
            //std::cout << "UlCqi_s::SRS1\n\n\n";
            // generate correspondent timer
            m_ueCqiTimers.insert (std::pair <uint16_t, uint32_t > (rnti, m_cqiTimersThreshold));
          }
        else
          {
            // update the values
            for (uint32_t j = 0; j < m_cschedCellConfig.m_ulBandwidth; j++)
              {
                double sinr = LteFfConverter::fpS11dot3toDouble (params.m_ulCqi.m_sinr.at (j));
                (*itCqi).second.at (j) = sinr;
                //std::cout << this << " RNTI " << rnti << " update SRS-CQI for RB  " << j << " value " << sinr;
                //std::cout << "UlCqi_s::SRS2\n\n\n";
                //NS_LOG_INFO (this << " RNTI " << rnti << " update SRS-CQI for RB  " << j << " value " << sinr);
              }
            // update correspondent timer
            std::map <uint16_t, uint32_t>::iterator itTimers;
            itTimers = m_ueCqiTimers.find (rnti);
            (*itTimers).second = m_cqiTimersThreshold;

          }


      }
      break;
    case UlCqi_s::PUCCH_1:
    case UlCqi_s::PUCCH_2:
    case UlCqi_s::PRACH:
      {
        NS_FATAL_ERROR ("PfFfMacScheduler supports only PUSCH and SRSUL-CQIs");
      }
      break;
    default:
      NS_FATAL_ERROR ("Unknown type of UL-CQI");
    }
  return;
}


void
RrFfMacScheduler::RefreshDlCqiMaps (void)
{
  //NS_LOG_FUNCTION (this << m_p10CqiTimers.size ());
  // refresh DL CQI P01 Map
  std::map <uint16_t,uint32_t>::iterator itP10 = m_p10CqiTimers.begin ();
  while (itP10 != m_p10CqiTimers.end ())
    {
      //NS_LOG_INFO (this << " P10-CQI for user " << (*itP10).first << " is " << (uint32_t)(*itP10).second << " thr " << (uint32_t)m_cqiTimersThreshold);
      if ((*itP10).second == 0)
        {
          // delete correspondent entries
          std::map <uint16_t,uint8_t>::iterator itMap = m_p10CqiRxed.find ((*itP10).first);
          NS_ASSERT_MSG (itMap != m_p10CqiRxed.end (), " Does not find CQI report for user " << (*itP10).first);
          NS_LOG_INFO (this << " P10-CQI exired for user " << (*itP10).first);
          m_p10CqiRxed.erase (itMap);
          std::map <uint16_t,uint32_t>::iterator temp = itP10;
          itP10++;
          m_p10CqiTimers.erase (temp);
        }
      else
        {
          (*itP10).second--;
          itP10++;
        }
    }

  return;
}


void
RrFfMacScheduler::RefreshUlCqiMaps (void)
{
  // refresh UL CQI  Map
  std::map <uint16_t,uint32_t>::iterator itUl = m_ueCqiTimers.begin ();
  while (itUl != m_ueCqiTimers.end ())
    {
      //NS_LOG_INFO (this << " UL-CQI for user " << (*itUl).first << " is " << (uint32_t)(*itUl).second << " thr " << (uint32_t)m_cqiTimersThreshold);
      if ((*itUl).second == 0)
        {
          // delete correspondent entries
          std::map <uint16_t, std::vector <double> >::iterator itMap = m_ueCqi.find ((*itUl).first);
          NS_ASSERT_MSG (itMap != m_ueCqi.end (), " Does not find CQI report for user " << (*itUl).first);
          NS_LOG_INFO (this << " UL-CQI exired for user " << (*itUl).first);
          (*itMap).second.clear ();
          m_ueCqi.erase (itMap);
          std::map <uint16_t,uint32_t>::iterator temp = itUl;
          itUl++;
          m_ueCqiTimers.erase (temp);
        }
      else
        {
          (*itUl).second--;
          itUl++;
        }
    }

  return;
}

void
RrFfMacScheduler::UpdateDlRlcBufferInfo (uint16_t rnti, uint8_t lcid, uint16_t size)
{
  NS_LOG_FUNCTION (this);
  std::list<FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it;
  for (it = m_rlcBufferReq.begin (); it != m_rlcBufferReq.end (); it++)
    {
      if (((*it).m_rnti == rnti) && ((*it).m_logicalChannelIdentity == lcid))
        {
          NS_LOG_INFO (this << " UE " << rnti << " LC " << (uint16_t)lcid << " txqueue " << (*it).m_rlcTransmissionQueueSize << " retxqueue " << (*it).m_rlcRetransmissionQueueSize << " status " << (*it).m_rlcStatusPduSize << " decrease " << size);
          // Update queues: RLC tx order Status, ReTx, Tx
          // Update status queue
           if (((*it).m_rlcStatusPduSize > 0) && (size >= (*it).m_rlcStatusPduSize))
              {
                (*it).m_rlcStatusPduSize = 0;
              }
            else if (((*it).m_rlcRetransmissionQueueSize > 0) && (size >= (*it).m_rlcRetransmissionQueueSize))
              {
                (*it).m_rlcRetransmissionQueueSize = 0;
              }
            else if ((*it).m_rlcTransmissionQueueSize > 0)
              {
                uint32_t rlcOverhead;
                if (lcid == 1)
                  {
                    // for SRB1 (using RLC AM) it's better to
                    // overestimate RLC overhead rather than
                    // underestimate it and risk unneeded
                    // segmentation which increases delay 
                    rlcOverhead = 4;                                  
                  }
                else
                  {
                    // minimum RLC overhead due to header
                    rlcOverhead = 2;
                  }
                // update transmission queue
                if ((*it).m_rlcTransmissionQueueSize <= size - rlcOverhead)
                  {
                    (*it).m_rlcTransmissionQueueSize = 0;
                  }
                else
                  {                    
                    (*it).m_rlcTransmissionQueueSize -= size - rlcOverhead;
                  }
              }
          return;
        }
    }
}

void
RrFfMacScheduler::UpdateUlRlcBufferInfo (uint16_t rnti, uint16_t size)
{

  size = size - 2; // remove the minimum RLC overhead
  std::map <uint16_t,uint32_t>::iterator it = m_ceBsrRxed.find (rnti);
  if (it != m_ceBsrRxed.end ())
    {
      NS_LOG_INFO (this << " Update RLC BSR UE " << rnti << " size " << size << " BSR " << (*it).second);
      if ((*it).second >= size)
        {
          (*it).second -= size;
        }
      else
        {
          (*it).second = 0;
        }
    }
  else
    {
      NS_LOG_ERROR (this << " Does not find BSR report info of UE " << rnti);
    }

}


void
RrFfMacScheduler::TransmissionModeConfigurationUpdate (uint16_t rnti, uint8_t txMode)
{
  NS_LOG_FUNCTION (this << " RNTI " << rnti << " txMode " << (uint16_t)txMode);
  FfMacCschedSapUser::CschedUeConfigUpdateIndParameters params;
  params.m_rnti = rnti;
  params.m_transmissionMode = txMode;
  m_cschedSapUser->CschedUeConfigUpdateInd (params);
}



}
