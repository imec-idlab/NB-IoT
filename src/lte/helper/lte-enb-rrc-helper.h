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

#ifndef LTE_ENB_RRC_HELPER_H
#define LTE_ENB_RRC_HELPER_H

#include "ns3/object-factory.h"

namespace ns3 {

class UeManager;

class LteEnbRrchHelper
{
public:
  /**
   * Create a LteEnbRrchHelper to make life easier for people who want to configure RRC ENB attributes   */

        LteEnbRrchHelper ();

   /**
   * Destroy a LteEnbRrchHelper.
   */

        virtual ~LteEnbRrchHelper ();

  /**
   *
   * This allows the ns3::WifiHelper class to create MAC objects from ns3::WifiHelper::Install.
   */
  virtual void Setrrctime (int t);


  Ptr<UeManager> um;
};

} // namespace ns3

#endif /* LTE_ENB_RRC_HELPER_H */
