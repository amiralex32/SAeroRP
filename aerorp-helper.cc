/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:"nill"; -*- */
/*
 * Copyright (c) 2012 Amir Reda
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
 *Based on AeroRP 
 *Authors: Dr/ Sherif Khatab <s.khattab@fci-cu.edu.eg>
 *         Eng/ Amir mohamed Reda <amiralex32@gmail.com>
*/

#include "aerorp-helper.h"
#include "ns3/aerorp-routing-protocol.h"
#include "ns3/node-list.h"
#include "ns3/node-container.h"
#include "ns3/names.h"
#include "ns3/callback.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/ptr.h"

namespace ns3 {
AeroRPHelper::~AeroRPHelper ()
{
}

AeroRPHelper::AeroRPHelper () : Ipv4RoutingHelper ()
{
  m_agentFactory.SetTypeId ("ns3::AeroRP::AeroRoutingProtocol");
}

AeroRPHelper*
AeroRPHelper::Copy (void) const
{
  return new AeroRPHelper (*this);
}

Ptr<Ipv4RoutingProtocol>
AeroRPHelper::Create (Ptr<Node> node) const
{
  Ptr<AeroRP::AeroRoutingProtocol> agent = m_agentFactory.Create<AeroRP::AeroRoutingProtocol> ();
  node->AggregateObject (agent);
  return agent;
}

void
AeroRPHelper::Set (std::string name, const AttributeValue &value)
{
  m_agentFactory.Set (name, value);
}

void 
AeroRPHelper::Install (void) const
{
  NodeContainer c = NodeContainer::GetGlobal ();
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); i++)
    {
      Ptr<Node> node = (*i);
      Ptr<UdpL4Protocol> udp = node->GetObject<UdpL4Protocol> ();
      Ptr<AeroRP::AeroRoutingProtocol> aerorp = node->GetObject<AeroRP::AeroRoutingProtocol> ();
      aerorp->SetDownTarget (udp->GetDownTarget ());
     // udp->SetDownTarget (MakeCallback(&AeroRP::AeroRoutingProtocol::AddHeaders, aerorp));
    }
 }


}
