/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007,2008,2009 INRIA, UDCAST
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
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
 */
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "udp-client.h"
#include "seq-ts-header.h"
#include <cstdlib>
#include <cstdio>
#include "ns3/core-module.h" //S: For random variable
#include "packet-ts-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("UdpClient");

NS_OBJECT_ENSURE_REGISTERED (UdpClient);

TypeId
UdpClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UdpClient")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<UdpClient> ()
    .AddAttribute ("MaxPackets",
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&UdpClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval",
                   "The time to wait between packets", TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&UdpClient::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&UdpClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort", "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&UdpClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize",
                   "Size of packets generated. The minimum packet size is 14 bytes which is the size of the header carrying the sequence number and the time stamp.",
                   UintegerValue (128),
                   MakeUintegerAccessor (&UdpClient::m_size),
                   MakeUintegerChecker<uint32_t> (14,1500))
	.AddAttribute ("Percent",
				   "Irregular traffic percentage",
				   UintegerValue(1),
				   MakeUintegerAccessor(&UdpClient::percent),
				   MakeUintegerChecker<uint32_t>(0, 100)) //S: Adding attribute percent for irregular traffic calculation
	.AddAttribute ("EnableRandom",
				   "EnableRandom",
				   UintegerValue(0),
				   MakeUintegerAccessor(&UdpClient::EnableRandom),
				   MakeUintegerChecker<uint32_t>(0, 7)) //S: Adding attribute for EnableRandom
    .AddAttribute ("EnableDiagnostic",
                   "Enabling Diagnostic flag",
                   UintegerValue(0),
                   MakeUintegerAccessor(&UdpClient::enable_diagnostic),
                   MakeUintegerChecker<uint16_t>(0, 1)) //S: Adding attribute for EnableRandom

  ;
  return tid;
}

UdpClient::UdpClient ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_socket = 0;
  m_sendEvent = EventId ();
}

UdpClient::~UdpClient ()
{
  NS_LOG_FUNCTION (this);
}

void
UdpClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
UdpClient::SetRemote (Address addr)
{
  NS_LOG_FUNCTION (this << addr);
  m_peerAddress = addr;
}

