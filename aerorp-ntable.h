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
 #ifndef AERORP_NTABLE_H
 #define AERORP_NTABLE_H

 #include <iostream>
 #include <math.h>
 #include <map>
 #include "ns3/callback.h"
 #include "ns3/ipv4.h"
 #include "ns3/timer.h"
 #include "ns3/vector.h"
 #include "ns3/wifi-mac-header.h"
 #include "ns3/simulator.h"
 #include "ns3/output-stream-wrapper.h"

 namespace ns3 {
 namespace AeroRP {

 class NeighborTableEntry
 {
  public:
  ///c-tor
  NeighborTableEntry ();
  ~NeighborTableEntry ();

   //set position for neighbor nodes it got it from GS
   void SetPositionVector (Vector position);
   //get position for neighbor nodes
   Vector GetPositionVector ();
  //set speed for neighbor nodes it got it from GS
  void SetVelocityVector (Vector velocity);
  //get speed for neighbor nodes
  Vector GetVelocityVector ();
  /*
   *this method is used to calculate the eculidian distance between neighbor and destination
   *rd = sqrt ((Xd-Xn)^2 + (Yd-Yn)^2 + (Zd-Zn)^2)
   */

   static double CalculateRelativeDistance (Vector dstPosition , Vector nodePosition);

  /*
   *this method used to calculate relative speed for the neighbor and the destination 
   *(coordinates come from destination data table)
   */

  static double CalculateRelativeSpeed (Vector dstSpeed , Vector nodeVelocity , Vector nodePosition);
  /*
   *\breif calculate tti factor 
   */

   static double CalculateTimeToIntercept (Vector dstPosition , Vector nodeVelocity , Vector nodePosition, double m_range) ;
    
   /*
    *\breif calculate TTI from my position
    */

   double CalculateMyTTI (Vector dstPosition);
   //print routing table entry
   void Print (Ptr<OutputStreamWrapper> stream) const;

  
  private:
  
   Vector m_position; 							// position vector
   Vector m_velocity;							// velocity vector

   double TTI;								    // time to inercept
 };

 class NeighborTable: public Object
 {
  public:
  /// c-tor
  NeighborTable ();
  ~NeighborTable ();
   static TypeId GetTypeId (void);

   //used only with the ground station
   //void Destroy ();
   /**
    * \brief Gets the last time the entry was updated
    * \param id Ipv4Address to get time of update from
    * \return Time of last update to the position
    */
   Time GetEntryUpdateTime (Ipv4Address id);
 
   /**
    * \brief Adds entry in routing table
    */
   void AddEntry (Ipv4Address id, Vector position , Vector velocity);
   /*
    * update the coordinates of the nodes due to hello packet or snooping using the set method
    */
  void UpdateEntry (Ipv4Address id, Vector position , Vector velocity);

   /**
    * \brief Deletes entry in routing table
    */
   void DeleteEntry (Ipv4Address id);

   /**
    * \brief remove entries with expired lifetime
    */
   void Purge ();
 
   /**
    * \brief clears all entries
    */
   void Clear ();
 
   /**
    * \Get Callback to ProcessTxError(this is not belong to locationservice)
    */
   Callback<void, WifiMacHeader const &> GetTxErrorCallback () const
   {
     return m_txErrorCallback;
   }

   /**
    * \brief Gets velocity from position table
    * \param id Ipv4Address to get velocity from
    * \return Position of that id or NULL if not known
    */   
   Vector GetVelocity (Ipv4Address id);
   /**
    * \brief Gets position from position table
    * \param id Ipv4Address to get position from
    * \return Position of that id or NULL if not known
    */
   Vector GetPosition (Ipv4Address id);
   
   /*
    * if the node has no geolocation information 
    */ 
   bool IsInSearch (Ipv4Address id);
   /*
    * if the node has a geolocation information
    */
   bool HasPosition (Ipv4Address id);
 
   static Vector GetInvalidPosition ()
   {
     return Vector (-1, -1, 0);
   }
   static Vector GetInvalidVelocity ()
   {
     return Vector (-1, -1, 0);
   }
 
  /**
    * \brief Gets next hop according to aerorp protocol the lowest TTI
    * \param position the position of the destination node
    * \param nodePos the position of the node that has the packet
    * \return Ipv4Address of the next hop, Ipv4Address::GetZero () if no nighbour was found in 
    * \greedy mode
    */

   Ipv4Address BestNeighbor (Vector dstPosition, Vector nodePosition, Vector nodeVelocity);
 
   /**
    * \brief Checks if a node is a neighbour
    * \param id Ipv4Address of the node to check
    * \return True if the node is neighbour, false otherwise(this func is not belong to loctionservice)
    */
   bool isNeighbour (Ipv4Address id);
  /// Print routing table
   void Print (Ptr<OutputStreamWrapper> stream) const;

private:

   Time m_entryLifeTime;
  /*
   *creating an object of class map
   *class map has operation find(id) to Get iterator to elementSearches the container for an element 
   *with a key equivalent to id and returns an iterator to it if found, otherwise it returns an 
   *iterator to map::end
   *iterator is a member type 
   */
   std::map<Ipv4Address,std::pair <NeighborTableEntry , Time > > m_table;
   // TX error callback
   Callback<void, WifiMacHeader const &> m_txErrorCallback;
   // Process layer 2 TX error notification
   void ProcessTxError (WifiMacHeader const&);
   double m_range;

};
}
}
#endif /* aerorp_RTABLE_H */
/*
Callback template class.

This class template implements the Functor Design Pattern. It is used to declare the type of a Callback:

the first non-optional template argument represents the return type of the callback.
the second optional template argument represents the type of the first argument to the callback.
the third optional template argument represents the type of the second argument to the callback.
the fourth optional template argument represents the type of the third argument to the callback.
the fifth optional template argument represents the type of the fourth argument to the callback.
the sixth optional template argument represents the type of the fifth argument to the callback.
Callback instances are built with the MakeCallback template functions. Callback instances have POD semantics: the memory they allocate is managed automatically, without user intervention which allows you to pass around Callback instances by value
#include "ns3/callback.h"
#include "ns3/assert.h"
#include <iostream>

using namespace ns3;

static double 
CbOne (double a, double b)
{
  std::cout << "invoke cbOne a=" << a << ", b=" << b << std::endl;
  return a;
}

class MyCb {
public:
  int CbTwo (double a) {
    std::cout << "invoke cbTwo a=" << a << std::endl;
    return -5;
  }
};


int main (int argc, char *argv[])
{
  // return type: double
  // first arg type: double
  // second arg type: double
  Callback<double, double, double> one;
  // build callback instance which points to cbOne function
  one = MakeCallback (&CbOne);
  // this is not a null callback
  NS_ASSERT (!one.IsNull ());
  // invoke cbOne function through callback instance
  double retOne;
  retOne = one (10.0, 20.0);
  // cast retOne to void, to suppress variable ‘retOne’ set but
  // not used compiler warning
  (void) retOne; 

  // return type: int
  // first arg type: double
  Callback<int, double> two;
  MyCb cb;
  // build callback instance which points to MyCb::cbTwo
  two = MakeCallback (&MyCb::CbTwo, &cb);
  // this is not a null callback
  NS_ASSERT (!two.IsNull ());
  // invoke MyCb::cbTwo through callback instance
  int retTwo;
  retTwo = two (10.0);
  // cast retTwo to void, to suppress variable ‘retTwo’ set but
  // not used compiler warning
  (void) retTwo;
  two = MakeNullCallback<int, double> ();
  // invoking a null callback is just like
  // invoking a null function pointer:
  // it will crash.
  //int retTwoNull = two (20.0);
  NS_ASSERT (two.IsNull ());

#if 0
  // The below type mismatch between CbOne() and callback two will fail to 
  // compile if enabled in this program.
  two = MakeCallback (&CbOne);
#endif

#if 0
  // This is a slightly different example, in which the code will compile
  // but because callbacks are type-safe, will cause a fatal error at runtime 
  // (the difference here is that Assign() is called instead of operator=)
  Callback<void, float> three;
  three.Assign (MakeCallback (&CbOne));
#endif

  return 0;
}
*/
