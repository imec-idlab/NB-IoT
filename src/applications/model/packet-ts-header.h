/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef PACKET_TS_HEADER_H
#define PACKET_TS_HEADER_H

#include "ns3/header.h"
#include "ns3/nstime.h"
#include "seq-ts-header.h"

namespace ns3 {

class PacketTsHeader : public Header
{
public:
        PacketTsHeader ();

  /**
   * \param seq the sequence number
   */
  void SetTypeP (uint16_t seq);
  /**
   * \return the sequence number
   */
  uint16_t GetTypeP (void) const;

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);


private:
  uint32_t m_packettype; //!< 0: Invalid, 1: Alarm,  2:Voice call initiator, 3: MoreVoice packets 4:Updates 5: Diagnostics, 6: Independent, 7: Dl follows UL
};

} // namespace ns3

#endif /* SEQ_TS_HEADER_H */
