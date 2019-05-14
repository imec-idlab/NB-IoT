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
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#ifndef NB_FF_MAC_COMMON_H
#define NB_FF_MAC_COMMON_H

#include <ns3/simple-ref-count.h>
#include <ns3/ptr.h>
#include <vector>


namespace ns3 {


/**
 * \brief See section 4.3.16 drxConfig, For simplicity we are using integers for the parameters instead of enumerations
 */
struct DrxConfigNB_s
{
  uint8_t  m_onDurationTimer;              ///< Allowed values in #PDCCH periods: [   1,2,3,4,8,16,32]
  uint8_t  m_drxInactivityTimer;           ///< Allowed values in #PDDCH periods: [ 0,1,2,3,4,8,16,32]
  uint8_t  m_drxRetransmissionTimer;       ///< Allowed values in #PDDCH periods: [ 0,1,2,4,6,8,16,24,33]
  uint16_t m_drxCycle;                     ///< Allowed values in SF: [ 256,512,1024,1536,2048,3072,4096,4608,6144,7680,8192,9216]
  uint8_t  m_drxStartOffset;               ///< Allowed values in SF: [ 0 to 255]
  uint16_t m_drxULRetransmissionTimer;     ///< Allowed values in #PDDCH periods: [ 0, 1, 2, 4, 6, 8, 16, 24, 33, 40, 64, 80, 96, 112, 128, 160, 320 ]
};

} // namespace ns3

#endif /* NB_FF_MAC_COMMON_H */
