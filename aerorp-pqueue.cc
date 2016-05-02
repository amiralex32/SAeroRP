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

#include "aerorp-pqueue.h"
#include <algorithm>
#include <functional>
#include "ns3/ipv4-route.h"
#include "ns3/socket.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("AeroRPPacketQueue");

namespace ns3
{
namespace AeroRP
{
uint32_t
RequestQueue::GetSize ()
{
  Purge ();
  return m_queue.size ();
}

  void 
  RequestQueue::PrintQueue (Ptr<OutputStreamWrapper> stream) const
  {

  *stream->GetStream() << "\n Queue table\n" << "Packet ID\t\tSource\t\tExpire Time\n";
   for (std::vector<QueueEntry>::const_iterator i = m_queue.begin (); i
       != m_queue.end (); ++i)
    {
      *stream->GetStream() << i->GetPacket ()->GetUid () << "\t\t"<<i->GetIpv4Header ().GetSource ()<< "\t\t"<< i->GetExpireTime ();
    }
  *stream->GetStream () << "\n";
 }

bool
RequestQueue::Enqueue (QueueEntry & entry)
{
  NS_LOG_FUNCTION ("Enqueing packet destined for"<<entry.GetIpv4Header ().GetDestination () <<entry.GetIpv4Header ().GetSource () << entry.GetPacket ()->GetUid ());
  Purge ();
  for (std::vector<QueueEntry>::const_iterator i = m_queue.begin (); i
       != m_queue.end (); ++i)
    {
      if ((i->GetPacket ()->GetUid () == entry.GetPacket ()->GetUid ())
          && (i->GetIpv4Header ().GetDestination ()
              == entry.GetIpv4Header ().GetDestination ()))
        return false;
    }
  entry.SetExpireTime (m_queueTimeout);
  if (m_queue.size () == m_maxLen)
    {
      Drop (m_queue.front (), "Drop the most aged packet"); // Drop the most aged packet
      m_queue.erase (m_queue.begin ());
    }
  m_queue.push_back (entry);
  return true;
}

void
RequestQueue::DropPacketWithDst (Ipv4Address dst)
{
  NS_LOG_FUNCTION (this);
  Purge ();
  for (std::vector<QueueEntry>::iterator i = m_queue.begin (); i
       != m_queue.end (); ++i)
    {
      if (IsEqual (*i, dst))
        {
          Drop (*i, "DropPacketWithDst ");
        }
    }
  m_queue.erase (std::remove_if (m_queue.begin (), m_queue.end (),
                                 std::bind2nd (std::ptr_fun (RequestQueue::IsEqual), dst)), m_queue.end ());
}

bool
RequestQueue::Dequeue (Ipv4Address dst, QueueEntry & entry)
{
  Purge ();

  for (std::vector<QueueEntry>::iterator i = m_queue.begin (); i != m_queue.end (); ++i)
    {
      if (i->GetIpv4Header ().GetDestination () == dst)
        {
          entry = *i;
 NS_LOG_FUNCTION ("Dequeue packet destined for"<< entry.GetIpv4Header ().GetDestination () <<entry.GetIpv4Header ().GetSource () << entry.GetPacket ()->GetUid ());
          m_queue.erase (i);
          return true;
        }
    }
  return false;
}

bool
RequestQueue::Find (Ipv4Address dst)
{
  NS_LOG_LOGIC (this<<dst);
  for (std::vector<QueueEntry>::const_iterator i = m_queue.begin (); i
       != m_queue.end (); ++i)
    {
      if (i->GetIpv4Header ().GetDestination () == dst)
        return true;
    }
  return false;
}

struct IsExpired
{
  bool
  operator() (QueueEntry const & e) const
  {
    return (e.GetExpireTime () < Seconds (0));
  }
};

void
RequestQueue::Purge ()
{
  NS_LOG_LOGIC (this);
  IsExpired pred;
  for (std::vector<QueueEntry>::iterator i = m_queue.begin (); i
       != m_queue.end (); ++i)
    {
      if (pred (*i))
        {
          Drop (*i, "Drop outdated packet ");
        }
    }
  m_queue.erase (std::remove_if (m_queue.begin (), m_queue.end (), pred),
                 m_queue.end ());
}

void
RequestQueue::Drop (QueueEntry en, std::string reason)
{
  NS_LOG_LOGIC ("Drop packet "<<reason << en.GetPacket ()->GetUid () << " " << en.GetIpv4Header ().GetDestination ());
  en.GetErrorCallback () (en.GetPacket (), en.GetIpv4Header (),
                          Socket::ERROR_NOROUTETOHOST);
  return;
}

}
}
