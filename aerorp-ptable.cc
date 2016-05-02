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

 #include "ns3/aerorp-ptable.h"
 #include "ns3/simulator.h"
 #include <iomanip>
 #include "ns3/log.h"
 #include <algorithm>
 
 NS_LOG_COMPONENT_DEFINE ("AeroRPPositionTable");
 
 
 namespace ns3 {
 namespace AeroRP {
 
 /*
   aerorp position table
 */
 //c-tor that determine life time of the entry and 
 NodesPositionTable::NodesPositionTable ()
 {
   m_txErrorCallback = MakeCallback (&NodesPositionTable::ProcessTxError, this);
   m_entryLifeTime = Seconds (5); 
 }
 NodesPositionTable::~NodesPositionTable ()
 {
 
 }

 /*
 void
 NeighborTable::Destroy
 {
   delete this;
 }
  */
 
 Time 
 NodesPositionTable::GetEntryUpdateTime (Ipv4Address id)
 {
// getzero method return the 0.0.0.0 address
   if (id == Ipv4Address::GetZero ())
     {
       return Time (Seconds (0));
     }
/*
template < class Key,                                     // map::key_type
           class T,                                       // map::mapped_type
           class Compare = less<Key>,                     // map::key_compare
           class Alloc = allocator<pair<const Key,T> >    // map::allocator_type
           > class map;
Map
Maps are associative containers that store elements formed by a combination of a key value and a mapped value, following a specific order.
In a map, the key values are generally used to sort and uniquely identify the elements, while the mapped values store the content associated to this
key. The types of key and mapped value may differ, and are grouped together in member type value_type, which is a pair type combining both:
typedef pair<const Key, T> value_type;
Internally, the elements in a map are always sorted by its key following a specific strict weak ordering criterion indicated by its internal 
comparison object (of type Compare).
map containers are generally slower than unordered_map containers to access individual elements by their key, but they allow the direct iteration 
on subsets based on their order.
Maps are typically implemented as binary search trees.
A unique feature of map among associative containers is that they implement the direct access operator (operator[]), which allows for direct 
access of the mapped value.
///we need the key to be the ID also pair is set to be vlocity vector also
*/
   std::map<Ipv4Address, std :: pair <Vector, Time> >::iterator i = m_table.find (id);
   return i->second.second;
 }
 
 /**
  * \brief Adds entry in position table
  */
 void 
 NodesPositionTable::AddEntry (Ipv4Address id, Vector position)
 {
   NS_LOG_FUNCTION (this << id << " " << position );

   std::map<Ipv4Address, std::pair<Vector, Time> >::iterator i = m_table.find (id);
   if (i != m_table.end () || id.IsEqual (i->first))
     {
       m_table.erase (id);
       m_table.insert (std::make_pair (id, std::make_pair (position, Simulator::Now ())));
       return;
     }
   
   m_table.insert (std::make_pair (id, std::make_pair (position, Simulator::Now ())));
 }
 void 
 NodesPositionTable :: UpdateEntry (Ipv4Address id, Vector position)
 {
      std::map<Ipv4Address, std::pair<Vector, Time> >::iterator i = m_table.find (id);
   if (i != m_table.end () || id.IsEqual (i->first))
       i->second.first = position;
 }
 
 /**
  * \brief Deletes entry in position table
  */
 void NodesPositionTable::DeleteEntry (Ipv4Address id)
 {
   m_table.erase (id);
 }
 
 /**
  * \brief Gets position from position table
  * \param id Ipv4Address to get position from
  * \return Position of that id or NULL if not known
  */
 Vector 
 NodesPositionTable::GetPosition (Ipv4Address id)
 {
 // getzero method return the 0.0.0.0 address
   if (id == Ipv4Address::GetZero ())
     {
       return NodesPositionTable::GetInvalidPosition ();
     }
   std::map<Ipv4Address, std :: pair <Vector, Time> >::iterator i = m_table.find (id);
      if (i != m_table.end () || id.IsEqual (i->first))
	  {
	  //return the position of the node
	    return i -> second.first;
	  } 
      else
       {
     	  return NodesPositionTable::GetInvalidPosition ();
       }
	return Vector (-1, -1, 0);
 }
 
  /**
  * \brief remove entries with expired lifetime
  */
 void 
 NodesPositionTable::Purge ()
 {
 
   if(m_table.empty ())
     {
       return;
     }
 
   std::list<Ipv4Address> toErase;
 
   std::map<Ipv4Address, std::pair<Vector, Time> >::iterator i = m_table.begin ();
   std::map<Ipv4Address, std::pair<Vector, Time> >::iterator listEnd = m_table.end ();
   
  for (; !(i == listEnd); i++)
     {
 
      if (m_entryLifeTime + GetEntryUpdateTime (i->first) <= Simulator::Now ())
         {
           toErase.insert (toErase.begin (), i->first);
 
         }
     }
   toErase.unique ();
 
   std::list<Ipv4Address>::iterator end = toErase.end ();
 
 for (std::list<Ipv4Address>::iterator it = toErase.begin (); it != end; ++it)
     {
 
       m_table.erase (*it);
 
     }
 }
 
 /**
  * \brief clears all entries
  */
 void 
 NodesPositionTable::Clear ()
 {
   m_table.clear ();
 }
 
  /**
  * \ProcessTxError
  */
 void NodesPositionTable::ProcessTxError (WifiMacHeader const & hdr)
 {
 }
 
 /**
  * \brief Returns true if is in search for destionation
  */
 bool NodesPositionTable::IsInSearch (Ipv4Address id)
 {
   return false;
 }
 
 bool NodesPositionTable::HasPosition (Ipv4Address id)
 {
   return true;
 }

  void 
  NodesPositionTable::Print (Ptr<OutputStreamWrapper> stream) const
  {
   std::map<Ipv4Address, std::pair<Vector, Time> >::const_iterator i;
  *stream->GetStream () << "\nAeroRP position table\n" << "Ipv4Address\t\t\t\tposition\t\t\ttime s\n";
  for (i=m_table.begin (); i!= m_table.end (); ++i)
    {
 *stream->GetStream () << std::setiosflags (std::ios::fixed)<< i->first << "\t" << std::setw (10) <<i->second.first << "\t\t\t"<< std::setprecision (3)<<"\t\t\t"<< i->second.second<< "s\n";
    }
  *stream->GetStream () << "\n";
 
 }
 }   // aerorp
 } // ns3