void
UdpClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
UdpClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
        {
          if (m_socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
      else if (Ipv6Address::IsMatchingType(m_peerAddress) == true)
        {
          if (m_socket->Bind6 () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
      else if (InetSocketAddress::IsMatchingType (m_peerAddress) == true)
        {
          if (m_socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (m_peerAddress);
        }
      else if (Inet6SocketAddress::IsMatchingType (m_peerAddress) == true)
        {
          if (m_socket->Bind6 () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (m_peerAddress);
        }
      else
        {
          NS_ASSERT_MSG (false, "Incompatible address type: " << m_peerAddress);
        }
    }

  m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  //m_socket->SetRecvCallback(MakeCallback(&UdpClient::Receive,this));
  m_socket->SetSendCallback(MakeCallback(&UdpClient::Receive,this));
  m_socket->SetAllowBroadcast (true);
  m_sendEvent = Simulator::Schedule (Seconds (0.0), &UdpClient::Send, this);
  if (enable_diagnostic == 1)
  {
       Simulator::Schedule (Seconds (0.0), &UdpClient::SendDiagnostics, this);
  }
}

void
UdpClient::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_sendEvent);
}


void UdpClient::Receive (Ptr<Socket> socket, uint32_t length)
{

    std::cout<<" UdpClient::Receive "<<std::endl;
  /*Address from;
  Ptr<Packet> packet= socket->RecvFrom(from);

  Ipv4Address ipv4From = InetSocketAddress::ConvertFrom(from).GetIpv4();

  Ipv4Header my;
  packet->PeekHeader (my);

  std::cout << "destination=" << my.GetDestination () << std::endl;
  std::cout << "source = " << my.GetSource () << " is sending." << std::endl;*/

    /*Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom (from)))
    {
        if (packet->GetSize () == 0)
        { //EOF
          break;
        }

        std::cout<<"At time " << Simulator::Now ().GetSeconds ()
                   << "s packet sink received "
                   <<  packet->GetSize () << " bytes from "
                   << InetSocketAddress::ConvertFrom(from).GetIpv4 ()
                   << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()<<std::endl;
    }*/

}

void UdpClient::SendSingleP(uint16_t pt, uint32_t siz)
{
    PacketTsHeader packt;
    packt.SetTypeP(pt);

    SeqTsHeader seqTs;
    seqTs.SetSeq (m_sent);

    Ptr<Packet> p = Create<Packet> (siz-(8+4+2)); // 8+4 : the size of the seqTs header
    //std::cout<<Simulator::Now().GetSeconds()<<" 1 UdpClient::Send:  "<<p->GetSize()<<std::endl;

    p->AddHeader (seqTs);
    p->AddHeader(packt);

    std::stringstream peerAddressStringStream;
    if (Ipv4Address::IsMatchingType (m_peerAddress))
    {
        peerAddressStringStream << Ipv4Address::ConvertFrom (m_peerAddress);
    }
    else if (Ipv6Address::IsMatchingType (m_peerAddress))
    {
        peerAddressStringStream << Ipv6Address::ConvertFrom (m_peerAddress);
    }

    if ((m_socket->Send (p)) >= 0)
    {

        //std::cout<<Simulator::Now().GetSeconds()<<" "<<m_sent<<" UdpClient::Send: "<<p<<std::endl;
        ++m_sent;
        NS_LOG_INFO ("TraceDelay TX " << siz << " bytes to "
                                    << peerAddressStringStream.str () << " Uid: "
                                    << p->GetUid () << " Time: "
                                    << (Simulator::Now ()).GetSeconds ());

    }
    else
    {
       NS_LOG_INFO ("Error while sending " << siz << " bytes to "
                                      << peerAddressStringStream.str ());
    }

}

void UdpClient::SendOtherVoicecall(void)
{
    int i;
    Time m_interval = (Seconds (0.25));
    for (i=0; i<4; i++)
    {
        m_sendEvent = Simulator::Schedule( m_interval, &UdpClient::SendSingleP, this, 3, 128);
        m_interval = m_interval + Seconds (0.25);
    }
}

void UdpClient::SendDiagnostics(void)
{
    Time random = Seconds(10);
    Simulator::Schedule (random, &UdpClient::SendSingleP, this, 5, 32); //0.03 is setup time
    Simulator::Schedule (random, &UdpClient::SendDiagnostics, this);
}

void
UdpClient::Send (void)
{
    NS_LOG_FUNCTION (this);
    NS_ASSERT (m_sendEvent.IsExpired ());

    if (m_sent <  m_count)
    {
        if (1 == EnableRandom)
        {
            std::cout<<"EnableRandom"<<EnableRandom<<" "<<m_sent<<std::endl;
            Ptr<ExponentialRandomVariable> m_rv = CreateObject<ExponentialRandomVariable> ();
            m_rv->SetAttribute ("Mean", DoubleValue (m_interval.ToDouble(ns3::Time::S) * 100 / percent)); //S: p% of the regular packet interval
            m_rv->SetAttribute ("Bound", DoubleValue (0.0));
            double random = m_rv->GetValue();
            m_sendEvent = Simulator::Schedule(Seconds(0.03), &UdpClient::SendSingleP, this, 6, m_size);
            m_sendEvent = Simulator::Schedule (Seconds(random), &UdpClient::Send, this);
        }
        else if (2 == EnableRandom) // Simulate Voice
        {
                std::cout<<"EnableRandom"<<EnableRandom<<" "<<m_sent<<std::endl;
            m_sendEvent = Simulator::Schedule(Seconds(0.03), &UdpClient::SendSingleP, this, 1, 17); //Alarm

            //Wait for upto 15 sec

            //First voice packet
            m_sendEvent = Simulator::Schedule( Seconds(15), &UdpClient::SendSingleP, this, 2, 128);  //Start Voice call

            //More voice packets
            int i;
            double random = 15;
            for (i=0; i<10; i++)   //Call Upto 50sec
            {
                Time m_interval = (Seconds (5.0));
                Ptr<ExponentialRandomVariable> m_rv = CreateObject<ExponentialRandomVariable> ();

                m_rv->SetAttribute ("Mean", DoubleValue (m_interval.ToDouble(ns3::Time::S) )); //S: p% of the regular packet interval
                m_rv->SetAttribute ("Bound", DoubleValue (0.0));
                random = random + 5;//m_rv->GetValue();
                std::cout<<"ExponentialRandomVariable "<<random<<std::endl;
                Simulator::Schedule (Seconds(random), &UdpClient::SendOtherVoicecall, this);
            }

            //Schedule next Alrm call next 24 hours
            m_sendEvent = Simulator::Schedule (Seconds(86400), &UdpClient::Send, this);
        }
        else if (3 == EnableRandom) // Simulate Update
        {
            m_sendEvent = Simulator::Schedule(Seconds(0.03), &UdpClient::SendSingleP, this, 1, 17); //Alarm

            //Wait for upto 15 sec

            //First voice packet
            m_sendEvent = Simulator::Schedule( Seconds(15), &UdpClient::SendSingleP, this, 4, 32);  //Start Voice call

            //More voice packets
            int i;
            Time m_interval = (Seconds (3.0));
            for (i=0; i<20; i++)   //Call Upto 60sec
            {
                Ptr<ExponentialRandomVariable> m_rv = CreateObject<ExponentialRandomVariable> ();
                m_sendEvent = Simulator::Schedule(m_interval, &UdpClient::SendSingleP, this, 4, 32);
                m_interval = m_interval + Seconds (3.0);
            }

            //Schedule next Alrm call next 24 hours
            m_sendEvent = Simulator::Schedule (Seconds(86400), &UdpClient::Send, this);
        }
        if (7 == EnableRandom)
        {
            std::cout<<"EnableRandom"<<EnableRandom<<" "<<m_sent<<std::endl;
            Ptr<ExponentialRandomVariable> m_rv = CreateObject<ExponentialRandomVariable> ();
            m_rv->SetAttribute ("Mean", DoubleValue (m_interval.ToDouble(ns3::Time::S) * 100 / percent)); //S: p% of the regular packet interval
            m_rv->SetAttribute ("Bound", DoubleValue (0.0));
            double random = m_rv->GetValue();
            m_sendEvent = Simulator::Schedule(Seconds(0.03), &UdpClient::SendSingleP, this, 7, m_size);
            m_sendEvent = Simulator::Schedule (Seconds(random), &UdpClient::Send, this);
        }
        else
        {
            std::cout<<"EnableRandom"<<EnableRandom<<" "<<m_sent<<std::endl;
            m_sendEvent = Simulator::Schedule (Seconds(0.03), &UdpClient::SendSingleP, this, 6, m_size); //0.03 is setup time
            m_sendEvent = Simulator::Schedule (m_interval, &UdpClient::Send, this);
        }


    }
}


} // Namespace ns3
