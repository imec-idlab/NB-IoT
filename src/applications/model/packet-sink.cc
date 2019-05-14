/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 * Author:  Tom Henderson (tomhend@u.washington.edu)
 */
#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "packet-sink.h"
#include "ns3/simulator.h"
#include "ns3/udp-header.h"
#include "ns3/seq-ts-header.h"
#include "ns3/ipv4.h"
#include "packet-ts-header.h"
#include "ns3/core-module.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PacketSink");

NS_OBJECT_ENSURE_REGISTERED (PacketSink);

TypeId 
PacketSink::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PacketSink")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<PacketSink> ()
    .AddAttribute ("Local",
                   "The Address on which to Bind the rx socket.",
                   AddressValue (),
                   MakeAddressAccessor (&PacketSink::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("Protocol",
                   "The type id of the protocol to use for the rx socket.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&PacketSink::m_tid),
                   MakeTypeIdChecker ())
    .AddTraceSource ("Rx",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&PacketSink::m_rxTrace),
                     "ns3::Packet::AddressTracedCallback")
  ;
  return tid;
}

PacketSink::PacketSink ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_totalRx = 0;
}

PacketSink::~PacketSink()
{
  NS_LOG_FUNCTION (this);
}

uint64_t PacketSink::GetTotalRx () const
{
  NS_LOG_FUNCTION (this);
  return m_totalRx;
}

Ptr<Socket>
PacketSink::GetListeningSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

std::list<Ptr<Socket> >
PacketSink::GetAcceptedSockets (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socketList;
}

void PacketSink::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_socketList.clear ();

  // chain up
  Application::DoDispose ();
}


// Application Methods
void PacketSink::StartApplication ()    // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      if (m_socket->Bind (m_local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      m_socket->Listen ();
      //m_socket->ShutdownSend ();
      if (addressUtils::IsMulticast (m_local))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, m_local);
            }
          else
            {
              NS_FATAL_ERROR ("Error: joining multicast on a non-UDP socket");
            }
        }
    }

  m_socket->SetRecvCallback (MakeCallback (&PacketSink::HandleRead, this));
  m_socket->SetAcceptCallback (
    MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
    MakeCallback (&PacketSink::HandleAccept, this));
  m_socket->SetCloseCallbacks (
    MakeCallback (&PacketSink::HandlePeerClose, this),
    MakeCallback (&PacketSink::HandlePeerError, this));
}

void PacketSink::StopApplication ()     // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);
  while(!m_socketList.empty ()) //these are accepted sockets, close them
    {
      Ptr<Socket> acceptedSocket = m_socketList.front ();
      m_socketList.pop_front ();
      acceptedSocket->Close ();
    }
  if (m_socket) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void PacketSink::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        { //EOF
          break;
        }
      m_totalRx += packet->GetSize ();
      if (InetSocketAddress::IsMatchingType (from))
        {
            PacketTsHeader packt;
            packet->RemoveHeader(packt);

            SeqTsHeader data;
            packet->RemoveHeader(data);

            std::cout<<Simulator::Now().GetSeconds()<<"PacketSink::HandleRead UL Received: "<<" "<<data.GetSeq()<< " "<<packt.GetTypeP()<<" "<<InetSocketAddress::ConvertFrom(from).GetIpv4 ()<< " port " << InetSocketAddress::ConvertFrom (from).GetPort ()<<std::endl;

            if (0 == packt.GetTypeP())
            {
                std::cout<<Simulator::Now().GetSeconds()<<"PacketSink::HandleRead Received invalid packet"<<std::endl;
            }
            else if (1 == packt.GetTypeP())
            {
                std::cout<<Simulator::Now().GetSeconds()<<"PacketSink::HandleRead Received Alarm packet"<<std::endl;

                PacketTsHeader packt;
                packt.SetTypeP(6);

                SeqTsHeader seqTs;
                seqTs.SetSeq (m_sent++);

                Ptr<Packet> p = Create<Packet> (17-(8+4+2)); // 8+4 : the size of the seqTs header

                p->AddHeader (seqTs);
                p->AddHeader(packt);


                //Ptr<Packet> p = Create<Packet> (17);
                socket->SendTo (p, 0, from);
            }
            else if (2 == packt.GetTypeP())
            {
                std::cout<<Simulator::Now().GetSeconds()<<"PacketSink::HandleRead Received Voice call initiator packet"<<std::endl;

                //Call for 5-60 seconds{

                int i;
                Time m_interval = (Seconds (1));
                for (i=0; i<10; i++)   //Call Upto 50sec
                {
                    Simulator::Schedule (m_interval, &PacketSink::SendSingleVoicecall, this, from, socket);
                    m_interval = m_interval + Seconds(5.0);
                    std::cout<<Simulator::Now().GetSeconds()<<"1 PacketSink::HandleRead Received Voice call initiator packet"<<std::endl;

                }
            }
            else if (3 == packt.GetTypeP())
            {
                std::cout<<Simulator::Now().GetSeconds()<<"PacketSink::HandleRead Received More Voice packet"<<std::endl;
            }
            else if (4 == packt.GetTypeP())
            {
                std::cout<<Simulator::Now().GetSeconds()<<"PacketSink::HandleRead Received Updates packet"<<std::endl;
            }
            else if (5 == packt.GetTypeP())
            {
                std::cout<<Simulator::Now().GetSeconds()<<"PacketSink::HandleRead Received Diagnostics packet"<<std::endl;
            }
            else if (6 == packt.GetTypeP())
            {
                std::cout<<Simulator::Now().GetSeconds()<<"PacketSink::HandleRead Received Independent packet"<<std::endl;
            }
            else if (7 == packt.GetTypeP())
            {
                std::cout<<Simulator::Now().GetSeconds()<<"PacketSink::HandleRead Received Alarm packet"<<std::endl;

                PacketTsHeader packt;
                packt.SetTypeP(6);

                SeqTsHeader seqTs;
                seqTs.SetSeq (m_sent++);

                Ptr<Packet> p = Create<Packet> (64-(8+4+2)); // 8+4 : the size of the seqTs header

                p->AddHeader (seqTs);
                p->AddHeader(packt);


                //Ptr<Packet> p = Create<Packet> (17);
                //socket->SendTo (p, 0, from);
                Simulator::Schedule (Seconds(1), &PacketSink::SendSingledata, this, p, from, socket);
            }
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s packet sink received "
                       <<  packet->GetSize () << " bytes from "
                       << InetSocketAddress::ConvertFrom(from).GetIpv4 ()
                       << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s packet sink received "
                       <<  packet->GetSize () << " bytes from "
                       << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
                       << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
      m_rxTrace (packet, from);
    }
}

void PacketSink::SendSingledata(Ptr<Packet> p, Address from, Ptr<Socket> socket)
{
    socket->SendTo (p, 0, from);
}
void PacketSink::SendSingleVoicecall (Address from, Ptr<Socket> socket)
{
    int i;
    Time m_interval = (Seconds (0.25));
    for (i=0; i<4; i++)
    {
        Ptr<Packet> p = Create<Packet> (128);
        Simulator::Schedule (m_interval, &PacketSink::SendSingledata, this, p, from, socket);
        m_interval =  m_interval + (Seconds (0.25));
    }
}

void PacketSink::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}
 
void PacketSink::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}
 

void PacketSink::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  s->SetRecvCallback (MakeCallback (&PacketSink::HandleRead, this));
  m_socketList.push_back (s);
}

} // Namespace ns3
