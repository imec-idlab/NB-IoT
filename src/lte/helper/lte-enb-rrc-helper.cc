/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "lte-enb-rrc-helper.h"
#include "ns3/lte-enb-rrc.h"
#include "ns3/boolean.h"

namespace ns3 {

LteEnbRrchHelper::LteEnbRrchHelper ()
{

}

LteEnbRrchHelper::~LteEnbRrchHelper ()
{
}


void Setrrctime (int t)
{
    Ptr<LteEnbRrc> um2 = CreateObject<LteEnbRrc> ();
    um2->set_rrc_timer(t);
}


} //namespace ns3
