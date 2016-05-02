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

 #include <math.h>
 #include <algorithm>
 #include <iostream>
 #include <iomanip>
 #include <map>
 #include "ns3/node-list.h"
 #include "ns3/node.h"
 #include "ns3/aerorp-ntable.h"
 #include "ns3/simulator.h"
 #include "ns3/log.h"
 #include "ns3/callback.h"
 #include "ns3/ipv4.h"
 #include "ns3/timer.h"
 #include "ns3/vector.h"
 #include "ns3/double.h"
 #include "ns3/wifi-mac-header.h"
 #include "ns3/mobility-model.h"
 
#define PI 3.14159265 

 NS_LOG_COMPONENT_DEFINE ("AeroRPNeighborTable");
 
 
 namespace ns3 {
 namespace AeroRP {

NS_OBJECT_ENSURE_REGISTERED ( NeighborTable);

TypeId
 NeighborTable::GetTypeId (void)
 {
   static TypeId tid = TypeId ("ns3::AeroRP::NeighborTable")
     .SetParent<Object>()
     .AddConstructor<NeighborTable> ()
     .AddAttribute ("MaxRange", "Max Range for TTI calculation",
                    DoubleValue (27800),
                    MakeDoubleAccessor (&NeighborTable::m_range),
                    MakeDoubleChecker<double> ());
   return tid;
 }


   NeighborTableEntry :: NeighborTableEntry ()
   {

   }

   NeighborTableEntry :: ~NeighborTableEntry ()
   { 
     
   }

   void
   NeighborTableEntry :: SetPositionVector (Vector position)
   {
      m_position = position;
   }

   Vector 
   NeighborTableEntry :: GetPositionVector ()
   {
    return m_position;
   }

   void 
   NeighborTableEntry :: SetVelocityVector (Vector velocity)
   {
      m_velocity = velocity;
   }

   Vector
   NeighborTableEntry :: GetVelocityVector ()
   {
      return m_velocity;
   }

   double
   NeighborTableEntry :: CalculateRelativeDistance (Vector dstPosition , Vector nodePosition )
   {
    //func at the vector header file it calculates the relative distance
    NS_LOG_LOGIC ("dst x " << dstPosition.x << "dst y  " <<dstPosition.y<< "dst z  " <<dstPosition.z<< "node x  " <<nodePosition.x<< "node y  " <<nodePosition.y<< "node z  " <<nodePosition.z<<"dx "<<(dstPosition.x-nodePosition.x)<<"dy "<<(dstPosition.y-nodePosition.y)<<"dz "<<(dstPosition.z-nodePosition.z));
    return (CalculateDistance (dstPosition,nodePosition ));
  }

  double
  NeighborTableEntry :: CalculateRelativeSpeed (Vector dstPosition , Vector nodeVelocity, Vector  											nodePosition)
  {
    double vel;
    double rvel;
    double theta;
    double thetadot;
    vel = sqrt ((nodeVelocity.x)*(nodeVelocity.x)+(nodeVelocity.y)*(nodeVelocity.y)
	  +(nodeVelocity.z)*(nodeVelocity.z));
    theta = atan2 (nodeVelocity.y , nodeVelocity.x)*180/PI; 
    thetadot = atan2 ((dstPosition.y-nodePosition.y),(dstPosition.x-nodePosition.x))*180/PI;
    rvel = vel * cos (theta-thetadot);
    return rvel;
  }


  double
  NeighborTableEntry :: CalculateTimeToIntercept (Vector dstPosition, Vector nodePosition, Vector nodeVelocity, double m_range)
  {
    double dd;						//relative distance
    double dv;						//relative speed
    double tti;						//time to intercept

    dd = CalculateRelativeDistance (dstPosition,nodePosition );
    dv = CalculateRelativeSpeed (dstPosition, nodeVelocity,nodePosition);
    NS_LOG_DEBUG ("Calculating TTI: rel. dis=" << dd << " rel speed=" << dv << " max range="
	<< m_range);
    if ( dv < 0 && dd >= m_range)
      tti = 0;
    else
    {
    tti = (dd - m_range)/dv;
    if (dd < (m_range-3000) && dv < 0 )
     {
    NS_LOG_DEBUG ("Calculating TTI: rel. dis=" << dd << " rel speed=" << dv << " max range="
	<< m_range << " tti=" << tti);
    return (-1 * tti);
     } 
    }
    NS_LOG_DEBUG ("Calculating TTI: rel. dis=" << dd << " rel speed=" << dv << " max range="
	<< m_range << " tti=" << tti);
    return tti;
  }

   void
   NeighborTableEntry :: Print (Ptr<OutputStreamWrapper> stream) const
   {

  *stream->GetStream () << std::setiosflags (std::ios::fixed) << std::setw (5) << m_position << "\t" << std::setw (5) << m_velocity << "\t\t"<< std::setprecision (3)<<"\t\t"<< TTI<<"s\t\t" << Simulator::Now () << "s\n";
   }

/*
  double
  NeighborTableEntry::CalculateMyTTI (Vector dstPosition)
  {
   Ptr<Node> node = NodeList().GetNode ();
   Vector myPosition;
   Vector myVelocity;
   double myTTI; 
   Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
   myPosition = (*node->GetObject<MobilityModel>()).GetPosition ();
   myVelocity = (*node->GetObject<MobilityModel>()).GetVelocity ();
   myTTI = CalculateTimeToIntercept (dstPosition,myVelocity,myPosition);
   return myTTI;
  }

*/
  /*
    aerorp neighbor table
  */
  //c-tor that determine life time of the entry and 
   
   NeighborTable::NeighborTable ()
   {
   m_txErrorCallback = MakeCallback (&NeighborTable::ProcessTxError, this);
   m_entryLifeTime = Seconds (4); 
   }

   NeighborTable::~NeighborTable ()
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
 NeighborTable::GetEntryUpdateTime (Ipv4Address id)
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
   std::map<Ipv4Address, std::pair <NeighborTableEntry , Time > >::iterator i = m_table.find (id);
   return i->second.second;
 }
 
 /**
  * \brief Adds entry in position table and update it
  */
 void 
 NeighborTable::AddEntry (Ipv4Address id, Vector position , Vector velocity)
 {

   NS_LOG_FUNCTION (this << id << " " << position << " " << velocity);

   NeighborTableEntry add;
   std::map<Ipv4Address, std::pair <NeighborTableEntry , Time > >::iterator i = m_table.find (id);
   add.SetPositionVector (position);
   add.SetVelocityVector(velocity);
   if (i != m_table.end () || id.IsEqual (i->first))
     {       
       m_table.erase (id);
       m_table.insert (std::make_pair (id, std::make_pair (add, Simulator::Now ())));
       return;
     }
       m_table.insert (std::make_pair (id, std::make_pair (add, Simulator::Now ())));
 }

 void 
 NeighborTable :: UpdateEntry (Ipv4Address id, Vector position , Vector velocity)
 {
   NeighborTableEntry add;
   std::map<Ipv4Address, std::pair <NeighborTableEntry , Time > >::iterator i = m_table.find (id);
   add.SetPositionVector (position);
   add.SetVelocityVector(velocity);
   Time m_time;
   if (i != m_table.end () || id.IsEqual (i->first))
      {
       i->second.first.SetPositionVector (position);
       i->second.first.SetVelocityVector(velocity);
       //i->second.m_time = Simulator::Now ();
      }
    else
     AddEntry(id,position,velocity);
 }
 
 /**
  * \brief Deletes entry in position table
  */
 void NeighborTable::DeleteEntry (Ipv4Address id)
 {
   m_table.erase (id);
 }
 
  /**
  * \brief remove entries with expired lifetime
  */
 void 
 NeighborTable::Purge ()
 {
 
   if(m_table.empty ())
     {
       return;
     }
 
   std::list<Ipv4Address> toErase;
 
   std::map<Ipv4Address, std::pair <NeighborTableEntry , Time > >::iterator i = m_table.begin();
   std::map<Ipv4Address, std::pair <NeighborTableEntry , Time > >::iterator listEnd = m_table.end ();
   
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
 NeighborTable::Clear ()
 {
   m_table.clear ();
 }

 /**
  * \brief Gets velocity from neighbor table
  * \param id Ipv4Address to get velocity from
  * \return Position of that id or NULL if not known
  */
 Vector 
 NeighborTable::GetVelocity (Ipv4Address id)
 {
 // getzero method return the 0.0.0.0 address
   if (id == Ipv4Address::GetZero ())
     {
       return NeighborTable::GetInvalidVelocity ();
     }
   std::map<Ipv4Address, std::pair <NeighborTableEntry , Time > >::iterator i = m_table.find (id);
      if (i != m_table.end () || id.IsEqual (i->first))
	  {
	  //return the position of the node
	    return i -> second.first.GetVelocityVector();
	  } 
      else
       {
     	  return NeighborTable::GetInvalidVelocity ();
       }
	return Vector (-1, -1, 0);
 }

 /**
  * \brief Gets position from position table
  * \param id Ipv4Address to get position from
  * \return Position of that id or NULL if not known
  */
 Vector 
 NeighborTable::GetPosition (Ipv4Address id)
 {
 // getzero method return the 0.0.0.0 address
   if (id == Ipv4Address::GetZero ())
     {
       return NeighborTable::GetInvalidPosition ();
     }
   std::map<Ipv4Address, std::pair <NeighborTableEntry , Time > >::iterator  i = m_table.find (id);
      if (i != m_table.end () || id.IsEqual (i->first))
	  {
	  //return the position of the node
	    return i -> second.first.GetPositionVector();
	  } 
      else
       {
     	  return NeighborTable::GetInvalidPosition ();
       }
	return Vector (-1, -1, 0);
 }
  
 /**
  * \brief Returns true if is in search for destionation
  */
 bool NeighborTable::IsInSearch (Ipv4Address id)
 {
   return false;
 }
 
 bool NeighborTable::HasPosition (Ipv4Address id)
 {
   return true;
 }
  /**
  * \ProcessTxError
  */
 void NeighborTable::ProcessTxError (WifiMacHeader const & hdr)
 {
 }

 
 /**
  * \brief Gets next hop according to aerorp protocol
  * \param position the position of the destination node
  * \param nodePos the position of the node that has the packet
  * \return Ipv4Address of the next hop, Ipv4Address::GetZero () if no nighbour was found in greedy mode
  */
 Ipv4Address 
 NeighborTable::BestNeighbor (Vector dstPosition, Vector nodePosition, Vector nodeVelocity)
 {
   Purge ();
   NS_LOG_DEBUG ("calculate intial TTI: ");
   double initialTTI = NeighborTableEntry::CalculateTimeToIntercept(dstPosition, nodePosition,nodeVelocity, m_range );
 
   if (m_table.empty ())
     {
       NS_LOG_DEBUG ("BestNeighbor table is empty; Position: " << nodePosition);
       return Ipv4Address::GetZero ();
     }     //if table is empty (no neighbours)
 
   Ipv4Address bestFoundID = m_table.begin ()->first;
   double bestFoundTTI = initialTTI;
   std::map<Ipv4Address,std::pair <NeighborTableEntry , Time > > ::iterator i;
  for (i = m_table.begin (); !(i == m_table.end ()); i++)
     {
       NS_LOG_DEBUG ("calculate TTI for neighbors: " << i->first);
      double currentTTI = NeighborTableEntry::CalculateTimeToIntercept (dstPosition,i->second.first.GetPositionVector(),i->second.first.GetVelocityVector(), m_range);
        
       if (((bestFoundTTI > currentTTI) && (currentTTI != 0))
          ||
          ((bestFoundTTI == 0) && (currentTTI != 0)))
         {
           bestFoundID = i->first;
           bestFoundTTI = currentTTI;
         }
     }
   NS_LOG_DEBUG ("best neighbor is: " << bestFoundID << " " << bestFoundTTI);
 
   if((initialTTI > bestFoundTTI)
     || ((initialTTI == 0) && (bestFoundTTI != 0))
     )
    {
       NS_LOG_DEBUG ("best neighbor is: " << bestFoundID);
       return bestFoundID;
    }
   else
       NS_LOG_DEBUG ("i have returned zero: ");
     return Ipv4Address::GetZero (); ///it can be removed
 }
 
 /**
  * \brief Checks if a node is a neighbour
  * \param id Ipv4Address of the node to check
  * \return True if the node is neighbour, false otherwise
  */
 bool
 NeighborTable::isNeighbour (Ipv4Address id)
 {
 
   std::map<Ipv4Address,std::pair <NeighborTableEntry , Time > > ::iterator i = m_table.find (id);
   if (i != m_table.end () || id.IsEqual (i->first))
     {
       return true;
     }
 
   return false;
 }

  void 
  NeighborTable::Print (Ptr<OutputStreamWrapper> stream) const
  {
  std::map<Ipv4Address, std::pair <NeighborTableEntry , Time > >::const_iterator i = m_table.begin();


  *stream->GetStream() << "\nAeroRP neighbor table\n" << "Ipv4Address\t\t\t\t\tposition\t\t\t\t\tvelocity\t\t\t\t\tTTI\n";
  for (; i != m_table.end (); ++i)
    {
      *stream->GetStream() << i->first << "\t\t";
      i->second.first.Print (stream);
    }
  *stream->GetStream () << "\n";
 }

 }   // aerorp
 } // ns3
