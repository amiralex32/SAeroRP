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
#ifndef AeroRPPacket_H
#define AeroRPPacket_H


#include <iostream>
#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/enum.h"
#include "ns3/simulator.h"
#include "ns3/aerorp-routing-protocol.h"
#include <map>

 namespace ns3 {
 namespace AeroRP {

 enum MessageType
 {
  AERORPTYPE_HELLO = 1, 		//AeroRP helloheader//
  AERORPTYPE_GSGEOLOCATION = 2, 	//AeroRP groundstation location header//
  AERORPTYPE_GSTOPOLGY = 3, 		// AeroRP groundstation topology header//
  AERORPTYPE_AUTHENTICATIONREQUEST = 4, // AeroRP AIR BIRNE NODE AUTHENTICATION REQUEST//
  AERORPTYPE_AUTHENTICATIONREPLY = 5,   // AeroRP AIR BIRNE NODE AUTHENTICATION REPLY//
  AERORPTYPE_SECURE_HELLO = 6, 		//AeroRP secure helloheader//
  AERORPTYPE_SECURE_GSGEOLOCATION = 7, 	//AeroRP secure groundstation location header//
 };
/*
we have 3 types of header for the AeroRP routing protocol the header will depend on the message type id 
0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  AeroRPType |   Flags       | header length |               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|               AeroRP type messages                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/


/*
this is the main type of the headers
*/
  class TypeHeader : public Header
   {
    public:
    ///constructor
    TypeHeader (MessageType t);
   ///\name Header serialization/deserialization
   //\{
   /*THIS func is derived from objectbase(but not virtual why we use it)associate an ns3::TypeId to each object instance
     set and get the attributes registered in the ns3::TypeId.
   */
    static TypeId GetTypeId ();
   /*return the TypeId associated to the most-derived type
   *          of this instance.
   *
   * This method is typically implemented by ns3::Object::GetInstanceTypeId
   * but some classes which derive from ns3::ObjectBase directly
   * have to implement it themselves.
   */
    TypeId GetInstanceTypeId () const;
   /*
    this func is derived from the header class it returns the size of the header it is used by func header::addheader to 
    store a header into the uint8_t buffer(what buffer this means) of a packet it returns the uint8_ts needed to store the full header 
    data by func serialize
    */
    uint32_t GetSerializedSize () const;
    /*
     this func is derived from the header class it is used for adding the header to the packet this method is used by fun
    packet::addheaderto store header in uint8_t buffer it receives an obj start for subclass iterator of class buffer 
    */
    void Serialize (Buffer::Iterator start) const;
    /*
     this func is derived from chunk::header  clases it is used for removing header from a packet it is used by func                  packet::removeheader to recreate the buffer uint8_t it receive a param start as an obj of subclass iterator of class buffer
    */
    uint32_t Deserialize (Buffer::Iterator start);
/*
this func is derived from class chunk::header 
   * \param os output stream
   * This method is used by Packet::Print to print the 
   * content of a trailer as ascii data to a c++ output stream.
   * Although the trailer is free to format its output as it
   * wishes, it is recommended to follow a few rules to integrate
   * with the packet pretty printer: start with flags, small field 
   * values located between a pair of parens. Values should be separated 
   * by whitespace. Follow the parens with the important fields, 
   * separated by whitespace.
   * i.e.: (field1 val1 field2 val2 field3 val3) field4 val4 field5 val5
*/
    void Print (std::ostream &os) const;
 
   /// Return type (to return the gpsr packet type from the messagetype)
   MessageType Get () const
   {
     return m_type;
   }
   /// Check that type if valid
   bool IsValid () const
   {
     return m_valid; 
   }
   //overload operator == that reeceive 2 obj of typeheader
   bool operator== (TypeHeader const & o) const;
 private:
   MessageType m_type;
   bool m_valid;
};

 // i don't know why????//
 std::ostream & operator<< (std::ostream & os, TypeHeader const & h);

class HelloHeader : public Header
{
/*
this header depends on broadcasting hello message to identify neighbors the header will depend on the message type id  
it attaches to the header AeroNP header
0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  AeroRPType |   Flags       | header length |               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| vers| C1| C | type  |priorty|  protocol ID  |  IP ECN/DSCP  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   sourceAN address          |   destination AN address      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   next hop AN address       |   previous hop AN address     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           length            |             flags             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|source dev id| dest dev id   |     NP HEC CRC-16             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                  GS timestamp [optional]                    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|GS fragment number [optional]|  reserved [optional]          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Tx X-coordinate optional         | Tx X-velocity optional  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Tx Y-coordinate optional         | Tx Y-velocity optional  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Tx Z-coordinate optional         | Tx Z-velocity optional  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                  Tx timestamp [optional]                    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Rx X-coordinate optional         | Rx X-velocity optional  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Rx Y-coordinate optional         | Rx Y-velocity optional  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Rx Z-coordinate optional         | Rx Z-velocity optional  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                  Rx timestamp [optional]                    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                 AeroTP payload                              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
 public:
   /// c-torm
   HelloHeader (uint32_t originPosx = 0, uint32_t originPosy = 0 , uint32_t originPosz = 0 ,uint8_t velSign = 0, uint32_t originVelx = 0 , uint32_t originVely = 0 , uint32_t originVelz = 0);
 
   ///\name Header serialization/deserialization
   //\{
   static TypeId GetTypeId ();
   TypeId GetInstanceTypeId () const;
   uint32_t GetSerializedSize () const;
   void Serialize (Buffer::Iterator start) const;
   uint32_t Deserialize (Buffer::Iterator start);
   void Print (std::ostream &os) const;
   //\}
    ///\name Fields
   //\{
   void SetOriginPosx (uint32_t posx)
   {
     m_originxcoordinate = posx;
   }
   uint32_t GetOriginPosx () const
   {
     return m_originxcoordinate;
   }
   void SetOriginPosy (uint32_t posy)
   {
     m_originycoordinate = posy;
   }
   uint32_t GetOriginPosy () const
   {
     return m_originycoordinate;
   }
   void SetOriginPosz (uint32_t posz)
   {
     m_originzcoordinate = posz;
   }
   uint32_t GetOriginPosz () const
   {
     return m_originzcoordinate;
   }
   void SetVelSign (uint8_t velSign)
   {
     m_velocitysign = velSign;
   }
   uint8_t GetVelSign () const
   {
     return m_velocitysign;
   }
   void SetOriginVelx (uint32_t velx)
   {
     m_originxvelocity = velx;
   }
   uint32_t GetOriginVelx () const
   {
     return m_originxvelocity;
   }
   void SetOriginVely (uint32_t vely)
   {
     m_originyvelocity = vely;
   }
   uint32_t GetOriginVely () const
   {
     return m_originyvelocity;
   }
   void SetOriginVelz (uint32_t velz)
   {
     m_originzvelocity = velz;
   }
   uint32_t GetOriginVelz () const
   {
     return m_originzvelocity;
   }
   //\}
 
 
   bool operator== (HelloHeader const & o) const;
 private:

   uint32_t         m_originxcoordinate;          ///< Originator Position x
   uint32_t         m_originycoordinate;          ///< Originator Position y
   uint32_t         m_originzcoordinate;          ///< Originator Position z
   uint8_t          m_velocitysign;		  /// originator velocity x
   uint32_t         m_originxvelocity;		/// originator velocity x
   uint32_t         m_originyvelocity;		/// originator velocity y
   uint32_t         m_originzvelocity;		/// originator velocity z        
 };
 
 std::ostream & operator<< (std::ostream & os, HelloHeader const &);
 

/*
this header depends on broadcasting geolocation information of all the airborne nodes this type only used by the ground station to send ground station advertisment it is used by position table to update all the coordinates of all nodes
0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  		 Type |   Flags       | header length |       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|         node id             |          reserved             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|       X-coordinate                 |    x-velocity          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      Y-coordinate                  |    Y-velocity          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      Z-coordinate                  |    Z-velocity          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|       start time            |        end time               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

  class GSGeoLocationHeader : public Header
  {
   public:
     GSGeoLocationHeader (Ipv4Address node = Ipv4Address (),uint32_t nodePosx = 0, uint32_t nodePosy = 0 , uint32_t nodePosz = 0 , uint32_t nodeVelx = 0 , uint32_t nodeVely = 0 , uint32_t nodeVelz = 0 , int64_t startTime = (int64_t)0 , int64_t endTime = (int64_t)0);

   ///\name Header serialization/deserialization
   //\{
   static TypeId GetTypeId ();
   TypeId GetInstanceTypeId () const;
   uint32_t GetSerializedSize () const;
   void Serialize (Buffer::Iterator start) const;
   uint32_t Deserialize (Buffer::Iterator start);
   void Print (std::ostream &os) const;
   //\}
 
   ///\name Fields
   //\{
  void SetNode (Ipv4Address a)
  { 
    m_node = a; 
  }
  Ipv4Address GetNode () const
  { 
    return m_node;
  }
   //\breif change simulation time to int so it can be saved in buffer
   void SetStartTime ()
   {
     m_startTime = Simulator::Now().GetInteger ();
   }
   int64_t GetStartTime ()
   {
    return m_startTime;
   }
   //\breif change simulation time to int so it can be saved in buffer
   void SetEndTime ()
   {
    m_endTime = (Simulator::Now()+Seconds(5)).GetInteger ();
   }
   int64_t GetEndTime ()
   {
     return m_endTime;
   }

   void SetNodePosx (uint32_t posx)
    {
      m_xcoordinate = posx;
    }
   uint32_t GetNodePosx () const
    {
      return m_xcoordinate;
    }
   void SetNodePosy (uint32_t posy)
    {
      m_ycoordinate = posy;
    }
   uint32_t GetNodePosy () const
    {
      return m_ycoordinate;
    }
   void SetNodePosz (uint32_t posz)
    {
      m_zcoordinate = posz;
    }
   uint32_t GetNodePosz () const
    {
      return m_zcoordinate;
    }
   void SetNodeVelocityx (uint32_t velx)
    {
      m_xvelocity = velx;
    }
   uint32_t GetNodeVelocityx () const
    {
      return m_xvelocity;
    }
   void SetNodeVelocityy (uint32_t vely)
    {
      m_yvelocity = vely;
    }
   uint32_t GetNodeVelocityy () const
    {
      return m_yvelocity;
    }
   void SetNodeVelocityz (uint32_t velz)
    {
      m_zvelocity = velz;
    }
   uint32_t GetNodeVelocityz () const
    {
      return m_zvelocity;
    }

   bool operator== (GSGeoLocationHeader const & o) const;

   private:
    Ipv4Address m_node;			//node 
    uint32_t m_xcoordinate;		//coordinate x for the node
    uint32_t m_ycoordinate;		//coordinate y for the node
    uint32_t m_zcoordinate;		//coordiante z for the node
    uint32_t m_xvelocity;		// velocity x for the node
    uint32_t m_yvelocity;		//velocity y for the node
    uint32_t m_zvelocity;		//velocity z for the node
    int64_t m_startTime;		//start time
    int64_t m_endTime;			//end time

};

 std::ostream & operator<< (std::ostream & os, GSGeoLocationHeader const &);

/*
this header depends on broadcasting broadcast the link information of all the nodes this type only used by the ground station to send ground station advertisment 
0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  AeroRPType |   Flags       | header length |               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|         node1 id            |          node2 id             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        link start time                      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        link end time                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        link cost                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

 class GSTopologyHeader : public Header
  {
   public:
    GSTopologyHeader (Ipv4Address node1 = Ipv4Address (), Ipv4Address node2 = Ipv4Address (), uint32_t linkCost = 0, int64_t linkStartTime = 0 , int64_t linkEndTime = 0 );

///\name Header serialization/deserialization
   //\{
   static TypeId GetTypeId ();
   TypeId GetInstanceTypeId () const;
   uint32_t GetSerializedSize () const;
   void Serialize (Buffer::Iterator start) const;
   uint32_t Deserialize (Buffer::Iterator start);
   void Print (std::ostream &os) const;
   //\}
   ///\name Fields
   //\{
  void SetNode1 (Ipv4Address a)
  { 
    m_node1 = a; 
  }
  Ipv4Address GetNode1 () const
  { 
    return m_node1;
  }
  void SetNode2 (Ipv4Address b)
  { 
    m_node2 = b; 
  }
  Ipv4Address GetNode2 () const
  { 
    return m_node2;
  }
   void SetLinkStartTime ( int64_t startime)
    {
     m_linkStartTime = startime;  
    }
   int64_t GetStartTime () const
   {
    return m_linkStartTime;
   }
   void SetLinkEndTime ( int64_t endtime)
    {
     m_linkEndTime = endtime;  
    }
   int64_t GetEndtTime () const
   {
    return m_linkEndTime;
   }
   void SetLinkCost (uint32_t linkcost)
    {
     m_linkCost = linkcost;  
    }
   uint32_t GetLinkCost ()
    {
     return m_linkCost;  
    }
   bool operator== (GSTopologyHeader const & o) const;

   private:
    Ipv4Address m_node1;			//node 
    Ipv4Address m_node2;			//node
   uint32_t m_linkCost;
   int64_t m_linkStartTime;			//link start time
   int64_t m_linkEndTime;			//link end time

};
 std::ostream & operator<< (std::ostream & os, GSTopologyHeader const &);



 class AuthenticationRequestHeader : public Header
{
 public:
   /// c-torm
   AuthenticationRequestHeader (uint8_t cert [CERT_SIZE] = 0, uint64_t timeStamp = 0 , uint32_t randomNo = 0 ,uint8_t identification = 0 ,uint8_t sign [RSA_KEY_SIZE] = 0);
 
   ///\name Header serialization/deserialization
   //\{
   static TypeId GetTypeId ();
   TypeId GetInstanceTypeId () const;
   uint32_t GetSerializedSize () const;
   void Serialize (Buffer::Iterator start) const;
   uint32_t Deserialize (Buffer::Iterator start);
   void Print (std::ostream &os) const;
   //\}
    ///\name Fields
   //\{
   void SetCertificate (uint8_t cert [CERT_SIZE])
   {
     for (int i = 0 ; i< CERT_SIZE ; i++)
     m_cert [i] = cert[i];
   }
   void GetCertificate (uint8_t cert [CERT_SIZE]) const
   {
     for (int i = 0 ; i< CERT_SIZE ; i++)  
      cert [i] = m_cert [i];
   }
   void SetTimeStamp (uint64_t timeStamp)
   {
     m_timeStamp = timeStamp;
   }
   uint64_t GetTimeStamp () const
   {
     return m_timeStamp;
   }
   void SetRandomNo (uint32_t randomNo)
   {
     m_randomNo = randomNo;
   }
   uint32_t GetRandomNo () const
   {
     return m_randomNo;
   }
   void SetIdentification (uint8_t identification)
   {
     m_identification = identification;
   }
   uint8_t GetIdentification () const
   {
     return m_identification;
   }
   void SetSign (uint8_t sign [RSA_KEY_SIZE])
   {
     for (int i = 0 ; i< RSA_KEY_SIZE ; i++)
     m_sign [i] = sign[i];
   }
   void GetSign (uint8_t sign [RSA_KEY_SIZE]) const
   {
     for (int i = 0 ; i< RSA_KEY_SIZE ; i++)  
      sign [i] = m_sign [i];
   }

   //\}
 
 
   bool operator== (AuthenticationRequestHeader const & o) const;
 private:
   uint8_t          m_cert [CERT_SIZE];           ///< Mycertificate
   uint64_t         m_timeStamp;                  ///< timeStamp
   uint32_t         m_randomNo;                   ///< RandomNo
   uint8_t          m_identification;		  /// IpV4Address
   uint8_t          m_sign [RSA_KEY_SIZE];		  /// IpV4Address

 };
 
 std::ostream & operator<< (std::ostream & os, AuthenticationRequestHeader const &);


 class AuthenticationReplyHeader : public Header
{
 public:
   /// c-torm
   AuthenticationReplyHeader (uint8_t cert [CERT_SIZE] = 0, uint64_t timeStamp = 0 , uint32_t randomNo = 0 , uint32_t oldrandomNo = 0 ,uint8_t identification = 0,uint8_t sharedkey [RSA_KEY_SIZE] = 0 ,uint8_t sign [RSA_KEY_SIZE] = 0);
 
   ///\name Header serialization/deserialization
   //\{
   static TypeId GetTypeId ();
   TypeId GetInstanceTypeId () const;
   uint32_t GetSerializedSize () const;
   void Serialize (Buffer::Iterator start) const;
   uint32_t Deserialize (Buffer::Iterator start);
   void Print (std::ostream &os) const;
   //\}
    ///\name Fields
   //\{
   void SetCertificate (uint8_t cert [CERT_SIZE])
   {
     for (int i = 0 ; i< CERT_SIZE ; i++)
     m_cert [i] = cert[i];
   }
   void GetCertificate (uint8_t cert [CERT_SIZE]) const
   {
     for (int i = 0 ; i< CERT_SIZE ; i++)  
      cert [i] = m_cert [i];
   }
   void SetTimeStamp (uint64_t timeStamp)
   {
     m_timeStamp = timeStamp;
   }
   uint64_t GetTimeStamp () const
   {
     return m_timeStamp;
   }
   void SetRandomNo (uint32_t randomNo)
   {
     m_randomNo = randomNo;
   }
   uint32_t GetRandomNo () const
   {
     return m_randomNo;
   }
   void SetOldRandomNo (uint32_t oldrandomNo)
   {
     m_oldRandomNo = oldrandomNo;
   }
   uint32_t GetOldRandomNo () const
   {
     return m_oldRandomNo;
   }
   void SetIdentification (uint8_t identification)
   {
     m_identification = identification;
   }
   uint8_t GetIdentification () const
   {
     return m_identification;
   }
   void SetSharedKey (uint8_t sharedkey [RSA_KEY_SIZE])
   {
     for (int i = 0 ; i< RSA_KEY_SIZE ; i++)
     m_sharedKey [i] = sharedkey[i];
   }
   void GetSharedKey (uint8_t sharedkey [RSA_KEY_SIZE]) const
   {
     for (int i = 0 ; i< RSA_KEY_SIZE ; i++)
     sharedkey [i] = m_sharedKey[i];
   }
   void SetSign (uint8_t sign [RSA_KEY_SIZE])
   {
     for (int i = 0 ; i< RSA_KEY_SIZE ; i++)
     m_sign [i] = sign[i];
   }
   void GetSign (uint8_t sign [RSA_KEY_SIZE]) const
   {
     for (int i = 0 ; i< RSA_KEY_SIZE ; i++)  
      sign [i] = m_sign [i];
   }

   //\}
 
 
   bool operator== (AuthenticationReplyHeader const & o) const;
 private:

   uint8_t          m_cert [CERT_SIZE];           ///< Mycertificate
   uint64_t         m_timeStamp;                  ///< timeStamp
   uint32_t         m_randomNo;                   ///< RandomNo
   uint32_t         m_oldRandomNo;               ///< RandomNo
   uint8_t          m_identification;		  /// IpV4Address
   uint8_t          m_sharedKey [RSA_KEY_SIZE];   /// SHAREDKEY
   uint8_t          m_sign [RSA_KEY_SIZE];        /// SIGN

 };
 
 std::ostream & operator<< (std::ostream & os, AuthenticationReplyHeader const &);

  class SecureHelloHeader : public Header
  {
   public:
     SecureHelloHeader (uint8_t cipherText [HELLO_PACKET_SIZE] = 0 ,uint8_t tag [TAG_SIZE] = 0);

   ///\name Header serialization/deserialization
   //\{
   static TypeId GetTypeId ();
   TypeId GetInstanceTypeId () const;
   uint32_t GetSerializedSize () const;
   void Serialize (Buffer::Iterator start) const;
   uint32_t Deserialize (Buffer::Iterator start);
   void Print (std::ostream &os) const;
   //\}
 
   ///\name Fields
   //\{
   void SetCipherText (uint8_t cipherText [HELLO_PACKET_SIZE])
   {
     for (int i = 0 ; i< HELLO_PACKET_SIZE ; i++)
     m_cipher [i] = cipherText[i];
   }
   void GetCipherText (uint8_t cipherText [HELLO_PACKET_SIZE]) const
   {
     for (int i = 0 ; i< HELLO_PACKET_SIZE ; i++)  
      cipherText [i] = m_cipher [i];
   }
   void SetTag (uint8_t tag [TAG_SIZE])
   {
     for (int i = 0 ; i< TAG_SIZE ; i++)
     m_tag [i] = tag[i];
   }
   void GetTag (uint8_t tag [TAG_SIZE]) const
   {
     for (int i = 0 ; i< TAG_SIZE ; i++)  
      tag [i] = m_tag [i];
   }

   bool operator== (SecureHelloHeader const & o) const;

   private:
   uint8_t          m_cipher [HELLO_PACKET_SIZE];           ///< Hello cipher
   uint8_t          m_tag [TAG_SIZE];           ///< Hello cipher
};

 std::ostream & operator<< (std::ostream & os, SecureHelloHeader const &);


  class SecureGSGeoLocationHeader : public Header
  {
   public:
     SecureGSGeoLocationHeader (uint8_t cipherText [GS_PACKET_SIZE] = 0 , uint8_t tag [TAG_SIZE] = 0 );

   ///\name Header serialization/deserialization
   //\{
   static TypeId GetTypeId ();
   TypeId GetInstanceTypeId () const;
   uint32_t GetSerializedSize () const;
   void Serialize (Buffer::Iterator start) const;
   uint32_t Deserialize (Buffer::Iterator start);
   void Print (std::ostream &os) const;
   //\}
 
   ///\name Fields
   //\{
   void SetCipherText (uint8_t cipherText [GS_PACKET_SIZE])
   {
     for (int i = 0 ; i< GS_PACKET_SIZE ; i++)
     m_cipher [i] = cipherText[i];
   }
   void GetCipherText (uint8_t cipherText [GS_PACKET_SIZE]) const
   {
     for (int i = 0 ; i< GS_PACKET_SIZE ; i++)  
      cipherText [i] = m_cipher [i];
   }
   void SetTag (uint8_t tag [TAG_SIZE])
   {
     for (int i = 0 ; i< TAG_SIZE ; i++)
     m_tag [i] = tag[i];
   }
   void GetTag (uint8_t tag [TAG_SIZE]) const
   {
     for (int i = 0 ; i< TAG_SIZE ; i++)  
      tag [i] = m_tag [i];
   }

   bool operator== (SecureGSGeoLocationHeader const & o) const;

   private:
   uint8_t          m_cipher [GS_PACKET_SIZE];           ///< Hello cipher
   uint8_t          m_tag [TAG_SIZE];           ///< Hello cipher

};

 std::ostream & operator<< (std::ostream & os, SecureGSGeoLocationHeader const &);

}
}
#endif /* AeroRPPacket_H*/
