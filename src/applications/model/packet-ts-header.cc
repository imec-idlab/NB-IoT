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

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include "packet-ts-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PacketTsHeader");

NS_OBJECT_ENSURE_REGISTERED (PacketTsHeader);

PacketTsHeader::PacketTsHeader ()
  : m_packettype (0)
{
  NS_LOG_FUNCTION (this);
}

void
PacketTsHeader::SetTypeP (uint16_t seq)
{
  NS_LOG_FUNCTION (this << seq);
  m_packettype = seq;
}
uint16_t
PacketTsHeader::GetTypeP (void) const
{
  NS_LOG_FUNCTION (this);
  return m_packettype;
}

TypeId
PacketTsHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PacketTsHeader")
    .SetParent<Header> ()
    .SetGroupName("Applications")
    .AddConstructor<PacketTsHeader> ()
  ;
  return tid;
}
TypeId
PacketTsHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
PacketTsHeader::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "(Packet type=" << m_packettype << ")";
}
uint32_t
PacketTsHeader::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return 2;
}

void
PacketTsHeader::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  i.WriteHtonU16 (m_packettype);
}
uint32_t
PacketTsHeader::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  m_packettype = i.ReadNtohU16 ();
  return GetSerializedSize ();
}

} // namespace ns3
