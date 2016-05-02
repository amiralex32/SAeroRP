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
 #include "aerorp-packet.h"
 #include "ns3/address-utils.h"
 #include "ns3/packet.h"
 #include "ns3/log.h"
 #include "ns3/nstime.h"

 NS_LOG_COMPONENT_DEFINE ("AeroRPPacket");
 namespace ns3
 {
  namespace AeroRP
  {
   //why we use this ns-obj and what does it mean????//
    NS_OBJECT_ENSURE_REGISTERED (TypeHeader);

   TypeHeader::TypeHeader (MessageType t = AERORPTYPE_HELLO) :
     m_type (t), m_valid (true)
    {
    }
    //what those func means
 TypeId
  TypeHeader::GetTypeId ()
   {
     static TypeId tid = TypeId ("ns3::AeroRP::TypeHeader")
     .SetParent<Header> ()
     .AddConstructor<TypeHeader> ();
      return tid;
   }

 TypeId
   TypeHeader::GetInstanceTypeId () const
   {
     return GetTypeId ();
   }

 uint32_t
   TypeHeader::GetSerializedSize () const
    {
     return 1;
    }

  void
    TypeHeader::Serialize (Buffer::Iterator i) const
    {
      i.WriteU8 ((uint8_t) m_type);
    }

  uint32_t
    TypeHeader::Deserialize (Buffer::Iterator start)
    {
      Buffer::Iterator i = start;
      uint8_t type = i.ReadU8 ();
      m_valid = true;
      switch (type)
      {
       case AERORPTYPE_HELLO:
       case AERORPTYPE_GSGEOLOCATION:
       case AERORPTYPE_GSTOPOLGY:
       case AERORPTYPE_AUTHENTICATIONREQUEST:
       case AERORPTYPE_AUTHENTICATIONREPLY:
       case AERORPTYPE_SECURE_HELLO:	
       case AERORPTYPE_SECURE_GSGEOLOCATION:		
       {
         m_type = (MessageType) type;
         break;
       }
       default:
       m_valid = false;
      }
      uint32_t dist = i.GetDistanceFrom (start);
      NS_ASSERT (dist == GetSerializedSize ());
      return dist;
    } 

  void
   TypeHeader::Print (std::ostream &os) const
    {
     switch (m_type)
      {
       case AERORPTYPE_HELLO:
      {
        os << "hello";
        break;
      }
       case AERORPTYPE_GSGEOLOCATION:
      {
        os << "GSGeolocation";
        break;
      }
       case AERORPTYPE_GSTOPOLGY:
      {
        os << "GSTopology";
        break;
      }
      
      case AERORPTYPE_AUTHENTICATIONREQUEST:
      {
        os << "Authentication Request";
        break;
      }
      
      case AERORPTYPE_AUTHENTICATIONREPLY:	
      {
        os << "Aut5hentication Reply";
        break;
      }
             
      case AERORPTYPE_SECURE_HELLO:	
      {
        os << "Secure Hello";
        break;
      }
             
      case AERORPTYPE_SECURE_GSGEOLOCATION:	
      {
        os << "Secure GSGeolocation";
        break;
      }
       default:
       os << "UNKNOWN_TYPE";
     }
   }

  bool
   TypeHeader::operator== (TypeHeader const & o) const
   {
    return (m_type == o.m_type && m_valid == o.m_valid);
   }

   std::ostream &
   operator<< (std::ostream & os, TypeHeader const & h)
   {
    h.Print (os);
    return os;
   }

 //-----------------------------------------------------------------------------
 // HELLO
 //-----------------------------------------------------------------------------
 HelloHeader::HelloHeader (uint32_t originPosx, uint32_t originPosy ,  uint32_t originPosz ,uint8_t velSign , uint32_t originVelx, uint32_t originVely, uint32_t originVelz)
   : m_originxcoordinate (originPosx),
     m_originycoordinate (originPosy),
     m_originzcoordinate (originPosz),
     m_velocitysign    (velSign),
     m_originxvelocity (originVelx),
     m_originyvelocity (originVely),
     m_originzvelocity (originVelz)
 {
 }
 
 NS_OBJECT_ENSURE_REGISTERED (HelloHeader);
 
 TypeId
 HelloHeader::GetTypeId ()
 {
   static TypeId tid = TypeId ("ns3::AeroRP::HelloHeader")
     .SetParent<Header> ()
     .AddConstructor<HelloHeader> ();
   return tid;
 }
 
 TypeId
 HelloHeader::GetInstanceTypeId () const
 {
   return GetTypeId ();
 }
 
 uint32_t
 HelloHeader::GetSerializedSize () const
 {
   return (6 * sizeof(uint32_t) +sizeof(uint8_t) );
 }
 
 void
 HelloHeader::Serialize (Buffer::Iterator i) const
 {
   NS_LOG_DEBUG ("Serialize X " << m_originxcoordinate << " Y " << m_originycoordinate << "Z" << m_originzcoordinate<< "Vel sign " << static_cast<uint16_t> (m_velocitysign) << "XVel" << m_originxvelocity << "YVel" << m_originyvelocity << "ZVel" << m_originzvelocity);
 
   i.WriteU32 (m_originxcoordinate);
   i.WriteU32 (m_originycoordinate);
   i.WriteU32 (m_originzcoordinate);
   i.WriteU8  (m_velocitysign);
   i.WriteU32 (m_originxvelocity);
   i.WriteU32 (m_originyvelocity);
   i.WriteU32 (m_originzvelocity);
 
 }
 
 uint32_t
 HelloHeader::Deserialize (Buffer::Iterator start)
 {
 
   Buffer::Iterator i = start;
 
   m_originxcoordinate = i.ReadU32 ();
   m_originycoordinate = i.ReadU32 ();
   m_originzcoordinate = i.ReadU32 ();
   m_velocitysign    = i.ReadU8 ();
   m_originxvelocity = i.ReadU32 ();
   m_originyvelocity = i.ReadU32 ();
   m_originzvelocity = i.ReadU32 ();

    NS_LOG_FUNCTION ("hello packet");

   NS_LOG_DEBUG ("Deserialize X " << m_originxcoordinate << " Y " << m_originycoordinate << "Z" << m_originzcoordinate << "VelSign " << static_cast<uint16_t> (m_velocitysign)<< "XVel" << m_originxvelocity << "YVel" << m_originyvelocity << "ZVel" << m_originzvelocity);
 
   uint32_t dist = i.GetDistanceFrom (start);
   NS_ASSERT (dist == GetSerializedSize ());
   return dist;
 }
 
 void
 HelloHeader::Print (std::ostream &os) const
 {
   os << " PositionX: " << m_originxcoordinate
      << " PositionY: " << m_originycoordinate
      << " PositionZ: " << m_originzcoordinate
      << " Velocity Sign: " << m_velocitysign
      << " VelocityX: " << m_originxvelocity
      << " VelocityY: " << m_originyvelocity
      << " VelocityZ: " << m_originzvelocity;
   std::cout<<std::endl;

 for (int j =0 ; j< 4 ; j++)
    {
     printf("0x%.2x ", ((uint8_t *)&m_originxcoordinate)[j]);
    }
 for (int j =0 ; j< 4 ; j++)
    {
     printf("0x%.2x ", ((uint8_t *)&m_originycoordinate)[j]);
    }
 for (int j =0 ; j< 4 ; j++)
    {
     printf("0x%.2x ", ((uint8_t *)&m_originzcoordinate)[j]);
    }
 for (int j =0 ; j< 1 ; j++)
    {
     printf("0x%.2x ", ((uint8_t *)&m_velocitysign)[j]);
    }
 for (int j =0 ; j< 4 ; j++)
    {
     printf("0x%.2x ", ((uint8_t *)&m_originxvelocity)[j]);
    }
 for (int j =0 ; j< 4 ; j++)
    {
     printf("0x%.2x ", ((uint8_t *)&m_originyvelocity)[j]);
    }
 for (int j =0 ; j< 4 ; j++)
    {
     printf("0x%.2x ", ((uint8_t *)&m_originzvelocity)[j]);
    }
std::cout << std::endl;
}
 
 std::ostream &
 operator<< (std::ostream & os, HelloHeader const & h)
 {
   h.Print (os);
   return os;
 }
 
 
 
 bool
 HelloHeader::operator== (HelloHeader const & o) const
 {
   return (m_originxcoordinate == o.m_originxcoordinate && m_originycoordinate == o.m_originycoordinate && m_originzcoordinate == o.m_originzcoordinate && m_velocitysign == o.m_velocitysign && m_originxvelocity == o.m_originxvelocity && m_originyvelocity == o.m_originyvelocity && m_originzvelocity == o.m_originzvelocity);
 }
 
  
 //-----------------------------------------------------------------------------
 // GSGeoLocation
 //-----------------------------------------------------------------------------
 GSGeoLocationHeader::GSGeoLocationHeader (Ipv4Address node, uint32_t nodePosx, uint32_t nodePosy, uint32_t nodePosz, uint32_t nodeVelx, uint32_t nodeVely, uint32_t nodeVelz, int64_t startTime, int64_t endTime)
   : m_node (node),
     m_xcoordinate (nodePosx),
     m_ycoordinate (nodePosy),
     m_zcoordinate (nodePosz),
     m_xvelocity (nodeVelx),
     m_yvelocity (nodeVely),
     m_zvelocity (nodeVelz),
     m_startTime (startTime),
     m_endTime   (endTime)
 {
 }
 
 NS_OBJECT_ENSURE_REGISTERED (GSGeoLocationHeader);
 
 TypeId
 GSGeoLocationHeader::GetTypeId ()
 {
   static TypeId tid = TypeId ("ns3::AeroRP::GSGeoLocationHeader")
     .SetParent<Header> ()
     .AddConstructor<GSGeoLocationHeader> ()
   ;
   return tid;
 }
 
 TypeId
 GSGeoLocationHeader::GetInstanceTypeId () const
 {
   return GetTypeId ();
 }
 
 uint32_t
 GSGeoLocationHeader::GetSerializedSize () const
 {
   return sizeof(Ipv4Address) + 6 * sizeof(uint32_t) + 2 * sizeof(int64_t);
 }
 
 void
 GSGeoLocationHeader::Serialize (Buffer::Iterator i) const
 {
   WriteTo (i, m_node);
   i.WriteU32 (m_xcoordinate);
   i.WriteU32 (m_ycoordinate);
   i.WriteU32 (m_zcoordinate);
   i.WriteU32 (m_xvelocity);
   i.WriteU32 (m_yvelocity);
   i.WriteU32 (m_zvelocity);
   i.WriteU64 (m_startTime);
   i.WriteU64 (m_endTime);
 }
 
 uint32_t
 GSGeoLocationHeader::Deserialize (Buffer::Iterator start)
 {
   Buffer::Iterator i = start;

   ReadFrom (i, m_node);
   m_xcoordinate = i.ReadU32 ();
   m_ycoordinate = i.ReadU32 ();
   m_zcoordinate = i.ReadU32 ();
   m_xvelocity = i.ReadU32 ();
   m_yvelocity = i.ReadU32 ();
   m_zvelocity = i.ReadU32 ();
   m_startTime =  i.ReadU64 ();
   m_endTime =  i.ReadU64 ();

    NS_LOG_FUNCTION ("gs packet");

   uint32_t dist = i.GetDistanceFrom (start);
   NS_ASSERT (dist == GetSerializedSize ());
   return dist;
 }
 
 void
 GSGeoLocationHeader::Print (std::ostream &os) const
 {
   os << "ipv4address:"  << m_node
      << " PositionX: "  << m_xcoordinate
      << " PositionY: "  << m_ycoordinate
      << " PositionZ: "  << m_zcoordinate
      << " VelocityX: "  << m_xvelocity
      << " VelocityY: "  << m_yvelocity
      << " VelocityZ: "  << m_zvelocity
      << " StartTime: "  << m_startTime
      << " EndTime:   "  << m_endTime; 
    std::cout<<std::endl;
 for (int j =0 ; j< 4 ; j++)
    {
     printf("0x%.2x ", ((uint8_t *)&m_node)[j]);
    }
 for (int j =0 ; j< 4 ; j++)
    {
     printf("0x%.2x ", ((uint8_t *)&m_xcoordinate)[j]);
    }
 for (int j =0 ; j< 4 ; j++)
    {
     printf("0x%.2x ", ((uint8_t *)&m_ycoordinate)[j]);
    }
 for (int j =0 ; j< 4 ; j++)
    {
     printf("0x%.2x ", ((uint8_t *)&m_zcoordinate)[j]);
    }
 for (int j =0 ; j< 4 ; j++)
    {
     printf("0x%.2x ", ((uint8_t *)&m_xvelocity)[j]);
    }
 for (int j =0 ; j< 4 ; j++)
    {
     printf("0x%.2x ", ((uint8_t *)&m_yvelocity)[j]);
    }
 for (int j =0 ; j< 4 ; j++)
    {
     printf("0x%.2x ", ((uint8_t *)&m_zvelocity)[j]);
    }
 for (int j =0 ; j< 8 ; j++)
    {
     printf("0x%.2x ", ((uint8_t *)&m_startTime)[j]);
    }
 for (int j =0 ; j< 8 ; j++)
    {
     printf("0x%.2x ", ((uint8_t *)&m_endTime)[j]);
    }
std::cout << std::endl;
 }
 
 std::ostream &
 operator<< (std::ostream & os, GSGeoLocationHeader const & h)
 {
   h.Print (os);
   return os;
 }
 
 
 
 bool
 GSGeoLocationHeader::operator== (GSGeoLocationHeader const & o) const
 {
   return (m_xcoordinate == o.m_xcoordinate && m_ycoordinate == o.m_ycoordinate && m_zcoordinate == o.m_zcoordinate && m_xvelocity == o.m_xvelocity && m_yvelocity == o.m_yvelocity && m_zvelocity == o.m_zvelocity && m_startTime == o.m_startTime && m_endTime == o.m_endTime );
 }

 //-----------------------------------------------------------------------------
 // GSTopologyHeader
 //-----------------------------------------------------------------------------
 GSTopologyHeader::GSTopologyHeader (Ipv4Address node1, Ipv4Address node2, uint32_t linkCost, int64_t linkStartTime, int64_t linkEndTime)
   : m_node1 (node1),
     m_node2 (node2),
     m_linkCost (linkCost),
     m_linkStartTime (linkStartTime),
     m_linkEndTime   (linkEndTime)
 {
 }
 
 NS_OBJECT_ENSURE_REGISTERED (GSTopologyHeader);
 
 TypeId
 GSTopologyHeader::GetTypeId ()
 {
   static TypeId tid = TypeId ("ns3::AeroRP::GSTopologyHeader")
     .SetParent<Header> ()
     .AddConstructor<GSTopologyHeader> ();
   return tid;
 }
 
 TypeId
 GSTopologyHeader::GetInstanceTypeId () const
 {
   return GetTypeId ();
 }
 
 uint32_t
 GSTopologyHeader::GetSerializedSize () const
 {
   return 16;
 }
 
 void
 GSTopologyHeader::Serialize (Buffer::Iterator i) const
 {
   WriteTo (i, m_node1);
   WriteTo (i, m_node2);
   i.WriteU32 (m_linkCost);
   i.WriteU64 (m_linkStartTime);
   i.WriteU64 (m_linkEndTime);
 }
 
 uint32_t
 GSTopologyHeader::Deserialize (Buffer::Iterator start)
 {
   Buffer::Iterator i = start;

   ReadFrom (i, m_node1);
   ReadFrom (i, m_node2);
   m_linkCost = i.ReadU32 ();
   m_linkStartTime =  i.ReadU64 ();
   m_linkEndTime =  i.ReadU64 ();

    NS_LOG_FUNCTION ("gstopology packet");

   uint32_t dist = i.GetDistanceFrom (start);
   NS_ASSERT (dist == GetSerializedSize ());
   return dist;
 }
 
 void
 GSTopologyHeader::Print (std::ostream &os) const
 {
   os << "ipv4address1:"  << m_node1
      << "ipv4address2:"  << m_node2
      << " LinkCost : "  << m_linkCost
      << " StartTime: "  << m_linkStartTime
      << " EndTime:   "  << m_linkEndTime ; 
 }
 
 std::ostream &
 operator<< (std::ostream & os, GSTopologyHeader const & h)
 {
   h.Print (os);
   return os;
 }
 
 
 
 bool
 GSTopologyHeader::operator== (GSTopologyHeader const & o) const
 {
   return (m_node1 == o.m_node1 && m_node2 == o.m_node2 && m_linkCost == o.m_linkCost && m_linkStartTime == o.m_linkStartTime && m_linkEndTime == o.m_linkEndTime );
 }
 
//-----------------------------------------------------------------------------
 // AuthenticationAndKeyExchange
 //-----------------------------------------------------------------------------
 AuthenticationRequestHeader::AuthenticationRequestHeader (uint8_t cert [CERT_SIZE], uint64_t timeStamp, uint32_t randomNo,uint8_t identification,uint8_t sign [RSA_KEY_SIZE])
   :
     m_timeStamp (timeStamp),
     m_randomNo (randomNo),
     m_identification (identification)
 {
     if(cert != NULL){ 
        for (int i = 0 ; i< CERT_SIZE ; i++)
           {
            m_cert [i] = cert[i];
           }
        for (int j = 0 ; j< RSA_KEY_SIZE ; j++)
           {
             m_sign [j] = sign[j];
           }
     }
 }
 
 NS_OBJECT_ENSURE_REGISTERED (AuthenticationRequestHeader);
 
 TypeId
 AuthenticationRequestHeader::GetTypeId ()
 {
   static TypeId tid = TypeId ("ns3::AeroRP::AuthenticationRequestHeader")
     .SetParent<Header> ()
     .AddConstructor<AuthenticationRequestHeader> ()
   ;
   return tid;
 }
 
 TypeId
 AuthenticationRequestHeader::GetInstanceTypeId () const
 {
   return GetTypeId ();
 }
 
 uint32_t
 AuthenticationRequestHeader::GetSerializedSize () const
 {
   return (CERT_SIZE + sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t) + RSA_KEY_SIZE);
 }
 
 void
 AuthenticationRequestHeader::Serialize (Buffer::Iterator i) const
 {
   for (int j =0 ; j< CERT_SIZE ; j++)
    {
      i.WriteU8 (m_cert [j]);
    }
   i.WriteU64 (m_timeStamp);
   i.WriteU32 (m_randomNo);
   i.WriteU8 (m_identification);
   for (int k =0 ; k< RSA_KEY_SIZE ; k++)
    {
      i.WriteU8 (m_sign [k]);
    }
 }
 
 uint32_t
 AuthenticationRequestHeader::Deserialize (Buffer::Iterator start)
 {
   Buffer::Iterator i = start;

for (int j =0 ; j< CERT_SIZE ; j++)
    {
     m_cert [j] = i.ReadU8 ();
    }
   m_timeStamp = i.ReadU64 ();
   m_randomNo = i.ReadU32 ();
   m_identification = i.ReadU8 ();
for (int k =0 ; k< RSA_KEY_SIZE ; k++)
    {
     m_sign [k] = i.ReadU8 ();
    }

    NS_LOG_FUNCTION ("Authentication Request packet");

   uint32_t dist = i.GetDistanceFrom (start);
   NS_ASSERT (dist == GetSerializedSize ());
   return dist;
 }
 
 void
 AuthenticationRequestHeader::Print (std::ostream &os) const
 {
   os << " Certificate: "  << m_cert
      << " TimeStamp:   "  << m_timeStamp
      << " RandomNo :   "  << m_randomNo
      << " Identification:"  << m_identification
      << " sign     :"     << m_sign ; 
 }
 
 std::ostream &
 operator<< (std::ostream & os, AuthenticationRequestHeader const & h)
 {
   h.Print (os);
   return os;
 }
   
 bool
 AuthenticationRequestHeader::operator== (AuthenticationRequestHeader const & o) const
 {
   return (m_cert == o.m_cert && m_timeStamp == o.m_timeStamp && m_randomNo == o.m_randomNo && m_identification == o.m_identification && m_sign == o.m_sign);
 }

 AuthenticationReplyHeader::AuthenticationReplyHeader (uint8_t cert [CERT_SIZE], uint64_t timeStamp, uint32_t randomNo, uint32_t oldrandomNo,uint8_t identification,uint8_t sharedkey [RSA_KEY_SIZE],uint8_t sign [RSA_KEY_SIZE])
   : m_timeStamp (timeStamp),
     m_randomNo (randomNo),
     m_oldRandomNo (oldrandomNo),
     m_identification (identification)
 {
     if(cert != NULL) {
        for (int i = 0 ; i< CERT_SIZE ; i++)
           {
            m_cert [i] = cert[i];
           }
        for (int j = 0 ; j< RSA_KEY_SIZE ; j++)
           {
             m_sharedKey [j] = sharedkey[j];
           }
        for (int k = 0 ; k< RSA_KEY_SIZE ; k++)
           {
             m_sign [k] = sign[k];
           }
     } 
}
 
 NS_OBJECT_ENSURE_REGISTERED (AuthenticationReplyHeader);
 
 TypeId
 AuthenticationReplyHeader::GetTypeId ()
 {
   static TypeId tid = TypeId ("ns3::AeroRP::AuthenticationReplyHeader")
     .SetParent<Header> ()
     .AddConstructor<AuthenticationReplyHeader> ()
   ;
   return tid;
 }
 
 TypeId
 AuthenticationReplyHeader::GetInstanceTypeId () const
 {
   return GetTypeId ();
 }
 
 uint32_t
 AuthenticationReplyHeader::GetSerializedSize () const
 {
   return (CERT_SIZE +sizeof(uint64_t) + 2 * sizeof(uint32_t) + sizeof(uint8_t) +  RSA_KEY_SIZE + RSA_KEY_SIZE );
 }
 
 void
 AuthenticationReplyHeader::Serialize (Buffer::Iterator i) const
 {
  for (int j =0 ; j< CERT_SIZE ; j++)
    {
      i.WriteU8 (m_cert [j]);
    }
   i.WriteU64 (m_timeStamp);
   i.WriteU32 (m_randomNo);
   i.WriteU32 (m_oldRandomNo);
   i.WriteU8 (m_identification);
  for (int k =0 ; k< RSA_KEY_SIZE ; k++)
    {
      i.WriteU8 (m_sharedKey [k]);
    }
  for (int l =0 ; l< RSA_KEY_SIZE ; l++)
    {
      i.WriteU8 (m_sign [l]);
    }

 }
 
 uint32_t
 AuthenticationReplyHeader::Deserialize (Buffer::Iterator start)
 {
   Buffer::Iterator i = start;
for (int j =0 ; j< CERT_SIZE ; j++)
    {
     m_cert [j] = i.ReadU8 ();
    }
   m_timeStamp = i.ReadU64 ();
   m_randomNo = i.ReadU32 ();
   m_oldRandomNo = i.ReadU32 ();
   m_identification = i.ReadU8 ();
for (int k =0 ; k< RSA_KEY_SIZE ; k++)
    {
     m_sharedKey [k] = i.ReadU8 ();
    }
for (int l =0 ; l< RSA_KEY_SIZE ; l++)
    {
     m_sign [l] = i.ReadU8 ();
    }

   uint32_t dist = i.GetDistanceFrom (start);
   NS_ASSERT (dist == GetSerializedSize ());
   return dist;
 }
 
 void
 AuthenticationReplyHeader::Print (std::ostream &os) const
 {
   os << "\nCert: ";
   for (int j =0 ; j< CERT_SIZE ; j++)
    {
     printf("0x%.2x ", m_cert[j]);
    }
   os << "\n";	

   os << " TimeStamp:   "  << m_timeStamp
      << " RandomNo :   "  << m_randomNo
      << " OldRandomNo :   "  << m_oldRandomNo
      << " Identification:"  << m_identification;
   os << "\n";	
   os << "Key: ";
   for (int j =0 ; j< RSA_KEY_SIZE ; j++)
    {
     printf("0x%.2x ", m_sharedKey[j]);
    }
   os << "\n";	
   os << "Sign: ";
   for (int j =0 ; j< RSA_KEY_SIZE ; j++)
    {
     printf("0x%.2x ", m_sign[j]);
    }
   os << "\n";	

 }
 
 std::ostream &
 operator<< (std::ostream & os, AuthenticationReplyHeader const & h)
 {
   h.Print (os);
   return os;
 }
   
 bool
 AuthenticationReplyHeader::operator== (AuthenticationReplyHeader const & o) const
 {
   return (m_cert == o.m_cert && m_timeStamp == o.m_timeStamp && m_randomNo == o.m_randomNo && m_oldRandomNo == o.m_oldRandomNo && m_identification == o.m_identification && m_sharedKey == o.m_sharedKey && m_sign == o.m_sign);
 }
 
//-----------------------------------------------------------------------------
 //  SECURE HELLO
 //-----------------------------------------------------------------------------
 SecureHelloHeader :: SecureHelloHeader (uint8_t cipherText [HELLO_PACKET_SIZE] , uint8_t tag [TAG_SIZE])

 {
     if(cipherText != NULL) 
     {
        for (int i = 0 ; i< HELLO_PACKET_SIZE ; i++)
           {
            m_cipher [i] = cipherText[i];
           }
        for (int i = 0 ; i< TAG_SIZE ; i++)
           {
            m_tag [i] = tag[i];
           }
     } 
 }
 
 NS_OBJECT_ENSURE_REGISTERED (SecureHelloHeader);
 
 TypeId
 SecureHelloHeader::GetTypeId ()
 {
   static TypeId tid = TypeId ("ns3::AeroRP::SecureHelloHeader")
     .SetParent<Header> ()
     .AddConstructor<SecureHelloHeader> ();
   return tid;
 }
 
 TypeId
 SecureHelloHeader::GetInstanceTypeId () const
 {
   return GetTypeId ();
 }
 
 uint32_t
 SecureHelloHeader::GetSerializedSize () const
 {
   return (HELLO_PACKET_SIZE + TAG_SIZE);
 }
 
 void
 SecureHelloHeader::Serialize (Buffer::Iterator i) const
 {
  for (int k =0 ; k< HELLO_PACKET_SIZE ; k++)
    {
      i.WriteU8 (m_cipher [k]);
    }
  for (int k =0 ; k< TAG_SIZE ; k++)
    {
      i.WriteU8 (m_tag [k]);
    }
 }
 
 uint32_t
 SecureHelloHeader::Deserialize (Buffer::Iterator start)
 {
 
   Buffer::Iterator i = start;
for (int k =0 ; k< HELLO_PACKET_SIZE ; k++)
    {
     m_cipher [k] = i.ReadU8 ();
    }
for (int k =0 ; k< TAG_SIZE ; k++)
    {
     m_tag [k] = i.ReadU8 ();
    }

    NS_LOG_FUNCTION (" secure hello packet");
 
   uint32_t dist = i.GetDistanceFrom (start);
   NS_ASSERT (dist == GetSerializedSize ());
   return dist;
 }
 
 void
 SecureHelloHeader::Print (std::ostream &os) const
 {
  os << "\nCipher: ";
   for (int j =0 ; j< HELLO_PACKET_SIZE ; j++)
    {
     printf("0x%.2x ", m_cipher[j]);
    }
  os << "\nTag: ";
   for (int j =0 ; j< TAG_SIZE ; j++)
    {
     printf("0x%.2x ", m_tag[j]);
    }
   os << "\n";			
 }
 
 std::ostream &
 operator<< (std::ostream & os, SecureHelloHeader const & h)
 {
   h.Print (os);
   return os;
 }
 
 bool
 SecureHelloHeader::operator== (SecureHelloHeader const & o) const
 {
   return (m_cipher == o.m_cipher && m_tag == o.m_tag );
 }
 
//-----------------------------------------------------------------------------
 //  SECURE GSGeoLocation
 //-----------------------------------------------------------------------------
     SecureGSGeoLocationHeader :: SecureGSGeoLocationHeader (uint8_t cipherText [GS_PACKET_SIZE], uint8_t tag [TAG_SIZE] )

 {
     if(cipherText != NULL) 
     {
        for (int i = 0 ; i< GS_PACKET_SIZE ; i++)
           {
            m_cipher [i] = cipherText[i];
           }
        for (int i = 0 ; i< TAG_SIZE ; i++)
           {
            m_tag [i] = tag[i];
           }
     } 
 }
 
 NS_OBJECT_ENSURE_REGISTERED (SecureGSGeoLocationHeader);
 
 TypeId
 SecureGSGeoLocationHeader::GetTypeId ()
 {
   static TypeId tid = TypeId ("ns3::AeroRP::SecureGSGeoLocationHeader")
     .SetParent<Header> ()
     .AddConstructor<SecureGSGeoLocationHeader> ();
   return tid;
 }
 
 TypeId
 SecureGSGeoLocationHeader::GetInstanceTypeId () const
 {
   return GetTypeId ();
 }
 
 uint32_t
 SecureGSGeoLocationHeader::GetSerializedSize () const
 {
   return (GS_PACKET_SIZE + TAG_SIZE);
 }
 
 void
 SecureGSGeoLocationHeader::Serialize (Buffer::Iterator i) const
 {
  for (int k =0 ; k< GS_PACKET_SIZE ; k++)
    {
      i.WriteU8 (m_cipher [k]);
    }
  for (int k =0 ; k< TAG_SIZE ; k++)
    {
      i.WriteU8 (m_tag [k]);
    } 
 }
 
 uint32_t
 SecureGSGeoLocationHeader::Deserialize (Buffer::Iterator start)
 {
 
   Buffer::Iterator i = start;
for (int k =0 ; k< GS_PACKET_SIZE ; k++)
    {
     m_cipher [k] = i.ReadU8 ();
    }
for (int k =0 ; k< TAG_SIZE ; k++)
    {
     m_tag [k] = i.ReadU8 ();
    }

    NS_LOG_FUNCTION (" secure GS packet");
 
   uint32_t dist = i.GetDistanceFrom (start);
   NS_ASSERT (dist == GetSerializedSize ());
   return dist;
 }
 
 void
 SecureGSGeoLocationHeader::Print (std::ostream &os) const
 {
  os << "\nCipher: ";
   for (int j =0 ; j< GS_PACKET_SIZE ; j++)
    {
     printf("0x%.2x ", m_cipher[j]);
    }
  os << "\nTag: ";
   for (int j =0 ; j< TAG_SIZE ; j++)
    {
     printf("0x%.2x ", m_tag[j]);
    }
   os << "\n";	
 }
 
 std::ostream &
 operator<< (std::ostream & os, SecureGSGeoLocationHeader const & h)
 {
   h.Print (os);
   return os;
 }
 
 bool
 SecureGSGeoLocationHeader::operator== (SecureGSGeoLocationHeader const & o) const
 {
   return (m_cipher == o.m_cipher && m_tag == o.m_tag);
 }
 
 }
 }

