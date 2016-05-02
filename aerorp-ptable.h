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
 #ifndef AERORP_PTABLE_H
 #define AERORP_PTABLE_H

 #include <iostream>
 #include <map>
 #include "ns3/callback.h"
 #include "ns3/ipv4.h"
 #include "ns3/timer.h"
 #include "ns3/vector.h"
 #include "ns3/wifi-mac-header.h"
 #include "ns3/output-stream-wrapper.h"

 namespace ns3 {
 namespace AeroRP {

  class NodesPositionTable
  {
   public:
    ///c-tor
    NodesPositionTable ();
    ~NodesPositionTable ();

   //used only with the ground station
   //void Destroy ();

	   /**
    * \brief Gets the last time the entry was updated
    * \param id Ipv4Address to get time of update from
    * \return Time of last update to the position
    */
   Time GetEntryUpdateTime (Ipv4Address id);
 
   /**
    * \brief Adds entry in position table
    */
   void AddEntry (Ipv4Address id, Vector position);
   
   /**
    * \breif update entry of the position vector every 5 seconds
    */
    void UpdateEntry (Ipv4Address id, Vector position);
 
   /**
    * \brief Deletes entry in position table
    */
   void DeleteEntry (Ipv4Address id);
 
   /**
    * \brief Gets position from position table
    * \param id Ipv4Address to get position from
    * \return Position of that id or NULL if not known
    */
   Vector GetPosition (Ipv4Address id);
      /**
    * \brief remove entries with expired lifetime
    */
   void Purge ();
 
   /**
    * \brief clears all entries
    */
   void Clear ();
      bool IsInSearch (Ipv4Address id);
 
   bool HasPosition (Ipv4Address id);
 
   static Vector GetInvalidPosition ()
   {
     return Vector (-1, -1, 0);
   }
   //print routing table entry
   void Print (Ptr<OutputStreamWrapper> stream) const;
   private:

   Time m_entryLifeTime;
   std::map<Ipv4Address, std::pair<Vector, Time> > m_table;
   // TX error callback
   Callback<void, WifiMacHeader const &> m_txErrorCallback;
   // Process layer 2 TX error notification
   void ProcessTxError (WifiMacHeader const&);
  };
  } //aerorp
  } //ns3
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
