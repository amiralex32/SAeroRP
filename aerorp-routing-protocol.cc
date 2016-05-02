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
#define NS_LOG_APPEND_CONTEXT                                           \
   if (m_ipv4) { std::clog << "[node " << m_ipv4->GetObject<Node> ()->GetId () << "] "<<"[Time "<<Simulator::Now ().GetSeconds ()<<"]"; }

 
 #include "aerorp-routing-protocol.h"
 #include "ns3/log.h"
 #include "ns3/boolean.h"
 #include "ns3/random-variable.h"
 #include "ns3/inet-socket-address.h"
 #include "ns3/trace-source-accessor.h"
 #include "ns3/udp-socket-factory.h"
 #include "ns3/wifi-net-device.h"
 #include "ns3/adhoc-wifi-mac.h"
 #include "ns3/mobility-model.h"
 #include "ns3/ipv4-routing-protocol.h"
 #include "ns3/output-stream-wrapper.h"
 #include "ns3/ip-l4-protocol.h"
 #include <algorithm>
 #include <limits>
 #include <complex>
 
#define GROUND_STATION_ADDRESS "192.168.1.1"
 #define aerorp_LS_GOD 0
 
 #define aerorp_LS_RLS 1
 
 NS_LOG_COMPONENT_DEFINE ("AeroRPRoutingProtocol");
 
 namespace ns3 {
 namespace AeroRP {
 
 std::ostream& 
 operator<< (std::ostream& os, const Packet &packet)
 {
  packet.Print (os);
  return os;
 }
 
 struct DeferredRouteOutputTag : public Tag
 {
   /// Positive if output device is fixed in RouteOutput
   uint32_t m_isCallFromL3;
 
   DeferredRouteOutputTag () : Tag (),
                               m_isCallFromL3 (0)
   {
   }
 
   static TypeId GetTypeId ()
   {
     static TypeId tid = TypeId ("ns3::AeroRP::DeferredRouteOutputTag").SetParent<Tag> ();
     return tid;
   }
 
   TypeId  GetInstanceTypeId () const
   {
     return GetTypeId ();
   }
 
   uint32_t GetSerializedSize () const
   {
     return sizeof(uint32_t);
   }
 
   void  Serialize (TagBuffer i) const
   {
     i.WriteU32 (m_isCallFromL3);
   }
 
   void  Deserialize (TagBuffer i)
   {
     m_isCallFromL3 = i.ReadU32 ();
   }
 
   void  Print (std::ostream &os) const
   {
     os << "DeferredRouteOutputTag: m_isCallFromL3 = " << m_isCallFromL3;
   }
 };
 
 
 
 /********** Miscellaneous constants **********/
 
 /// Maximum allowed jitter.
 #define AERORPAuthentication_MAXJITTER          (AuthenticationInterval.GetSeconds ())
 /// Random number between [(-AERORPAuthentication_MAXJITTER)-AERORPAuthentication_MAXJITTER] used to jitter Authenication packet transmission.
 #define AUTHJITTER (Seconds (UniformVariable ().GetValue (-AERORPAuthentication_MAXJITTER, AERORPAuthentication_MAXJITTER))) 
 #define FIRSTAUTH_JITTER (Seconds (UniformVariable ().GetValue (0, AERORPAuthentication_MAXJITTER))) //first Hello can not be in the past, used only on SetIpv4

 /// Maximum allowed jitter.
 #define AERORP_MAXJITTER          (HelloInterval.GetSeconds () / 2)
 /// Random number between [(-aerorp_MAXJITTER)-aerorp_MAXJITTER] used to jitter HELLO packet transmission.
 #define JITTER (Seconds (UniformVariable ().GetValue (-AERORP_MAXJITTER, AERORP_MAXJITTER))) 
 #define FIRST_JITTER (Seconds (UniformVariable ().GetValue (0, AERORP_MAXJITTER))) //first Hello can not be in the past, used only on SetIpv4
 
 
 
 NS_OBJECT_ENSURE_REGISTERED (AeroRoutingProtocol);
 
 /// UDP Port for aerorp control traffic, not defined by IANA yet
 const uint32_t AeroRoutingProtocol::AERORP_PORT = 666;
 
 AeroRoutingProtocol::AeroRoutingProtocol ()
   : HelloInterval (Seconds (2)),
     MaxQueueLen (28160000),
     MaxQueueTime (Seconds (10000)),
     m_queue (MaxQueueLen, MaxQueueTime),
     HelloIntervalTimer (Timer::CANCEL_ON_DESTROY),
     CheckQueueTimer (Timer::CANCEL_ON_DESTROY),
     GSTimer (Timer::CANCEL_ON_DESTROY),
     GSInterval(Seconds (5)),
     m_isGroundStation(false),
     m_isAttacker(false),
     m_SecurityState(false),
     m_AttackState(false),
     AuthenticationInterval(Seconds (3))
 {
   /*
    * SetPrivateKey (2048 , RSA_F4 , NULL , NULL);
    * SetPublicKey (m_privatekey);
    * SetCertificate (1 , amir , m_publicKey);
    */
   m_neighbors = CreateObject<NeighborTable> ();

 }
 
 TypeId
 AeroRoutingProtocol::GetTypeId (void)
 {
   static TypeId tid = TypeId ("ns3::AeroRP::AeroRoutingProtocol")
     .SetParent<Ipv4RoutingProtocol> ()
     .AddConstructor<AeroRoutingProtocol> ()
     .AddAttribute ("HelloInterval", "HELLO messages emission interval.",
                    TimeValue (Seconds (1)),
                    MakeTimeAccessor (&AeroRoutingProtocol::HelloInterval),
                    MakeTimeChecker ())
     .AddAttribute ("LocationServiceName", "Indicates wich Location Service is enabled",
                    EnumValue (aerorp_LS_GOD),
                    MakeEnumAccessor (&AeroRoutingProtocol::LocationServiceName),
                    MakeEnumChecker (aerorp_LS_GOD, "GOD",
                                     aerorp_LS_RLS, "RLS"))
     .AddTraceSource ("RxHello", "Receive Hello packet.",
        	    MakeTraceSourceAccessor (&AeroRoutingProtocol::m_rxHelloTrace))
     .AddTraceSource ("RxGstation", "Receive Gstation packet.",
        	    MakeTraceSourceAccessor (&AeroRoutingProtocol::m_rxGsTrace))
     .AddTraceSource ("TxGstation", "Send Gstation packet.",
        	    MakeTraceSourceAccessor (&AeroRoutingProtocol::m_txGsTrace))
     .AddTraceSource ("TxHello", "Send Hello packet.",
		    MakeTraceSourceAccessor (&AeroRoutingProtocol::m_txHelloTrace))
    .AddTraceSource ("SRxHello", "Receive SHello packet.",
        	    MakeTraceSourceAccessor (&AeroRoutingProtocol::m_rxSHelloTrace))
     .AddTraceSource ("SRxGstation", "Receive SGstation packet.",
        	    MakeTraceSourceAccessor (&AeroRoutingProtocol::m_rxSGsTrace))
     .AddTraceSource ("STxGstation", "Send SGstation packet.",
        	    MakeTraceSourceAccessor (&AeroRoutingProtocol::m_txSGsTrace))
     .AddTraceSource ("STxHello", "Send SHello packet.",
		    MakeTraceSourceAccessor (&AeroRoutingProtocol::m_txSHelloTrace));
   return tid;
 }
 
 AeroRoutingProtocol::~AeroRoutingProtocol ()
 {
   AuthenticationDestroy();
 }
 
 ///what is the purbose of using this method to do what?????
 void
 AeroRoutingProtocol::DoDispose ()
 {
   m_ipv4 = 0;
   m_godGroundStation = 0;
   m_neighbors = 0;
   Ipv4RoutingProtocol::DoDispose ();
 }
 /*socket is an API based loosely on the BSD Socket API.
  * A few things to keep in mind about this type of socket:
 * - it uses ns-3 API constructs such as class ns3::Address instead of
 *   C-style structs
 * - in contrast to the original BSD socket API, this API is asynchronous:
 *   it does not contain blocking calls.  Sending and receiving operations
 *   must make use of the callbacks provided. 
 * - It also uses class ns3::Packet as a fancy byte buffer, allowing 
 *   data to be passed across the API using an ns-3 Packet instead of 
 *   a raw data pointer.
 * - Not all of the full POSIX sockets API is supported
 *
 * Other than that, it tries to stick to the BSD API to make it 
 * easier for those who know the BSD API to use this API.
 *\ used to define route parameters such as gateway,(source,dst) IP,netdevice
 */
 Ptr<Ipv4Route>
 AeroRoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header,
                               Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
 {
   NS_LOG_FUNCTION (this << header << (oif ? oif->GetIfIndex () : 0));
  if (m_SecurityState == true)
   {
    unsigned char srcAdd[4];
    unsigned char dstAdd[4];
    Ipv4Address src = m_ipv4->GetAddress (1, 0).GetLocal ();
    Ipv4Address dst = header.GetDestination ();
    dst.Serialize(dstAdd);
    src.Serialize(srcAdd);
    unsigned char tag[TAG_SIZE];
    unsigned char aad [AAD_LENGTH];

   //cout<<"set aad...\n";
    for (int i = 0 ; i < (AAD_LENGTH-4) ; i++)
    {
      aad[i] = srcAdd[i];
    }
    for (int j = (AAD_LENGTH-4) ; j < AAD_LENGTH ; j++)
    {
      aad[j] = dstAdd[j-4];
    }
    //cout<<"Aditional Auth Data \n";
    //for (int j = 0 ; j < AAD_LENGTH ; j++)
   // {
   //  printf("0x%.2x ", aad[j]);
   // }
   // cout<<endl;
   //cout<<"set iv...\n";
   uint64_t packetid = p->GetUid();
    unsigned char iv [IV_LENGTH];
    for (int i = 0 ; i < 8 ; i++)
    {
      iv[i] = ((uint8_t *)&packetid)[i];

    }
    for (int i = 8 ; i < IV_LENGTH ; i++)
    {
      iv[i] = i;
    } 
   // for (int i = 0 ; i < IV_LENGTH ; i++)
   // {
   //  printf("0x%.2x ", iv[i]);
   // }
   // cout<<endl;

    //TypeHeader tHeader (AERORPTYPE_HELLO);
   //NS_LOG_FUNCTION (this << " theader " << tHeader.Get());
    //if (tHeader.Get() == AERORPTYPE_HELLO)
    if ((p->GetSize ()) == (HELLO_PACKET_SIZE + 1))
   {

    //remove the old header type 1 byte
    TypeHeader hHeader (AERORPTYPE_HELLO);
    p->RemoveHeader(hHeader); 

    unsigned char cipher[HELLO_PACKET_SIZE];    

    uint8_t pText[HELLO_PACKET_SIZE];
    //Buffer buf;
    p->CopyData(pText,HELLO_PACKET_SIZE);
         
   //cout << "After copying...\n";
   //for (int j =0 ; j< HELLO_PACKET_SIZE ; j++)
    //{
     //printf("0x%.2x ", pText[j]);
    //}
    //cout << endl;

   m_gcm.Encryption(pText,HELLO_PACKET_SIZE,aad,AAD_LENGTH,m_sharedKey,SHARED_KEY_SIZE,iv,IV_LENGTH,cipher,tag);    

   //cout << "After encryption cipher...\n";
   //for (int j =0 ; j< HELLO_PACKET_SIZE ; j++)
    //{
    // printf("0x%.2x ", cipher[j]);
    //}
    //cout << endl;

   //cout << "After encryption tag...\n";
   //for (int j =0 ; j< TAG_SIZE ; j++)
    //{
     //printf("0x%.2x ", tag[j]);
    //}
    //cout << endl;

    HelloHeader hdr;
    p->RemoveHeader(hdr);
    //add the new header
    TypeHeader sHeader (AERORPTYPE_SECURE_HELLO);
    SecureHelloHeader secureHelloHeader(cipher, tag);
    p->AddHeader (secureHelloHeader);
    p->AddHeader (sHeader);
    //cout << "After new packet...\n";
    //p->Print(cout);
   }
   if ((p->GetSize ()) == (GS_PACKET_SIZE + 1))
   { 
    //remove the old header type 1 byte
    TypeHeader gHeader (AERORPTYPE_GSGEOLOCATION);
    p->RemoveHeader(gHeader); 
   //NS_LOG_FUNCTION (this << " theader after " << tHeader.Get());

    unsigned char cipher[GS_PACKET_SIZE];    

    uint8_t pText[GS_PACKET_SIZE];
    //Buffer buf;
    p->CopyData(pText,GS_PACKET_SIZE);
         
  // cout << "After copying...\n";
  // for (int j =0 ; j< GS_PACKET_SIZE ; j++)
  //  {
  //   printf("0x%.2x ", pText[j]);
  //  }
  // cout << endl;

    m_gcm.Encryption(pText,GS_PACKET_SIZE,aad,AAD_LENGTH,m_sharedKey,SHARED_KEY_SIZE,iv,IV_LENGTH,cipher,tag);    

  // cout << "After encryption cipher...\n";
  // for (int j =0 ; j< GS_PACKET_SIZE ; j++)
  //  {
  //   printf("0x%.2x ", cipher[j]);
  //  }
  //  cout << endl;

  // cout << "After encryption tag...\n";
  // for (int j =0 ; j< TAG_SIZE ; j++)
  //  {
  //   printf("0x%.2x ", tag[j]);
  //  }
  //  cout << endl;

    GSGeoLocationHeader ghdr;
    p->RemoveHeader(ghdr);
    //add the new header
    TypeHeader sHeader (AERORPTYPE_SECURE_GSGEOLOCATION);
    SecureGSGeoLocationHeader secureGSHeader(cipher, tag);
    p->AddHeader (secureGSHeader);
    p->AddHeader (sHeader);
    //cout << "After new packet...\n";
    //p->Print(cout);
}

    if ((p->GetSize ()) == PACKET_DATA_LEN_BEFORE_ENCRYPTON)
     {
      unsigned char totalSecurePacket[PACKET_DATA_LEN_AFTER_ENCRYPTON];      
      unsigned char pText[PACKET_DATA_LEN_BEFORE_ENCRYPTON];
      unsigned char cipher[PACKET_DATA_LEN_BEFORE_ENCRYPTON];
      p->CopyData(pText,PACKET_DATA_LEN_BEFORE_ENCRYPTON);
   //cout << " put the packet in array...\n";
   //for (int j =0 ; j< PACKET_DATA_LEN_BEFORE_ENCRYPTON ; j++)
   // {
     //printf("0x%.2x ", pText[j]);
   // }
   // cout << endl;
    
    m_gcm.Encryption(pText,PACKET_DATA_LEN_BEFORE_ENCRYPTON,aad,AAD_LENGTH,m_sharedKey,SHARED_KEY_SIZE,iv,IV_LENGTH,cipher,tag); 

   //cout << "After encryption cipher...\n";
   //for (int j =0 ; j< PACKET_DATA_LEN_BEFORE_ENCRYPTON  ; j++)
   // {
     //printf("0x%.2x ", cipher[j]);
   // }
   //cout << endl;

   //cout << "After encryption tag...\n";
   //for (int j =0 ; j< TAG_SIZE ; j++)
   // {
    // printf("0x%.2x ", tag[j]);
   // }
    //cout << endl; 

   for (int j =0 ; j< PACKET_DATA_LEN_BEFORE_ENCRYPTON ; j++)
    {
      totalSecurePacket[j] = cipher[j];
    }
   for (int j =PACKET_DATA_LEN_BEFORE_ENCRYPTON ; j< PACKET_DATA_LEN_AFTER_ENCRYPTON ; j++)
    {
      totalSecurePacket[j] = tag[j-PACKET_DATA_LEN_BEFORE_ENCRYPTON];
    }
    Ptr<Packet> secureDataPacket = Create<Packet> (totalSecurePacket, PACKET_DATA_LEN_AFTER_ENCRYPTON);
    //p->Packet(secureDataPacket);
    (*p) = (*secureDataPacket);

   unsigned char test[PACKET_DATA_LEN_AFTER_ENCRYPTON];  
   p->CopyData(test,PACKET_DATA_LEN_AFTER_ENCRYPTON);
   //cout << " put the packet after equal in array...\n";
   //for (int j =0 ; j< PACKET_DATA_LEN_AFTER_ENCRYPTON ; j++)
   // {
    // printf("0x%.2x ", test[j]);
   // }
    //cout << endl;
  
      
     }

}
   if (!p)
     {
       return LoopbackRoute (header, oif);     // later
     }
   if (m_socketAddresses.empty ())
     {
       sockerr = Socket::ERROR_NOROUTETOHOST;
       NS_LOG_LOGIC ("No AeroRP interfaces");
       Ptr<Ipv4Route> route;
       return route;
     }
   sockerr = Socket::ERROR_NOTERROR;
   Ptr<Ipv4Route> route = Create<Ipv4Route> ();
   ///what does this mean????
   Ipv4Address dst = header.GetDestination ();
   Ipv4Address src = m_ipv4->GetAddress (1, 0).GetLocal ();
   Ipv4Address authSrc = m_ipv4->GetAddress (2, 0).GetLocal ();
  NS_LOG_DEBUG ("Packet Size: " << p->GetSize ()<< ", Packet id: " << p->GetUid () << ", Destination address in Packet: " << dst<<", Source address in Packet: "<<header.GetSource ()<<" source is "<<src);
 
   Vector dstPos = Vector (0, 0, 0);
   Vector myPos;
   Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();
   myPos = MM->GetPosition ();

   Vector myVel;
   myVel = MM->GetVelocity (); 
 
 
   Ipv4Address nextHop;

   if (dst == m_ipv4->GetAddress (1, 0).GetBroadcast ())
     {
        return LoopbackRoute (header, oif);
     
      }

   /*
    * Because addresses can be removed, the addressIndex is not guaranteed to be static across calls  	  * to this method
    * param 1 for Interface number of an Ipv4 interface
    * param 0 for index of Ipv4InterfaceAddress
    * return Ipv4InterfaceAddress associated to the interface and addressIndex
    */

   if ( (dst == GROUND_STATION_ADDRESS) && (m_SecurityState == true))
     {
       route->SetDestination (dst);
       route->SetSource (m_ipv4->GetAddress (2, 0).GetLocal ());
       route->SetGateway (dst);
       route->SetOutputDevice (m_ipv4->GetNetDevice (2));
       return route;
     }

    if ((authSrc == GROUND_STATION_ADDRESS) && (m_SecurityState == true) && (!(dst == m_ipv4->GetAddress (2, 0).GetBroadcast ())))
    //if ((authSrc == GROUND_STATION_ADDRESS) && (m_SecurityState == true))
      {
 NS_LOG_FUNCTION (this << "GS is sender && not broadcast");
       route->SetDestination (dst);
       route->SetSource (m_ipv4->GetAddress (2, 0).GetLocal ());
       route->SetGateway (dst);
       route->SetOutputDevice (m_ipv4->GetNetDevice (2));
       return route;
      }

   if (!(dst == m_ipv4->GetAddress (1, 0).GetBroadcast ()))
     {
       dstPos = m_nodesPosition.GetPosition (dst);
     
      }
      if (m_neighbors->BestNeighbor (dstPos, myPos, myVel)== m_ipv4->GetAddress (1, 0).GetLocal ()) 
  /// if (CalculateDistance (dstPos, m_nodesPosition.GetInvalidPosition ()) == 0 && m_nodesPosition.IsInSearch (dst))
     {
 NS_LOG_FUNCTION (this << "i am best neighbor so add tag and make loopback and put in queue");
       ///make tag for the packet that unreachable
       DeferredRouteOutputTag tag;
       ///Search a matching tag and call Tag::Deserialize if it is found.
       ///true if the requested tag is found, false otherwise
       /// param tag to search for
       if (!p->PeekPacketTag (tag))
         {
           p->AddPacketTag (tag);
         }
       /*
        * return pointer to route to define route parameters such as (source,dst) IP, gateway
        * o/p device
        */
       return LoopbackRoute (header, oif);
      }
 
 
   if(m_neighbors->isNeighbour (dst))
     {
       nextHop = dst;
       NS_LOG_DEBUG (" best neighbor is neighbor : " << nextHop);
     }
   else
     {
       nextHop = m_neighbors->BestNeighbor (dstPos, myPos, myVel);
       NS_LOG_DEBUG ("calculate best neighbor : " << nextHop);
     }
 
 
   if (nextHop != Ipv4Address::GetZero ())
     {
       NS_LOG_DEBUG ("Destination: " << dst);
 
       route->SetDestination (dst);
       if (header.GetSource () == Ipv4Address ("102.102.102.102"))
         {
           route->SetSource (m_ipv4->GetAddress (1, 0).GetLocal ());
       NS_LOG_DEBUG ("header source is 102  : " << header.GetSource ());
         }
       else
         {
           route->SetSource (header.GetSource ());
       NS_LOG_DEBUG ("header source is  : " << header.GetSource ());
         }
       route->SetGateway (nextHop);
       route->SetOutputDevice (m_ipv4->GetNetDevice (1));
       route->SetDestination (header.GetDestination ());
       NS_ASSERT (route != 0);
       NS_LOG_DEBUG ("Exist route to " << route->GetDestination () << " from interface " << route->GetSource ());
       if (oif != 0 && route->GetOutputDevice () != oif)
         {
           NS_LOG_DEBUG ("Output device doesn't match. Dropped.");
           sockerr = Socket::ERROR_NOROUTETOHOST;
           return Ptr<Ipv4Route> ();
         }
       return route;
     }
   else
     {
       DeferredRouteOutputTag tag;
       if (!p->PeekPacketTag (tag))
         {
           p->AddPacketTag (tag); 
         }
       return LoopbackRoute (header, oif);     //in RouteInput the recovery-mode is called
     }
 
 }
 
 
/*
 *\route an i/p packet to be locally delivered or forward
 */
 bool 
 AeroRoutingProtocol::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev, UnicastForwardCallback ucb, MulticastForwardCallback mcb,LocalDeliverCallback lcb,ErrorCallback ecb)
 {
 
 NS_LOG_FUNCTION (this << p->GetUid () << header.GetDestination () << header.GetSource () << idev->GetAddress ());
   unsigned char srcAdd[4];
    unsigned char dstAdd[4];
    Ipv4Address checkedSrc = header.GetSource ();
    Ipv4Address checkedDst = header.GetDestination ();
    checkedSrc.Serialize(srcAdd);
    checkedDst.Serialize(dstAdd);


   for (int i = 0 ; i < (AAD_LENGTH-4) ; i++)
    {
      GCMAADTOBECHECKED[i] = srcAdd[i];
    }
    for (int j = (AAD_LENGTH-4) ; j < AAD_LENGTH ; j++)
    {
      GCMAADTOBECHECKED[j] = dstAdd[j-4];
    }

  //cout<<"Aditional Auth Data to be checked in routeinput \n";
   // for (int j = 0 ; j < AAD_LENGTH ; j++)
   // {
     //printf("0x%.2x ", GCMAADTOBECHECKED[j]);
    //}
    //cout<<endl;

   //cout<<"set iv...\n";
   //cout<<"set iv...\n";
   uint64_t packetid = p->GetUid();
    unsigned char iv [IV_LENGTH];
    for (int i = 0 ; i < 8 ; i++)
    {
      iv[i] = ((uint8_t *)&packetid)[i];

    }
    for (int i = 8 ; i < IV_LENGTH ; i++)
    {
      iv[i] = i;
    } 
   // for (int i = 0 ; i < IV_LENGTH ; i++)
   // {
   //  printf("0x%.2x ", iv[i]);
   // }
   // cout<<endl;

 NS_LOG_FUNCTION (this << "print input packet");
   //cout<<endl;
   //p->Print(cout);


   if (m_socketAddresses.empty ())
     {
       //NS_LOG_LOGIC ("No aerorp interfaces");
       return false;
     }
   NS_ASSERT (m_ipv4 != 0);
   NS_ASSERT (p != 0);
   // Check if input device supports IP
   NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);
   int32_t iif = m_ipv4->GetInterfaceForDevice (idev);
   Ipv4Address dst = header.GetDestination ();
   Ipv4Address origin = header.GetSource ();
 
   DeferredRouteOutputTag tag; 
   if (p->PeekPacketTag (tag) && IsMyOwnAddress (origin))
     {
       ///what does it mean peekpackettag????? and deferredrouto/p?????
       Ptr<Packet> packet = p->Copy ();
       packet->RemovePacketTag(tag);
       DeferredRouteOutput (packet, header, ucb, ecb);
       return true; 
     }
   /*
    *\Determine whether address and interface corresponding to received packet can be accepted for
    * local delivery.param dst IP and interface index iif
    */
   if (m_ipv4->IsDestinationAddress (dst, iif))
     {

       Ptr<Packet> packet = p->Copy ();

       if ((dst != m_ipv4->GetAddress (1, 0).GetBroadcast ())&&(p->GetSize ()==PACKET_DATA_LEN_AFTER_ENCRYPTON) && (m_SecurityState == true))
         {

           unsigned char totalSecurePacket[PACKET_DATA_LEN_FINAL];  
          p->CopyData(totalSecurePacket,PACKET_DATA_LEN_FINAL);
          //cout << " put the packet after copy in array at routenput()...\n";
           //for (int j =0 ; j< PACKET_DATA_LEN_FINAL ; j++)
            //{
              //printf("0x%.2x ", totalSecurePacket[j]);
            //}
            //cout << endl;

          unsigned char totalCipher[PACKET_DATA_LEN_AFTER_ENCRYPTON];
         for (int i = 8; i<PACKET_DATA_LEN_FINAL ; i++)
         {
           totalCipher[(i-8)] = totalSecurePacket[i];
         }
      //cout<<endl;

      //cout<<" extract the cipher and tag ....\n";
      unsigned char cipher[PACKET_DATA_LEN_BEFORE_ENCRYPTON];  
      unsigned char tag[TAG_SIZE];
         for (int i = 0; i<PACKET_DATA_LEN_BEFORE_ENCRYPTON ; i++)
         {
           cipher[i] = totalCipher[i];
         }
      //cout<<" cipher for data packet n i/p ....\n";
        // for (int i = 0; i<PACKET_DATA_LEN_BEFORE_ENCRYPTON ; i++)
         //{
           //printf("0x%.2x ", cipher[i]);
         //}
       //cout<<endl;
         for (int i = PACKET_DATA_LEN_BEFORE_ENCRYPTON; i<PACKET_DATA_LEN_AFTER_ENCRYPTON ; i++)
         {
           tag[(i-PACKET_DATA_LEN_BEFORE_ENCRYPTON)] = totalCipher[i];
         }
      //cout<<" tag for data packet n i/p ....\n";
        // for (int i = 0; i<TAG_SIZE ; i++)
        // {
          // printf("0x%.2x ", tag[i]);
         //}
       //cout<<endl;
  

      unsigned char pText[PACKET_DATA_LEN_BEFORE_ENCRYPTON];

    int check = 5;
    check = m_gcm.Dencryption(cipher,PACKET_DATA_LEN_BEFORE_ENCRYPTON,GCMAADTOBECHECKED,AAD_LENGTH,tag ,m_sharedKey,SHARED_KEY_SIZE,iv,IV_LENGTH,pText);

   //cout<<"print data text after decryption ...\n";

     //    for (int i = 0; i<PACKET_DATA_LEN_BEFORE_ENCRYPTON ; i++)
      //   {
        //   printf("0x%.2x ", pText[i]);
         //}
       //cout<<endl;    
   //cout<<" authentication status is "<<m_authenticationStatus<<endl;
   //for(int k = 0; k <SHARED_KEY_SIZE ; k++)
   // {
     //  printf("0x%.2x ", m_sharedKey[k]);
   // }   
   // cout<<endl; 
   //cout<<" for packet check value of gcm aad is "<<check<<endl;

   if (check == 1)
    {
      NS_LOG_LOGIC (this <<" auhtentication success for data packet ");
      //return;
    }
    else
    {
      NS_LOG_LOGIC (this <<" auhtentication fail for data packet ");
    }

           NS_LOG_LOGIC ("Unicast local delivery to " << dst<<"packet ID is "<<p->GetUid ());
         }
    if ((dst != m_ipv4->GetAddress (1, 0).GetBroadcast ()))
       {
           NS_LOG_LOGIC ("Unicast local delivery to " << dst<<"packet ID is "<<p->GetUid ());
       }
       else
         {
           //NS_LOG_LOGIC ("Broadcast local delivery to " << dst);
         }
 
       lcb (packet, header, iif);
       return true;
     }
 
    return Forwarding (p, header, ucb, ecb);
 }

void
 AeroRoutingProtocol::NotifyInterfaceUp (uint32_t interface)
 {
   NS_LOG_FUNCTION (this << m_ipv4->GetAddress (interface, 0).GetLocal ()<< " interfaces "<<interface);
   Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
   if (l3->GetNAddresses (interface) > 1)
     {
       NS_LOG_WARN ("aerorp does not work with more then one address per each interface.");
     }
   Ipv4InterfaceAddress iface = l3->GetAddress (interface, 0);
   if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
     {
       return;
     }
   if (interface == 1)
{
   // Create a socket to listen only on this interface
   Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                              UdpSocketFactory::GetTypeId ());
   NS_ASSERT (socket != 0);
   socket->SetRecvCallback (MakeCallback (&AeroRoutingProtocol::RecvAERORP, this));
   socket->BindToNetDevice (l3->GetNetDevice (interface));
   socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), AERORP_PORT));
   socket->SetAllowBroadcast (true);
   socket->SetAttribute ("IpTtl", UintegerValue (1));
   m_socketAddresses.insert (std::make_pair (socket, iface));
 
    //should we change wifi to TDMA??????
   // Allow neighbor manager use this interface for layer 2 feedback if possible
   Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
  Ptr<WifiNetDevice> wifi = dev->GetObject<WifiNetDevice> ();
   if (wifi == 0)
     {
       return;
     }
   Ptr<WifiMac> mac = wifi->GetMac ();
   if (mac == 0)
     {
       return;
     }
 
   mac->TraceConnectWithoutContext ("TxErrHeader", m_neighbors->GetTxErrorCallback ());
}
   if (interface == 2)
{
   // Create a socket to listen only on this interface
   Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                              UdpSocketFactory::GetTypeId ());
   NS_ASSERT (socket != 0);
   socket->SetRecvCallback (MakeCallback (&AeroRoutingProtocol::RecvAERORP, this));
   socket->BindToNetDevice (l3->GetNetDevice (interface));
   socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), AERORP_PORT));
   socket->SetAllowBroadcast (false);
   socket->SetAttribute ("IpTtl", UintegerValue (1));
   m_authSocketAddresses.insert (std::make_pair (socket, iface));
 
    //should we change wifi to TDMA??????
   // Allow neighbor manager use this interface for layer 2 feedback if possible
   Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
  Ptr<WifiNetDevice> wifi = dev->GetObject<WifiNetDevice> ();
   if (wifi == 0)
     {
       return;
     }
   Ptr<WifiMac> mac = wifi->GetMac ();
   if (mac == 0)
     {
       return;
     }
 
   mac->TraceConnectWithoutContext ("TxErrHeader", m_neighbors->GetTxErrorCallback ());
}  

 
 }

void
 AeroRoutingProtocol::NotifyInterfaceDown (uint32_t interface)
 {
   NS_LOG_FUNCTION (this << m_ipv4->GetAddress (interface, 0).GetLocal ());
 
   // Disable layer 2 link state monitoring (if possible)
   Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
   Ptr<NetDevice> dev = l3->GetNetDevice (interface);
   Ptr<WifiNetDevice> wifi = dev->GetObject<WifiNetDevice> ();
   if (wifi != 0)
     {
       Ptr<WifiMac> mac = wifi->GetMac ()->GetObject<AdhocWifiMac> ();
       if (mac != 0)
         {
           mac->TraceDisconnectWithoutContext ("TxErrHeader",
                                               m_neighbors->GetTxErrorCallback ());
         }
     }
 
   // Close socket
   Ptr<Socket> socket = FindSocketWithInterfaceAddress (m_ipv4->GetAddress (interface, 0));
   NS_ASSERT (socket);
   socket->Close ();
   m_socketAddresses.erase (socket);
   if (m_socketAddresses.empty ())
     {
       NS_LOG_LOGIC ("No aerorp interfaces");
       m_neighbors->Clear ();
       m_godGroundStation->Clear ();
       m_nodesPosition.Clear ();
       return;
     }
 }
 
 void AeroRoutingProtocol::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address)
 {
   NS_LOG_FUNCTION (this << " interface " << interface << " address " << address);
   Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
   if (!l3->IsUp (interface))
     {
       return;
     }
   if (l3->GetNAddresses ((interface) == 1))
     {
       Ipv4InterfaceAddress iface = l3->GetAddress (interface, 0);
       Ptr<Socket> socket = FindSocketWithInterfaceAddress (iface);
       if (!socket)
         {
           if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
             {
               return;
             }
           // Create a socket to listen only on this interface
           Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                                      UdpSocketFactory::GetTypeId ());
           NS_ASSERT (socket != 0);
           socket->SetRecvCallback (MakeCallback (&AeroRoutingProtocol::RecvAERORP,this));
           socket->BindToNetDevice (l3->GetNetDevice (interface));
           // Bind to any IP address so that broadcasts can be received
           socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), AERORP_PORT));
           socket->SetAllowBroadcast (true);
           m_socketAddresses.insert (std::make_pair (socket, iface));
 
           Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
         }
     }

   if (l3->GetNAddresses ((interface) == 2))
     {
   //cout<<" interface is "<<interface<<endl;
   NS_LOG_FUNCTION (this << " interface 2");
       Ipv4InterfaceAddress iface = l3->GetAddress (interface, 0);
       Ptr<Socket> authSocket = FindSocketWithInterfaceAddress (iface);
       if (!authSocket)
         {
           if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
             {
               return;
             }
           // Create a socket to listen only on this interface
           Ptr<Socket> authSocket = Socket::CreateSocket (GetObject<Node> (),
                                                      UdpSocketFactory::GetTypeId ());
           NS_ASSERT (authSocket != 0);
           authSocket->SetRecvCallback (MakeCallback (&AeroRoutingProtocol::RecvAERORP,this));
           authSocket->BindToNetDevice (l3->GetNetDevice (interface));
           // Bind to any IP address so that broadcasts can be received
           authSocket->Bind (InetSocketAddress (Ipv4Address::GetAny (), AERORP_PORT));
           authSocket->SetAllowBroadcast (false);
           m_authSocketAddresses.insert (std::make_pair (authSocket, iface));
 
           Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
         }
     }

   else
     {
       NS_LOG_LOGIC ("aerorp does not work with more then one address per each interface. Ignore added address");
     }
 }

void
 AeroRoutingProtocol::NotifyRemoveAddress (uint32_t i, Ipv4InterfaceAddress address)
 {
   NS_LOG_FUNCTION (this);
   Ptr<Socket> socket = FindSocketWithInterfaceAddress (address);
   if (socket)
     {
 
       m_socketAddresses.erase (socket);
       Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
       if (l3->GetNAddresses (i))
         {
           Ipv4InterfaceAddress iface = l3->GetAddress (i, 0);
           // Create a socket to listen only on this interface
           Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                                      UdpSocketFactory::GetTypeId ());
           NS_ASSERT (socket != 0);
           socket->SetRecvCallback (MakeCallback (&AeroRoutingProtocol::RecvAERORP, this));
           // Bind to any IP address so that broadcasts can be received
           socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), AERORP_PORT));
           socket->SetAllowBroadcast (true);
           m_socketAddresses.insert (std::make_pair (socket, iface));
 
           // Add local broadcast record to the routing table
           Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
 
         }
       if (m_socketAddresses.empty ())
         {
           NS_LOG_LOGIC ("No aerorp interfaces");
           m_neighbors->Clear ();
           m_godGroundStation->Clear ();
           return;
         }
     }
   else
     {
       NS_LOG_LOGIC ("Remove address not participating in aerorp operation");
     }
 }
 
 void
 AeroRoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
 {
   NS_ASSERT (ipv4 != 0);
   NS_ASSERT (m_ipv4 == 0);
 
   m_ipv4 = ipv4;
 

   //Schedule only when it has packets on queue
   CheckQueueTimer.SetFunction (&AeroRoutingProtocol::CheckQueue, this);
 
   //Simulator::ScheduleNow (&AeroRoutingProtocol::Start, this);
 }

 uint32_t
 AeroRoutingProtocol::GetProtocolNumber (void) const
 {
   return AERORP_PORT;
 }

 void 
 AeroRoutingProtocol::PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const
 {
   uint32_t n = NodeList().GetNNodes ();
   uint32_t i;
   Ptr<Node> node;
   for(i = 0; i < n; i++)
   {

    node = NodeList().GetNode (i);
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
    Ipv4Address nodeadd = ipv4->GetAddress (1, 0).GetLocal ();
   *stream->GetStream () << "Node: " << nodeadd;
      m_queue.PrintQueue (stream);
   }
   *stream->GetStream () << "Node: " << m_ipv4->GetObject<Node> ()->GetId () << " Time: " <<      Simulator::Now ().GetSeconds () << "s ";
  m_neighbors->Print (stream);
  //m_nodesPosition.Print (stream);
 }

 void
 AeroRoutingProtocol::RecvAERORP (Ptr<Socket> socket)
 {
   NS_LOG_FUNCTION (this << socket);

   X509 *sendercert = NULL;
   Time sendertimestamp;
   uint8_t myIdentification;
   uint32_t senderrandno;
   unsigned char recsigneddata[RSA_KEY_SIZE];
   uint8_t senderidentification = 0;
   uint32_t anrandomno = 0;
   uint32_t gsrandno;
   unsigned char recencryptedkey[RSA_KEY_SIZE];
   Address sourceAddress;
  ///Read a single packet from the socket and retrieve the sender address.return pointer to packet
  ///output parameter that will return the address of the sender of the received packet, if any.
 ///Remains untouched if no packet is received
   Ptr<Packet> packet = socket->RecvFrom (sourceAddress);

      NS_LOG_DEBUG (" source address is  " << sourceAddress );

   TypeHeader tHeader (AERORPTYPE_HELLO);
   packet->RemoveHeader (tHeader);
   
   if (!tHeader.IsValid ())
     {
      NS_LOG_DEBUG ("aerorp message " << packet->GetUid () << " with unknown type received: " << tHeader.Get () << ". Ignored");
       return;
     }
    if((m_authenticationStatus == UNAUTH)&& (m_SecurityState == true) &&((tHeader.Get() == AERORPTYPE_HELLO)||(tHeader.Get() == AERORPTYPE_GSGEOLOCATION)||(tHeader.Get() == AERORPTYPE_SECURE_HELLO)||(tHeader.Get() == AERORPTYPE_SECURE_GSGEOLOCATION))) 
     {
       NS_LOG_DEBUG ("aerorp message " << packet->GetUid () << " node is unauth: " << tHeader.Get () << ". Ignored");
       return;
     }

if (tHeader.Get() == AERORPTYPE_AUTHENTICATIONREQUEST)
    {
          //the sender is the AN and received data from the AN and the receiver is GS
   //extract data from packet then validate it
   AuthenticationRequestHeader hdr;
   packet->RemoveHeader (hdr);

   //extract cert from the packet and save it to array and change it to X509 and verify it
   //pass it to  CheckPacketValidity()
    NS_LOG_LOGIC (this <<"authentication request packet");
   uint8_t extractedcertarray[CERT_SIZE];
   unsigned char certarray[CERT_SIZE];
   hdr.GetCertificate(extractedcertarray);
   for (int i =0 ; i< CERT_SIZE ; i++)
    {
      certarray[i] = ((byte *)(&extractedcertarray))[i]; 
    }
    unsigned char *buf1;
    buf1 = certarray;
    const unsigned char *p1 = buf1;
    p1 = buf1;
    sendercert = d2i_X509(NULL, &p1, CERT_SIZE);

   EVP_PKEY *senderpubkey = NULL;
   senderpubkey = X509_get_pubkey(sendercert);
   RSA *senderPairs = NULL;
   //the pairs just have the pub key of the sender
   senderPairs = EVP_PKEY_get1_RSA(senderpubkey);

    uint64_t intTimeStamp = hdr.GetTimeStamp();
    sendertimestamp.FromInteger(intTimeStamp,Time::S);
  NS_LOG_FUNCTION (this << "int recv time stamp "<<intTimeStamp<<" recv time stamp "<<sendertimestamp);
    //the identification of the GS received from the AN packet
    myIdentification = hdr.GetIdentification();

    senderrandno = hdr.GetRandomNo();

    hdr.GetSign(recsigneddata);

   unsigned char recANdata[AN_DATA_SIZE];

   //how can i use it to return signed data we can pass the array and its size
    SetANData(intTimeStamp , senderrandno , myIdentification , recANdata , AN_DATA_SIZE);


   if (CheckPacketValidity(sendercert,sendertimestamp,myIdentification,recANdata,AN_DATA_SIZE,recsigneddata,RSA_KEY_SIZE,senderpubkey) == true)
     {
   InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
   Ipv4Address sender = inetSourceAddr.GetIpv4 ();
   uint32_t senderip = sender.Get();
   //senderidentification is the identification of the AN 
   //senderrandno is the rand no that we recive from the AN
   //senderidentification = static_cast<uint8_t>((senderip % 167837954)-3);
   //error: this decimal constant is unsigned only in ISO C90
   senderidentification = static_cast<uint8_t>(senderip % 3232235777U);
    NS_LOG_LOGIC (this <<"sender IP int is "<<senderip<<" IP address is "<<sender<<" ID is  "<<senderidentification<<" sender rand no "<<senderrandno);
   SendAuthenticationReplyPacket(sender,senderidentification,senderrandno,senderPairs);
     }
    }

   if (tHeader.Get() == AERORPTYPE_AUTHENTICATIONREPLY)
   {
       // the sender is the GS and reciver is AN and received data is GS data
   AuthenticationReplyHeader replyhdr;
   packet->RemoveHeader (replyhdr);
    NS_LOG_LOGIC (this <<"authentication reply packet received:");
   //cout << replyhdr;
   uint8_t extractedcertarray[CERT_SIZE];
   unsigned char certarray[CERT_SIZE];
   replyhdr.GetCertificate(extractedcertarray);
   for (int i =0 ; i< CERT_SIZE ; i++)
    {
      certarray[i] = ((byte *)(&extractedcertarray))[i]; 
    }
    unsigned char *buf1;
    buf1 = certarray;
    const unsigned char *p1 = buf1;
    p1 = buf1;
    sendercert = d2i_X509(NULL, &p1, CERT_SIZE);

   EVP_PKEY *senderpubkey = NULL;
   senderpubkey = X509_get_pubkey(sendercert);

   //extract data from packet then validate it

    uint64_t intTimeStamp = replyhdr.GetTimeStamp();
    sendertimestamp.FromInteger(intTimeStamp,Time::S);
    // identification of the AN
    myIdentification = replyhdr.GetIdentification();

   gsrandno = replyhdr.GetRandomNo();
   anrandomno = replyhdr.GetOldRandomNo();
   
   //set the shared key
   replyhdr.GetSharedKey(recencryptedkey);
   //NS_LOG_FUNCTION (this << "the encrypted shared key from packet is ");
   //for (int i = 0 ; i<RSA_KEY_SIZE ; i++)
   //{
   //   printf("0x%.2x ", recencryptedkey[i]);
   //}
   //cout<<endl;

   replyhdr.GetSign(recsigneddata);
  // NS_LOG_FUNCTION (this << "the signed data from packet is ");
  // for (int i = 0 ; i<RSA_KEY_SIZE ; i++)
  // {
  //    printf("0x%.2x ", recsigneddata[i]);
  // }
  // cout<<endl;
   unsigned char recGSdata[GS_DATA_SIZE];
   //how can i use it to return signed data we can pass the array and its size
   SetGSData(intTimeStamp , gsrandno , anrandomno , myIdentification ,recencryptedkey, recGSdata , GS_DATA_SIZE);
NS_LOG_FUNCTION (this << " int time is "<<intTimeStamp<<"GSRandno "<<gsrandno<<" ANRandono "<<anrandomno<<" ANID "<<myIdentification);
   
   if ((CheckPacketValidity
(sendercert,sendertimestamp,myIdentification,recGSdata,GS_DATA_SIZE,recsigneddata,RSA_KEY_SIZE,senderpubkey)) && (VerifyRandomNo(anrandomno))== true )
     {
       DecryptSharedKey (recencryptedkey , RSA_KEY_SIZE);
       m_authenticationStatus = AUTH;
 NS_LOG_FUNCTION (this << " Successful end of authentication for node "<<m_ipv4->GetObject<Node> ()->GetId ()<<m_authenticationStatus);
     //cout<<"I am authenticated with key \n";
     //for (int i = 0 ; i<SHARED_KEY_SIZE ; i++)
     // {
       //printf("0x%.2x ", m_sharedKey[i]);
    //  }
      //cout<<endl;
      
     }
    else 
     {
     m_authenticationStatus = UNAUTH;
     }

  }

   ///control packet hello
   if (tHeader.Get() == AERORPTYPE_HELLO)
    {

   m_rxHelloTrace(packet);

   HelloHeader hdr;
   packet->RemoveHeader (hdr);
   Vector Position;
   Vector Velocity;
   uint8_t velocitySign;
   double nodeVelocityX;
   double nodeVelocityY;
   double nodeVelocityZ;

   Position.x = hdr.GetOriginPosx ();
   Position.y = hdr.GetOriginPosy ();
   Position.z = hdr.GetOriginPosz ();
   velocitySign = hdr.GetVelSign ();
   if (velocitySign == 0)
     {
      nodeVelocityX = hdr.GetOriginVelx ();
      nodeVelocityY = hdr.GetOriginVely ();
      nodeVelocityZ = hdr.GetOriginVelz ();
     }
   if (velocitySign == 1)
     {
      nodeVelocityX = (-1) * (static_cast<double>(hdr.GetOriginVelx ()));
      nodeVelocityY = (-1) * (static_cast<double>(hdr.GetOriginVely ()));
      nodeVelocityZ = (-1) * (static_cast<double>(hdr.GetOriginVelz ()));
     }
   if (velocitySign == 2)
     {
      nodeVelocityX = (-1) * (static_cast<double>(hdr.GetOriginVelx ()));
      nodeVelocityY = (-1) * (static_cast<double>(hdr.GetOriginVely ()));
      nodeVelocityZ = hdr.GetOriginVelz ();
     }
   if (velocitySign == 3)
     {
      nodeVelocityX = (-1) * (static_cast<double>(hdr.GetOriginVelx ()));
      nodeVelocityY = hdr.GetOriginVely ();
      nodeVelocityZ = (-1) * (static_cast<double>(hdr.GetOriginVelz ()));
     }
   if (velocitySign == 4)
     {
      nodeVelocityX = hdr.GetOriginVelx ();
      nodeVelocityY = (-1) * (static_cast<double>(hdr.GetOriginVely ()));
      nodeVelocityZ = (-1) * (static_cast<double>(hdr.GetOriginVelz ()));
     }
   if (velocitySign == 5)
     {
      nodeVelocityX = (-1) * (static_cast<double>(hdr.GetOriginVelx ()));
      nodeVelocityY = hdr.GetOriginVely ();
      nodeVelocityZ = hdr.GetOriginVelz ();
     }
   if (velocitySign == 6)
     {
      nodeVelocityX = hdr.GetOriginVelx ();
      nodeVelocityY = (-1) * (static_cast<double>(hdr.GetOriginVely ()));
      nodeVelocityZ = hdr.GetOriginVelz ();
     }
   if (velocitySign == 7)
     {
      nodeVelocityX = hdr.GetOriginVelx ();
      nodeVelocityY = hdr.GetOriginVely ();
      nodeVelocityZ = (-1) * (static_cast<double>(hdr.GetOriginVelz ()));
     }

     //NS_LOG_LOGIC ("vel x "<<nodeVelocityX <<"vel y "<<nodeVelocityY<<"vel z "<<nodeVelocityZ);

      Velocity.x = nodeVelocityX;
      Velocity.y = nodeVelocityY;
      Velocity.z = nodeVelocityZ;

    //NS_LOG_LOGIC ("vel x "<<Velocity.x <<"vel y "<<Velocity.y<<"vel z "<<Velocity.z<<"");

   InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
   Ipv4Address sender = inetSourceAddr.GetIpv4 ();
   //Ipv4Address receiver = m_socketAddresses[socket].GetLocal ();
    NS_LOG_LOGIC (this <<" update neighbor table "<<" sender is "<<sender);
   UpdateRouteToNeighbor (sender, Position, Velocity);
   }

    if ((tHeader.Get() == AERORPTYPE_SECURE_HELLO))
   {
    unsigned char cipher[HELLO_PACKET_SIZE];    
    unsigned char ptext[HELLO_PACKET_SIZE];    
    unsigned char tag[TAG_SIZE];
   //cout<<"set aad...\n";
    //for (int i = 0 ; i < AAD_LENGTH ; i++)
    //{
     //GCMAADTOBECHECKED[i] = i;
    // printf("0x%.2x ", GCMAADTOBECHECKED[i]);
    //}
    //cout<<endl;
   //cout<<"set iv...\n";
   //cout<<"set iv...\n";
   uint64_t packetid = packet->GetUid();
    unsigned char iv [IV_LENGTH];
    for (int i = 0 ; i < 8 ; i++)
    {
      iv[i] = ((uint8_t *)&packetid)[i];

    }
    for (int i = 8 ; i < IV_LENGTH ; i++)
    {
      iv[i] = i;
    } 
    //for (int i = 0 ; i < IV_LENGTH ; i++)
   // {
   //  printf("0x%.2x ", iv[i]);
   // }
   // cout<<endl;

    uint8_t tmessage[(HELLO_PACKET_SIZE + TAG_SIZE) ];
    //Buffer buf;
    packet->CopyData(tmessage,(HELLO_PACKET_SIZE + TAG_SIZE));
         
   //cout << "After copying cipher is...\n";
   for (int j =0 ; j< HELLO_PACKET_SIZE ; j++)
    {
     cipher[j] = tmessage[j];
     //printf("0x%.2x ", cipher[j]);
    }
    //cout << endl;
   //cout << "After copying tag is...\n";
   for (int j =0 ; j< TAG_SIZE ; j++)
    {
     tag[j] = tmessage[HELLO_PACKET_SIZE + j];
     //printf("0x%.2x ", tag[j]);
    }
    //cout << endl;
   SecureHelloHeader sHellohdr;
   packet->RemoveHeader (sHellohdr);
   
    int check = 5;
    check = m_gcm.Dencryption(cipher,HELLO_PACKET_SIZE,GCMAADTOBECHECKED,AAD_LENGTH,tag ,m_sharedKey,SHARED_KEY_SIZE,iv,IV_LENGTH,ptext);

   //cout<<" for hello check value of gcm aad is "<<check<<endl;

   if (check != 1)
    {
      NS_LOG_LOGIC (this <<" auhtentication fail for hello ");
      return;
    }
    else
    {
   //cout<<" authentication status is "<<m_authenticationStatus<<endl;
   //for(int k = 0; k <SHARED_KEY_SIZE ; k++)
   // {
     //  printf("0x%.2x ", m_sharedKey[k]);
    //}   
    //cout<<endl;  

   //cout << "After decryption the data is ...\n";
   //for (int j =0 ; j< HELLO_PACKET_SIZE ; j++)
   // {
    // printf("0x%.2x ", ptext[j]);
    //}
    //cout << endl;

//   cout << "parsing data...\n";

   HelloHeader hdr;
   Buffer helloBuf;
   helloBuf.AddAtEnd(HELLO_PACKET_SIZE);
   //cout << "Serializing header...";  	 
   helloBuf.Begin().Write (ptext,HELLO_PACKET_SIZE);
   //helloBuf.Deserialize (ptext,HELLO_PACKET_SIZE);

   uint8_t helloSerBuf[HELLO_PACKET_SIZE];        
   helloBuf.CopyData(helloSerBuf, HELLO_PACKET_SIZE);
   //cout << "chech the buffer...\n";
   //for (int j =0 ; j< HELLO_PACKET_SIZE ; j++)
   // {
   //  printf("0x%.2x ", helloSerBuf[j]);
   // }
   // cout << endl;

   hdr.Deserialize(helloBuf.Begin());
   //hdr.Print(cout);
   //cout<<endl;

   Vector Position;
   Vector Velocity;
   uint8_t velocitySign;
   double nodeVelocityX;
   double nodeVelocityY;
   double nodeVelocityZ;

   Position.x = hdr.GetOriginPosx ();
   Position.y = hdr.GetOriginPosy ();
   Position.z = hdr.GetOriginPosz ();
   velocitySign = hdr.GetVelSign ();
   if (velocitySign == 0)
     {
      nodeVelocityX = hdr.GetOriginVelx ();
      nodeVelocityY = hdr.GetOriginVely ();
      nodeVelocityZ = hdr.GetOriginVelz ();
     }
   if (velocitySign == 1)
     {
      nodeVelocityX = (-1) * (static_cast<double>(hdr.GetOriginVelx ()));
      nodeVelocityY = (-1) * (static_cast<double>(hdr.GetOriginVely ()));
      nodeVelocityZ = (-1) * (static_cast<double>(hdr.GetOriginVelz ()));
     }
   if (velocitySign == 2)
     {
      nodeVelocityX = (-1) * (static_cast<double>(hdr.GetOriginVelx ()));
      nodeVelocityY = (-1) * (static_cast<double>(hdr.GetOriginVely ()));
      nodeVelocityZ = hdr.GetOriginVelz ();
     }
   if (velocitySign == 3)
     {
      nodeVelocityX = (-1) * (static_cast<double>(hdr.GetOriginVelx ()));
      nodeVelocityY = hdr.GetOriginVely ();
      nodeVelocityZ = (-1) * (static_cast<double>(hdr.GetOriginVelz ()));
     }
   if (velocitySign == 4)
     {
      nodeVelocityX = hdr.GetOriginVelx ();
      nodeVelocityY = (-1) * (static_cast<double>(hdr.GetOriginVely ()));
      nodeVelocityZ = (-1) * (static_cast<double>(hdr.GetOriginVelz ()));
     }
   if (velocitySign == 5)
     {
      nodeVelocityX = (-1) * (static_cast<double>(hdr.GetOriginVelx ()));
      nodeVelocityY = hdr.GetOriginVely ();
      nodeVelocityZ = hdr.GetOriginVelz ();
     }
   if (velocitySign == 6)
     {
      nodeVelocityX = hdr.GetOriginVelx ();
      nodeVelocityY = (-1) * (static_cast<double>(hdr.GetOriginVely ()));
      nodeVelocityZ = hdr.GetOriginVelz ();
     }
   if (velocitySign == 7)
     {
      nodeVelocityX = hdr.GetOriginVelx ();
      nodeVelocityY = hdr.GetOriginVely ();
      nodeVelocityZ = (-1) * (static_cast<double>(hdr.GetOriginVelz ()));
     }

 NS_LOG_LOGIC ("posx "<<Position.x<<" posy "<<Position.y<<" posz "<<Position.z<<" vel sign "<<velocitySign<<"vel x "<<nodeVelocityX <<"vel y "<<nodeVelocityY<<"vel z "<<nodeVelocityZ);

      Velocity.x = nodeVelocityX;
      Velocity.y = nodeVelocityY;
      Velocity.z = nodeVelocityZ;

    //NS_LOG_LOGIC ("vel x "<<Velocity.x <<"vel y "<<Velocity.y<<"vel z "<<Velocity.z<<"");

   InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
   Ipv4Address sender = inetSourceAddr.GetIpv4 ();
   //Ipv4Address receiver = m_socketAddresses[socket].GetLocal ();
    NS_LOG_LOGIC (this <<" update neighbor table "<<" sender is "<<sender);
   UpdateRouteToNeighbor (sender, Position, Velocity);
   }
   }

   ///control packet hello
   if (tHeader.Get() == AERORPTYPE_SECURE_GSGEOLOCATION)
    {

   m_rxGsTrace(packet);   

    unsigned char cipher[GS_PACKET_SIZE];    
    unsigned char ptext[GS_PACKET_SIZE];    
    unsigned char tag[TAG_SIZE];
   //cout<<"set aad...\n";
   // for (int i = 0 ; i < AAD_LENGTH ; i++)
   // {
   // printf("0x%.2x ", GCMAADTOBECHECKED[i]);
   // }
   // cout<<endl;
   //cout<<"set iv...\n";
   uint64_t packetid = packet->GetUid();
    unsigned char iv [IV_LENGTH];
    for (int i = 0 ; i < 8 ; i++)
    {
      iv[i] = ((uint8_t *)&packetid)[i];

    }
    for (int i = 8 ; i < IV_LENGTH ; i++)
    {
      iv[i] = i;
    } 
   // for (int i = 0 ; i < IV_LENGTH ; i++)
   // {
   //  printf("0x%.2x ", iv[i]);
    //}
   // cout<<endl;

    uint8_t tmessage[(GS_PACKET_SIZE + TAG_SIZE) ];
    //Buffer buf;
    packet->CopyData(tmessage,(GS_PACKET_SIZE + TAG_SIZE));
         
   //cout << "After copying cipher is...\n";
   for (int j =0 ; j< GS_PACKET_SIZE ; j++)
    {
     cipher[j] = tmessage[j];
     //printf("0x%.2x ", cipher[j]);
    }
    //cout << endl;
   //cout << "After copying tag is...\n";
   for (int j =0 ; j< TAG_SIZE ; j++)
    {
     tag[j] = tmessage[GS_PACKET_SIZE + j];
     //printf("0x%.2x ", tag[j]);
    }
   //cout << endl;
   SecureGSGeoLocationHeader sGShdr;
   packet->RemoveHeader (sGShdr);

    int check = 5;

    check = m_gcm.Dencryption(cipher,GS_PACKET_SIZE,GCMAADTOBECHECKED,AAD_LENGTH,tag ,m_sharedKey,SHARED_KEY_SIZE,iv,IV_LENGTH,ptext);

  //cout<<" for gs check value of gcm aad is "<<check<<endl;

   if (check != 1)
    {
      NS_LOG_LOGIC (this <<" auhtentication fail for gs ");
      return;
    }
    else
    {
   //cout<<" authentication status is "<<m_authenticationStatus<<endl;
   //for(int k = 0; k <SHARED_KEY_SIZE ; k++)
   // {
   //    printf("0x%.2x ", m_sharedKey[k]);
   // }   
   // cout<<endl;  

  //cout << "After decryption the data is ...\n";
   //for (int j =0 ; j< GS_PACKET_SIZE ; j++)
   // {
   //  printf("0x%.2x ", ptext[j]);
   // }
   // cout << endl;

  // cout << "parsing data...\n";

   GSGeoLocationHeader gsHdr;
   Buffer gsBuf;
   gsBuf.AddAtEnd(GS_PACKET_SIZE);
  // cout << "Serializing header...";  	 
   gsBuf.Begin().Write (ptext,GS_PACKET_SIZE);
   //helloBuf.Deserialize (ptext,HELLO_PACKET_SIZE);

   uint8_t gsSerBuf[GS_PACKET_SIZE];        
   gsBuf.CopyData(gsSerBuf, GS_PACKET_SIZE);
  // cout << "chech the buffer...\n";
  // for (int j =0 ; j< GS_PACKET_SIZE ; j++)
  //  {
  //   printf("0x%.2x ", gsSerBuf[j]);
  //  }
  //  cout << endl;

   gsHdr.Deserialize(gsBuf.Begin());
   //gsHdr.Print(cout);
   //cout<<endl;

   
   Vector Position;
   Vector Velocity;
   Position.x = gsHdr.GetNodePosx ();
   Position.y = gsHdr.GetNodePosy ();
   Position.z = gsHdr.GetNodePosz ();
   Velocity.x = gsHdr.GetNodeVelocityx ();
   Velocity.y = gsHdr.GetNodeVelocityy ();
   Velocity.z = gsHdr.GetNodeVelocityz ();
   Ipv4Address Node = gsHdr.GetNode();
 

    //NS_LOG_LOGIC (" potsition x "<< Position.x <<" potsition y "<< Position.y <<" potsition z "<< Position.z <<" vel x "<< Velocity.x <<" vel y "<< Velocity.y <<" vel z "<< Velocity.z);
 
   UpdateRouteToNodes (Node, Position, Velocity);
   }
   }
   ///control packet GSGeolocation
   if (tHeader.Get() == AERORPTYPE_GSGEOLOCATION)
    {

   m_rxGsTrace(packet);   

   GSGeoLocationHeader hdr;
   packet->RemoveHeader (hdr);
   
   Vector Position;
   Vector Velocity;
   Position.x = hdr.GetNodePosx ();
   Position.y = hdr.GetNodePosy ();
   Position.z = hdr.GetNodePosz ();
   Velocity.x = hdr.GetNodeVelocityx ();
   Velocity.y = hdr.GetNodeVelocityy ();
   Velocity.z = hdr.GetNodeVelocityz ();
   Ipv4Address Node = hdr.GetNode();
 
   UpdateRouteToNodes (Node, Position, Velocity);
    }
 }

 void
 AeroRoutingProtocol::UpdateRouteToNeighbor (Ipv4Address sender,Vector Pos , Vector Vel )
 {
   m_neighbors->AddEntry (sender, Pos, Vel);
 
 }

 void
 AeroRoutingProtocol::UpdateRouteToNodes (Ipv4Address Node,Vector Pos ,Vector Vel )
 {
   m_nodesPosition.AddEntry (Node, Pos);
 
 }

 void
 AeroRoutingProtocol::SendHello ()
 {
  NS_LOG_FUNCTION (this);
   double positionX;
   double positionY;
   double positionZ;

   double velocityX;
   double velocityY;
   double velocityZ;
 
   Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();
 
   positionX = MM->GetPosition ().x;
   positionY = MM->GetPosition ().y;
   positionZ = MM->GetPosition ().z;
   //NS_LOG_LOGIC ("node x  " << positionX <<"node y " <<positionY<<"node z "<<positionZ);
   velocityX = MM->GetVelocity ().x;
   velocityY = MM->GetVelocity ().y;
   velocityZ = MM->GetVelocity ().z;

   //NS_LOG_LOGIC ("vel x  " << velocityX <<"vel y " <<velocityY<<"vel z "<<velocityZ); 
   uint16_t VelocitySign = 0;   
   uint32_t nodeVelocityX = 0;
   uint32_t nodeVelocityY = 0;
   uint32_t nodeVelocityZ = 0;

   if (velocityX < 0 ||velocityY < 0 || velocityZ < 0 )
    {
        if ((velocityX < 0) && (velocityY < 0) && (velocityZ < 0) )
         {
           VelocitySign = 1;
           nodeVelocityX = (-1 * velocityX);
           nodeVelocityY = (-1 * velocityY);
           nodeVelocityZ = (-1 * velocityZ);
         }
      if ((velocityX < 0) && (velocityY < 0) && (velocityZ >= 0))
         {
           VelocitySign = 2;
           nodeVelocityX = (-1 * velocityX);
           nodeVelocityY = (-1 * velocityY);
           nodeVelocityZ = velocityZ;
         }
       if ((velocityX < 0) && (velocityY >=0) && (velocityZ < 0))
         {
           VelocitySign = 3;
           nodeVelocityX = (-1 * velocityX);
           nodeVelocityY = velocityY;
           nodeVelocityZ = (-1 * velocityZ);
         }
       if ((velocityX >= 0) && (velocityY < 0) && (velocityZ < 0))
         {
           VelocitySign = 4;
           nodeVelocityX = velocityX;
           nodeVelocityY = (-1 * velocityY);
           nodeVelocityZ = (-1 * velocityZ);
         }
       if ((velocityX < 0) && (velocityY >= 0) && (velocityZ >= 0))
         {
           VelocitySign = 5;
           nodeVelocityX = (-1 * velocityX);
           nodeVelocityY = velocityY;
           nodeVelocityZ = velocityZ;
         }
       if ((velocityX >= 0) && (velocityY < 0) && (velocityZ >= 0))
         {
           VelocitySign = 6;
           nodeVelocityX = velocityX;
           nodeVelocityY = (-1 * velocityY);
           nodeVelocityZ = velocityZ;
         }
       if ((velocityX >= 0) && (velocityY >= 0) && (velocityZ < 0))
         {
           VelocitySign = 7;
           nodeVelocityX = velocityX;
           nodeVelocityY = velocityY;
           nodeVelocityZ = (-1 * velocityZ);
         }

     }
    else
     {
           VelocitySign = 0;
           nodeVelocityX = velocityX;
           nodeVelocityY = velocityY;
           nodeVelocityZ = velocityZ;
     }
   //NS_LOG_LOGIC ("vel sign "<<static_cast<uint16_t> (VelocitySign)); 
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
     {
       Ptr<Socket> socket = j->first;
       Ipv4InterfaceAddress iface = j->second;

       HelloHeader helloHeader (((uint32_t) positionX),((uint32_t) positionY),((uint32_t) positionZ),((uint8_t) VelocitySign),((uint32_t) nodeVelocityX),((uint32_t) nodeVelocityY),((uint32_t) nodeVelocityZ));
 
       Ptr<Packet> packet = Create<Packet> ();
       packet->AddHeader (helloHeader);
       TypeHeader tHeader (AERORPTYPE_HELLO);
       packet->AddHeader (tHeader);
       // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
       Ipv4Address destination;
       if (iface.GetMask () == Ipv4Mask::GetOnes ())
         {
           destination = Ipv4Address ("255.255.255.255");
         }
       else
         {
           destination = iface.GetBroadcast ();
         }
       /*
        * payload packet
        * uint32_t size = 1024;
        * Ptr<Packet> p = Create<Packet> (size);
        * uint8_t *buffer = ...;
        * uint32_t size = ...;
        * Ptr<Packet> p = Create<Packet> (buffer, size);
        */
       m_txHelloTrace (packet);
       NS_LOG_LOGIC (this <<" print the hello packet "); 
       //packet->Print(cout);
       socket->SendTo (packet, 0, InetSocketAddress (destination, AERORP_PORT));
 
     }
 }

 void
 AeroRoutingProtocol::SendBadHello ()
 {
/*
  NS_LOG_FUNCTION (this);
   double positionX;
   double positionY;
   double positionZ;
  
   positionX = 10000;
   positionY = 10000;
   positionZ = 10000;

   //NS_LOG_LOGIC ("vel x  " << velocityX <<"vel y " <<velocityY<<"vel z "<<velocityZ); 
   uint16_t VelocitySign = 0;   
   uint32_t nodeVelocityX = 900;
   uint32_t nodeVelocityY = 900;
   uint32_t nodeVelocityZ = 900;

   //NS_LOG_LOGIC ("vel sign "<<static_cast<uint16_t> (VelocitySign)); 
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
     {
       Ptr<Socket> socket = j->first;
       Ipv4InterfaceAddress iface = j->second;

       HelloHeader helloHeader (((uint32_t) positionX),((uint32_t) positionY),((uint32_t) positionZ),((uint8_t) VelocitySign),((uint32_t) nodeVelocityX),((uint32_t) nodeVelocityY),((uint32_t) nodeVelocityZ));
 
       Ptr<Packet> packet = Create<Packet> ();
       packet->AddHeader (helloHeader);
       TypeHeader tHeader (AERORPTYPE_HELLO);
       packet->AddHeader (tHeader);
       // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
       Ipv4Address destination;
       if (iface.GetMask () == Ipv4Mask::GetOnes ())
         {
           destination = Ipv4Address ("255.255.255.255");
         }
       else
         {
           destination = iface.GetBroadcast ();
         }
       NS_LOG_LOGIC (this <<" print the hello packet "); 
       //packet->Print(cout);
       socket->SendTo (packet, 0, InetSocketAddress (destination, AERORP_PORT));
 
     }
*/
 }

 void
 AeroRoutingProtocol::SendGroundStationPackets ()
 {
   NS_LOG_FUNCTION (this);
   double positionX;
   double positionY;
   double positionZ;

   double velocityX;
   double velocityY;
   double velocityZ;

   Ipv4Address nodeadd;
   
   uint32_t n = NodeList().GetNNodes ();
   uint32_t i;
   Ptr<Node> node;
   for(i = 0; i < n; i++)
   {

    node = NodeList().GetNode (i);
	  //pointer to the requested node//
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();

    nodeadd = ipv4->GetAddress (1, 0).GetLocal ();
   
    if (m_AttackState == true)
    {
   positionX = m_godGroundStation->GetFaultedPosition (nodeadd).x;

   positionY = m_godGroundStation->GetFaultedPosition (nodeadd).y;
   positionZ = m_godGroundStation->GetFaultedPosition (nodeadd).z;

   velocityX = m_godGroundStation->GetFaultedVelocity (nodeadd).x;
   velocityY = m_godGroundStation->GetFaultedVelocity (nodeadd).y;
   velocityZ = m_godGroundStation->GetFaultedVelocity (nodeadd).z;
    }
   else
   {
   positionX = m_godGroundStation->GetPosition (nodeadd).x;

   positionY = m_godGroundStation->GetPosition (nodeadd).y;
   positionZ = m_godGroundStation->GetPosition (nodeadd).z;

   velocityX = m_godGroundStation->GetVelocity (nodeadd).x;
   velocityY = m_godGroundStation->GetVelocity (nodeadd).y;
   velocityZ = m_godGroundStation->GetVelocity (nodeadd).z;
   }
    for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin(); 
        j != m_socketAddresses.end (); ++j)
     {
       Ptr<Socket> socket = j->first;
       Ipv4InterfaceAddress iface = j->second;
       ///still need to add the rest of feilds
       GSGeoLocationHeader GsGeolocation (((Ipv4Address) nodeadd),((uint32_t) positionX),((uint32_t) positionY),((uint32_t) positionZ),((uint32_t) velocityX),((uint32_t) velocityY),((uint32_t) velocityZ));
 
       Ptr<Packet> packet = Create<Packet> ();

       packet->AddHeader (GsGeolocation);

       TypeHeader tHeader (AERORPTYPE_GSGEOLOCATION);
       packet->AddHeader (tHeader);
       // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
       Ipv4Address destination;
       if (iface.GetMask () == Ipv4Mask::GetOnes ())
         {
           destination = Ipv4Address ("255.255.255.255");
         }
       else
         {
           destination = iface.GetBroadcast ();
         }
       m_txGsTrace (packet);
       NS_LOG_LOGIC (this <<" print the gs packet "); 
      // packet->Print(cout);
       socket->SendTo (packet, 0, InetSocketAddress (destination, AERORP_PORT));
     }
   }


 }

 bool
 AeroRoutingProtocol::IsMyOwnAddress (Ipv4Address src)
 {
   NS_LOG_FUNCTION (this << src);
   for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
     {
       Ipv4InterfaceAddress iface = j->second;
       if (src == iface.GetLocal ())
         {
           return true;
         }
     }
   return false;
 }

 void
 AeroRoutingProtocol::SetDownTarget (IpL4Protocol::DownTargetCallback callback)
 {
   m_downTarget = callback;
 }
  
IpL4Protocol::DownTargetCallback
 AeroRoutingProtocol::GetDownTarget (void) const
 {
   return m_downTarget;
 }

 void
 AeroRoutingProtocol::Start (uint32_t noofnodes)
 {
   NS_LOG_FUNCTION (this);
   if (m_SecurityState == true)
   {
   AuthenticationIntialize();
   NS_LOG_FUNCTION (this<<" gs add is "<<GROUND_STATION_ADDRESS);
   if(m_isGroundStation)
   {
     GSTimer.SetFunction (&AeroRoutingProtocol::GSTimerExpire, this);
     GSTimer.Schedule (FIRST_JITTER);

     m_godGroundStation = Create<GodGroundStation>();
     SetSharedKey();
     m_authenticationStatus = AUTH;
     //m_neighbors->Destroy();
     //m_nodesPosition.Destroy();

   } 
   else 
   {
     AuthenticationTimer.SetFunction (&AeroRoutingProtocol::AuthenticationTimerExpire, this);
   if (m_authenticationStatus == UNAUTH)
    {
      AuthenticationTimer.Schedule (FIRSTAUTH_JITTER);
    }
    else
    {
     AuthenticationTimer.Cancel ();
     HelloIntervalTimer.SetFunction (&AeroRoutingProtocol::HelloTimerExpire, this);
     HelloIntervalTimer.Schedule (FIRST_JITTER);
    }
     HelloIntervalTimer.SetFunction (&AeroRoutingProtocol::HelloTimerExpire, this);
     HelloIntervalTimer.Schedule (FIRST_JITTER);
   }
   }
   if (m_SecurityState == false)
   {
   if(m_isGroundStation)
   {
     GSTimer.SetFunction (&AeroRoutingProtocol::GSTimerExpire, this);
     GSTimer.Schedule (FIRST_JITTER);

     m_godGroundStation = Create<GodGroundStation>();
    }
/*
   if(m_isAttacker)
   {
     HelloIntervalTimer.SetFunction (&AeroRoutingProtocol::HelloTimerExpire, this);
     HelloIntervalTimer.Schedule (FIRST_JITTER);
    }
*/
    else
    {
     HelloIntervalTimer.SetFunction (&AeroRoutingProtocol::HelloTimerExpire, this);
     HelloIntervalTimer.Schedule (FIRST_JITTER);
    }
   }

   m_queuedAddresses.clear ();
 
   //FIXME ajustar timer, meter valor parametrizavel
   Time tableTime ("2s");
 
 }
 
 /// if i can't find a next hop or best neighbor to dst i put it in qeue untill i find one 
 void
 AeroRoutingProtocol::DeferredRouteOutput (Ptr<const Packet> p, const Ipv4Header & header,
                                       UnicastForwardCallback ucb, ErrorCallback ecb)
 {
   NS_LOG_FUNCTION (this << p << header);
   NS_ASSERT (p != 0 && p != Ptr<Packet> ());
 
   if (m_queue.GetSize () == 0)
     {
       CheckQueueTimer.Cancel ();
       CheckQueueTimer.Schedule (Time ("500ms"));
     }
 
   QueueEntry newEntry (p, header, ucb, ecb);
   bool result = m_queue.Enqueue (newEntry);
 
 
   m_queuedAddresses.insert (m_queuedAddresses.begin (), header.GetDestination ());
   m_queuedAddresses.unique ();
 
   if (result)
     {
       NS_LOG_LOGIC ("Add packet " << p->GetUid () << " to queue. Protocol " << (uint16_t) header.GetProtocol ());
 
     } 
 }

 void
 AeroRoutingProtocol::HelloTimerExpire ()
 {
   NS_LOG_FUNCTION (this << "authenticated?" << (AUTH == m_authenticationStatus));
   if((AUTH == m_authenticationStatus) && (m_SecurityState == true))
    {
    SendHello ();
    HelloIntervalTimer.Cancel ();
    HelloIntervalTimer.Schedule (HelloInterval + JITTER);
    }
 if((UNAUTH == m_authenticationStatus) && (m_SecurityState == true))
    {
    HelloIntervalTimer.Schedule (HelloInterval + JITTER);
    }
/*
   if((m_SecurityState == false)&& (m_AttackState == true)&& (m_isAttacker == true))
    {
    SendBadHello ();
    HelloIntervalTimer.Cancel ();
    HelloIntervalTimer.Schedule (HelloInterval + JITTER);
    }
*/
   if(m_SecurityState == false)
    {
    SendHello ();
    HelloIntervalTimer.Cancel ();
    HelloIntervalTimer.Schedule (HelloInterval + JITTER);
    }
 }

 void
 AeroRoutingProtocol::GSTimerExpire ()
 {
   SendGroundStationPackets ();
   GSTimer.Cancel ();
   ///iwat to simulate the rotation of a radar with jitter1
   GSTimer.Schedule (GSInterval + JITTER);
 }

 void
 AeroRoutingProtocol::SetGroundStation ()
 {
     m_isGroundStation = true;
 }

 void
 AeroRoutingProtocol::SetAttacker ()
 {
     m_isAttacker = true;
 }
Ptr<Ipv4Route>
 AeroRoutingProtocol::LoopbackRoute (const Ipv4Header & hdr, Ptr<NetDevice> oif)
 {
   NS_LOG_FUNCTION (this << hdr);
  /**
   * \param interface The interface number(?????) of an Ipv4 interface.
   * \returns The NetDevice associated with the Ipv4 interface number.
   */
   m_lo = m_ipv4->GetNetDevice (0);
   NS_ASSERT (m_lo != 0);
   /*
    * \breif ipv4route used to define some param such as dest add,source add,gateway,o/p devices
    * and for multicast parameters 
    */
   Ptr<Ipv4Route> rt = Create<Ipv4Route> ();
   rt->SetDestination (hdr.GetDestination ());
    //* \brief a class to store IPv4 address information on an interface
   std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin ();
   if (oif)
     {
       // Iterate to find an address on the oif device
      for (j = m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
         {
           Ipv4Address addr = j->second.GetLocal ();
   /*
    * \brief Return the interface number of the interface that has been
    * \assigned the specified IP address.
    */
           int32_t interface = m_ipv4->GetInterfaceForAddress (addr);
           if (oif == m_ipv4->GetNetDevice (static_cast<uint32_t> (interface)))
             {
               rt->SetSource (addr);
               break;
             }
         }
     }
   else
     {
       rt->SetSource (j->second.GetLocal ());
     }
   NS_ASSERT_MSG (rt->GetSource () != Ipv4Address (), "Valid aerorp source address not found");
   rt->SetGateway (Ipv4Address ("127.0.0.1"));
   rt->SetOutputDevice (m_lo);
   return rt;
 }

/* 
void
 RoutingProtocol::AddHeaders (Ptr<Packet> p, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route)
 {
 
   NS_LOG_FUNCTION (this << " source " << source << " destination " << destination);
  
   Vector myPos;
   Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();
   myPosition = MM->GetPosition ();
   myVelocity = MM->GetVelocity ();

 
   uint16_t positionX = 0;
   uint16_t positionY = 0;
   uint32_t hdrTime = 0;
 
   if(destination != m_ipv4->GetAddress (1, 0).GetBroadcast ())
     {
       positionX = m_locationService->GetPosition (destination).x;
       positionY = m_locationService->GetPosition (destination).y;
       hdrTime = (uint32_t) m_locationService->GetEntryUpdateTime (destination).GetSeconds ();
     }
 
   PositionHeader posHeader (positionX, positionY,  hdrTime, (uint64_t) 0,(uint64_t) 0, (uint8_t) 0, myPos.x, myPos.y); 
   p->AddHeader (posHeader);
   TypeHeader tHeader (GPSRTYPE_POS);
   p->AddHeader (tHeader);
 
   m_downTarget (p, source, destination, protocol, route);
 
 }
 
*/
 bool
 AeroRoutingProtocol::Forwarding (Ptr<const Packet> packet, const Ipv4Header & header,
                              UnicastForwardCallback ucb, ErrorCallback ecb)
 {
   Ptr<Packet> p = packet->Copy ();
   NS_LOG_FUNCTION (this);
   Ipv4Address dst = header.GetDestination ();
   Ipv4Address source = header.GetSource ();

   Vector Position;
   Vector myPosition;
   Vector myVelocity;
   m_neighbors->Purge ();
   m_nodesPosition.GetPosition(dst);

   Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();
   myPosition = MM->GetPosition ();
   myVelocity = MM->GetVelocity ();
   Ipv4Address nextHop;
   //this i added
   Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
   //the Ipv4InterfaceAddress associated to the interface and addressIndex
   Ipv4InterfaceAddress iface = l3->GetAddress (1, 0);
   Ptr<NetDevice> oif;
   //GetLocal() return local address.
   /*GetInterfaceForAddress (address)Return the interface number of the interface that has been
    *assigned the specified IP address.
    *Each IP interface has one or more IP addresses associated with it. This method searches the
    *list of interfaces for one that holds a particular address. This call takes an IP address as
    *a parameter and returns the interface number of the first interface that has been assigned
    *that address, or -1 if not found. There must be an exact match; this method will not match
    *broadcast or multicast addresses
    */
    //GetNetDevice() return pointer to NetDevice for the particualr interface
   oif =m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
   Ptr<Ipv4Route> route = Create<Ipv4Route> ();

   if(m_neighbors->isNeighbour (dst))
     {
       nextHop = dst;

       //this i added
       route->SetDestination (dst);
       route->SetSource (source);
       route->SetGateway (nextHop);
       route->SetOutputDevice (m_ipv4->GetNetDevice (1));
   NS_LOG_LOGIC ("modified route destination is " << route->GetDestination() <<"source is " <<route->GetSource()<<"gateway is "<<route->GetGateway()<<"output device is "<<route->GetOutputDevice ());
      } 
    else 
      {
      
        nextHop = m_neighbors->BestNeighbor (Position, myPosition, myVelocity);
        
        if (nextHop != Ipv4Address::GetZero ())
         {
 
              //Ptr<NetDevice> oif = m_ipv4->GetObject<NetDevice> ();
              //Ptr<Ipv4Route> route = Create<Ipv4Route> ();
              route->SetDestination (dst);
              route->SetSource (source);
              route->SetGateway (nextHop);
           
              // FIXME: Does not work for multiple interfaces
              route->SetOutputDevice (m_ipv4->GetNetDevice (1));
              //route->SetDestination (header.GetDestination ());
   NS_LOG_LOGIC ("modified route destination is " << route->GetDestination() <<"source is " <<route->GetSource()<<"gateway is "<<route->GetGateway()<<"output device is "<<route->GetOutputDevice ());
          } 
         else 
            {

             DeferredRouteOutput (packet, header, ucb, ecb);
          
             //CheckQueue ();
	     return true;
          }
   }
   //this i added
   ucb (route, p, header);
   NS_LOG_LOGIC ("modified route destination is " << route->GetDestination() <<"source is " <<route->GetSource()<<"gateway is "<<route->GetGateway()<<"output device is "<<route->GetOutputDevice ());

   return true;
 }

 Ptr<Socket>
 AeroRoutingProtocol::FindSocketWithInterfaceAddress (Ipv4InterfaceAddress addr ) const
 {
   NS_LOG_FUNCTION (this << addr);
   for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
          m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
     {
       Ptr<Socket> socket = j->first;
       Ipv4InterfaceAddress iface = j->second;
       if (iface == addr)
         {
           return socket;
         }
     }
   Ptr<Socket> socket;
   return socket;
 }
 
 bool
 AeroRoutingProtocol::SendPacketFromQueue (Ipv4Address dst)
 {
   NS_LOG_FUNCTION (this);

   QueueEntry queueEntry;

   Vector myPos;   
   Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();
   myPos = MM->GetPosition ();

   Vector myVel;
   myVel = MM->GetVelocity (); 
   Ipv4Address nextHop;
   //this i added
   Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
   //the Ipv4InterfaceAddress associated to the interface and addressIndex
   Ipv4InterfaceAddress iface = l3->GetAddress (1, 0);
   Ptr<NetDevice> oif;
   //GetLocal() return local address.
   /*GetInterfaceForAddress (address)Return the interface number of the interface that has been
    *assigned the specified IP address.
    *Each IP interface has one or more IP addresses associated with it. This method searches the
    *list of interfaces for one that holds a particular address. This call takes an IP address as
    *a parameter and returns the interface number of the first interface that has been assigned
    *that address, or -1 if not found. There must be an exact match; this method will not match
    *broadcast or multicast addresses
    */
    //GetNetDevice() return pointer to NetDevice for the particualr interface
   oif =m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
   Ptr<Ipv4Route> route = Create<Ipv4Route> ();
 
   if(m_neighbors->isNeighbour (dst))
     {
       nextHop = dst;
       route->SetDestination (dst);
       route->SetGateway (nextHop);
 
      // FIXME: Does not work for multiple interfaces
      route->SetOutputDevice (m_ipv4->GetNetDevice (1));
   NS_LOG_LOGIC ("modified route destination is " << route->GetDestination() <<"source is " <<route->GetSource()<<"gateway is "<<route->GetGateway()<<"output device is "<<route->GetOutputDevice ());
     }
   else
    {
     Vector dstPos = m_nodesPosition.GetPosition (dst);
     nextHop = m_neighbors->BestNeighbor (dstPos, myPos, myVel);
     if (nextHop == Ipv4Address::GetZero ())
      {
        return false;
      }
     else
     {
     route->SetDestination (dst);
     route->SetGateway (nextHop);
     route->SetOutputDevice (m_ipv4->GetNetDevice (1));
   NS_LOG_LOGIC ("modified route destination is " << route->GetDestination() <<"source is " <<route->GetSource()<<"gateway is "<<route->GetGateway()<<"output device is "<<route->GetOutputDevice ());
     }
     }
   while (m_queue.Dequeue (dst, queueEntry))
     {
       DeferredRouteOutputTag tag;
       Ptr<Packet> p = ConstCast<Packet> (queueEntry.GetPacket ());
       UnicastForwardCallback ucb = queueEntry.GetUnicastForwardCallback ();
       Ipv4Header header = queueEntry.GetIpv4Header ();
   NS_LOG_LOGIC ("header destination is " << header.GetDestination () <<"source is " << header.GetSource () <<"packet id " << p->GetUid ());
      
       if (header.GetSource () == Ipv4Address ("102.102.102.102"))
         {
           route->SetSource (m_ipv4->GetAddress (1, 0).GetLocal ());
   NS_LOG_LOGIC ("modified route destination is " << route->GetDestination() <<"source is " <<route->GetSource()<<"gateway is "<<route->GetGateway()<<"output device is "<<route->GetOutputDevice ());
         }
       else
         {
           route->SetSource (header.GetSource ());
         
   NS_LOG_LOGIC ("modified route destination is " << route->GetDestination() <<"source is " <<route->GetSource()<<"gateway is "<<route->GetGateway()<<"output device is "<<route->GetOutputDevice ());
         }
   NS_LOG_LOGIC ("hello i'm ucb  "<<"route destination is "<<route->GetDestination() <<"source is " <<route->GetSource()<<"gateway is "<<route->GetGateway()<<"output device is "<<route->GetOutputDevice ()<<"header destination is " << header.GetDestination () <<"source is " << header.GetSource () <<"packet id " << p->GetUid ()<<"Packet Size: " << p->GetSize ());
       ucb (route, p, header);
    }
   return true;
  }
 
 void
 AeroRoutingProtocol::CheckQueue ()
 {
   NS_LOG_LOGIC (this);
   CheckQueueTimer.Cancel ();
 
   std::list<Ipv4Address> toRemove;
 
   for (std::list<Ipv4Address>::iterator i = m_queuedAddresses.begin (); i != m_queuedAddresses.end (); ++i)
     {
       if (SendPacketFromQueue (*i))
         {
           //Insert in a list to remove later
           toRemove.insert (toRemove.begin (), *i);
        NS_LOG_LOGIC ("insert queue for ");
         }
     }
 
   //remove all that are on the list
   for (std::list<Ipv4Address>::iterator i = toRemove.begin (); i != toRemove.end (); ++i)
     {
       m_queuedAddresses.remove (*i);
        NS_LOG_LOGIC ("removing queue for ");
     }
 
   if (!m_queuedAddresses.empty ()) //Only need to schedule if the queue is not empty
     {
       CheckQueueTimer.Schedule (Time ("60000ms"));
       //90000 512 and 4kbps bes 60000 1000 8kbps (60 node)
        NS_LOG_LOGIC ("timer has been rescheduled ");
     }
 }

 void
 AeroRoutingProtocol :: SetSecurityMode (bool mode)
 {
    m_SecurityState = mode;
 }

 void
 AeroRoutingProtocol :: SetAttackMode (bool mode)
 {
    m_AttackState = mode;
 }

 void 
 AeroRoutingProtocol :: AuthenticationIntialize ()
 {
   m_certificate = CertificateAuthority::GetCertificateAuthority();
   m_myCertReq = X509_REQ_new();
   m_myCert = X509_new();
   m_name = X509_NAME_new ();
   m_rsa_keyPair = RSA_new();
   m_puk  = EVP_PKEY_new();
   m_randomVariable = CreateObject<UniformRandomVariable> ();
   GenerateRSAKeyPair ();
   MakeSignedCertReq();
   SetCert();
   GenerateRandomNumber ();
   m_authenticationStatus = UNAUTH;

 } 

 void 
 AeroRoutingProtocol :: AuthenticationDestroy ()
 {
   X509_REQ_free(m_myCertReq);
   X509_free(m_myCert);
   RSA_free(m_rsa_keyPair);
 } 

 void
 AeroRoutingProtocol :: GenerateRSAKeyPair ( )
 {
     NS_LOG_LOGIC ("[Node "<<m_ipv4->GetObject<Node> ()->GetId ()<<"] Generating 2048 bit RSA key");
     m_rsa_keyPair = RSA_generate_key((8*RSA_KEY_SIZE),RSA_F4,NULL,NULL);
 }

 void 
 AeroRoutingProtocol :: SetPublicKey ()
 {
   EVP_PKEY_assign_RSA(m_puk,m_rsa_keyPair);
 }


  X509_REQ*
  AeroRoutingProtocol :: MakeSignedCertReq ()
  {
     NS_LOG_LOGIC ("Generating Certificate Request");
    //adds all digest algorithms to the table
    OpenSSL_add_all_digests();
    SetPublicKey();
    //include the public key in the req
    X509_REQ_set_pubkey(m_myCertReq,m_puk);
    //set the subject name of the request
    m_name=X509_REQ_get_subject_name(m_myCertReq);
    //set the request
    X509_NAME_add_entry_by_txt(m_name,"C",MBSTRING_ASC, (const unsigned char *)"UK", -1, -1, 0);
    X509_NAME_add_entry_by_txt(m_name,"CN",MBSTRING_ASC, (const unsigned char *)"OpenSSL Group", -1, -1, 0);
    //sign the req
    X509_REQ_sign(m_myCertReq,m_puk,EVP_sha1());
    return m_myCertReq;
  }


  void 
  AeroRoutingProtocol :: SetCert ()
  {
    NS_LOG_FUNCTION (this << "Creating Certificate");
    m_myCert = m_certificate->CreateCertificate (m_myCertReq);
/*
    FILE * fcert;
    char name[20];
    int i = m_ipv4->GetObject<Node> ()->GetId ();
    snprintf(name, 20, "cert%d.pem", i);
    fcert = fopen(name, "wb");
	 PEM_write_X509(
	     fcert,   
	     m_myCert 
	 );
     fclose(fcert);
*/
  }

  void
  AeroRoutingProtocol :: SetIdentification (uint8_t identification)
  {
    m_myIdentification = identification;
  }

  void
  AeroRoutingProtocol :: SetSharedKey ()
  {
    NS_LOG_FUNCTION (this << "Shared key has been created ");
    m_certificate->GenerateSharedKey (m_sharedKey , SHARED_KEY_SIZE);
  //NS_LOG_FUNCTION (this << "the shared key in GS is ");
   //for (int j = 0 ; j<SHARED_KEY_SIZE ; j++)
    //{
     // printf("0x%.2x ", m_sharedKey[j]);
    //}
    //cout<<endl;
  }

  uint32_t
  AeroRoutingProtocol :: GenerateRandomNumber ()
  {
  uint32_t rand = m_ipv4->GetObject<Node> ()->GetId ();
  m_myRandomNo = m_randomVariable->GetInteger (rand,(rand+1000));
  NS_LOG_FUNCTION (this << "the created random no is " << m_myRandomNo);
  return m_myRandomNo;
  }

  void 
  AeroRoutingProtocol :: AuthenticationTimerExpire ()
  {
  NS_LOG_FUNCTION (this << "begin Authentication process ");
   SendAuthenticationRequestPacket ();
   AuthenticationTimer.Cancel (); 
     // if (m_authenticationStatus == UNAUTH)
    //{
      //   AuthenticationTimer.Schedule (AuthenticationInterval + JITTER);
    //}
  }

  void
  AeroRoutingProtocol :: SendAuthenticationRequestPacket ()
  {
  NS_LOG_FUNCTION (this << "send authentication request ");
     //change crtificate into array to be put in packet but it is unsigned char not uint8_t
     int len = i2d_X509(m_myCert, NULL);
     unsigned char *buf, *p;
     buf = (unsigned char *)OPENSSL_malloc(len);
     p = buf;
     i2d_X509(m_myCert, &p);
     unsigned char certarray[len];
       for (int i = 0 ; i<len ; i++)
	 {
	  certarray[i] = *(p-len+i);
	 }

     Time timestamp = Simulator::Now ();
     //ID of GS is 0
     uint8_t identification = 0;
     //this function return int64_t am i correct for that
     uint64_t time = timestamp.GetMilliSeconds();
  NS_LOG_FUNCTION (this << "int sent time stamp "<<time<<" sent time stamp "<<timestamp);

     SetANData (time,m_myRandomNo,identification ,ANdata,AN_DATA_SIZE );

     //it is unsigned char not uint8_t
     unsigned char signedData [RSA_KEY_SIZE];
     SignData(ANdata, AN_DATA_SIZE ,signedData , m_rsa_keyPair);

    for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_authSocketAddresses.begin(); j != m_authSocketAddresses.end (); ++j)
    //for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin(); j != m_socketAddresses.end (); ++j)
     {
       Ptr<Socket> socket = j->first;
       Ipv4InterfaceAddress iface = j->second;

       ///still need to add the rest of feilds
  NS_LOG_FUNCTION (this << "auth req packet filling"<<" interface "<<iface);
       AuthenticationRequestHeader AuthenticateRequest (certarray , time , m_myRandomNo , identification, signedData);
 
       Ptr<Packet> packet = Create<Packet> ();

       packet->AddHeader (AuthenticateRequest);

       TypeHeader tHeader (AERORPTYPE_AUTHENTICATIONREQUEST);
       packet->AddHeader (tHeader);
       // Send to ground station 
       Ipv4Address destination;

           destination = GROUND_STATION_ADDRESS;

       //socket->SendTo (packet, 0, InetSocketAddress (destination, AERORP_PORT));
       socket->SendTo (packet, 0, InetSocketAddress (destination, AERORP_PORT));
     }
     
  }

  void
  AeroRoutingProtocol :: SetANData (uint64_t timer , uint32_t randno , uint8_t identifier , unsigned char ANdata[] , int size)
  {
  NS_LOG_FUNCTION (this << "set AN Data ");
//    uint64_t timer;
  //  timer = timestamp.GetMilliSeconds();
    
    for (int i = 0 ; i<(size-5) ; i++)
    {
      ANdata[i] = ((byte *)(&timer))[i];
    }
    for (int j = (size-5) ; j< (size-1) ; j++)
    {
      ANdata[j] = ((byte *)(&randno))[j-(size-5)];
    }
    for (int l = (size-1) ; l< size ; l++)
    {
      ANdata[l] = ((byte *)(&identifier))[l-(size-1)];
    }

  }


  void
  AeroRoutingProtocol :: SendAuthenticationReplyPacket (Ipv4Address source , uint8_t identification , uint32_t anrandno , RSA *senderpubkey)
  {

  NS_LOG_FUNCTION (this << "send authentication reply ");
     //it should be changed to uint8_t from unsigned char 
     int len = i2d_X509(m_myCert, NULL);
     unsigned char *buf, *p;
     buf = (unsigned char *)OPENSSL_malloc(len);
     p = buf;
     i2d_X509(m_myCert, &p);
     unsigned char certarray[len];
       for (int i = 0 ; i<len ; i++)
	 {
	  certarray[i] = *(p-len+i);
	 }

     Time timestamp = Simulator::Now ();
     uint64_t time = timestamp.GetMilliSeconds();

     uint32_t  gsrandno;
     gsrandno = m_myRandomNo;
     //encrypt using the pub key of the sender AN
     EncryptSharedKey(senderpubkey);
  //NS_LOG_FUNCTION (this <<"the encrypted shared key befor packet is ");
   //for (int j = 0 ; j<RSA_KEY_SIZE ; j++)
   // {
   //    printf("0x%.2x ", encryptedSharedKey[j]);
   // }
   // cout<<endl;

     SetGSData (time,gsrandno,anrandno,identification,encryptedSharedKey,GSdata ,GS_DATA_SIZE );
     NS_LOG_FUNCTION (this << " int time is "<<time<<"GSRandno "<<gsrandno<<" ANRandono "<<anrandno<<" ANID "<<identification);
     unsigned char signedData [RSA_KEY_SIZE];
     //the key pair should be the private of mine
     SignData(GSdata, GS_DATA_SIZE ,signedData , m_rsa_keyPair );
  NS_LOG_FUNCTION (this <<"the signed data before packet is ");
   //for (int j = 0 ; j<RSA_KEY_SIZE ; j++)
    //{
    //   printf("0x%.2x ", signedData[j]);
    //}
    //cout<<endl;

    for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_authSocketAddresses.begin(); 
        j != m_authSocketAddresses.end (); ++j)
     {
       Ptr<Socket> socket = j->first;
       Ipv4InterfaceAddress iface = j->second;

       ///still need to add the rest of feilds
       AuthenticationReplyHeader AuthenticateReply (certarray , time , gsrandno , anrandno, identification ,  encryptedSharedKey , signedData);
 
       Ptr<Packet> packet = Create<Packet> ();

       packet->AddHeader (AuthenticateReply);

       TypeHeader tHeader (AERORPTYPE_AUTHENTICATIONREPLY);
       packet->AddHeader (tHeader);
       // Send to the air borne node which send the request
       NS_LOG_FUNCTION (this << "printing the packet reply ");
       //cout<<AuthenticateReply;

       Ipv4Address destination;

           destination = source;

       socket->SendTo (packet, 0, InetSocketAddress (destination, AERORP_PORT));
     }

  }


  void
  AeroRoutingProtocol :: SetGSData (uint64_t timer , uint32_t gsrandno ,  uint32_t anrandno ,uint8_t identifier , unsigned char encryptedshkey[] , unsigned char GSdata[] , int size )
  {
  NS_LOG_FUNCTION (this << "set GS Data ");
    //uint64_t timer;
    //timer = timestamp.GetMilliSeconds();
    //size is 273
    for (int i = 0 ; i<(size-265) ; i++)
    {
      GSdata[i] = ((byte *)(&timer))[i];
    }
    for (int j = (size-265) ; j< (size-261) ; j++)
    {
      GSdata[j] = ((byte *)(&gsrandno))[j-(size-265)];
    }
    for (int j = (size-261) ; j< (size-257) ; j++)
    {
      GSdata[j] = ((byte *)(&anrandno))[j-(size-261)];
    }
    for (int l = (size-257) ; l< (size-256) ; l++)
    {
      GSdata[l] = ((byte *)(&identifier))[l-(size-257)];
    }
    for (int k = (size-256) ; k< size ; k++)
    {
      GSdata[k] = encryptedshkey[k-(size-256)];
      //GSdata[k] = ((byte *)(&encryptedSharedKey))[k-(size-256)];
    }
  //NS_LOG_FUNCTION (this <<"the gs data is ");
   //for (int j = 0 ; j<size ; j++)
    //{
     //  printf("0x%.2x ", GSdata[j]);
    //}
    //cout<<endl;
  }


/*
 * encrypts the plain text bytes at shared key (usually a session key) using the public key rsa and stores 
 * the ciphertext in ctext. ctext must point to RSA_size(rsa) bytes of memory. 
 */

  void 
  AeroRoutingProtocol :: EncryptSharedKey (RSA *receiverpubkey)
  {
  NS_LOG_FUNCTION (this << "Encrypt the Shared Key ");
  NS_LOG_FUNCTION (this << "the shared key is ");
   //for (int i = 0 ; i<SHARED_KEY_SIZE ; i++)
    //{
     // printf("0x%.2x ", m_sharedKey[i]);
    //}
    //cout<<endl;
   //encrypt with the pubkey of the reciver so m_rsa_keypair should be for the receiver
   int padding = RSA_PKCS1_PADDING;
   int size;
   size = RSA_public_encrypt(SHARED_KEY_SIZE,m_sharedKey,encryptedSharedKey,receiverpubkey,padding);
  NS_LOG_FUNCTION (this << "size "<<size<< "the encrypted shared key befor packet is ");
  // for (int j = 0 ; j<RSA_KEY_SIZE ; j++)
    //{
      // printf("0x%.2x ", encryptedSharedKey[j]);
    //}
    //cout<<endl;
  }

/*
  void
  AeroRoutingProtocol :: ReceiveAuthenticationPacket (Ptr<Socket> socket)
  {
  NS_LOG_FUNCTION (this << "receive authentication request ");
   X509 *sendercert = NULL;
   Time sendertimestamp;
   uint8_t myIdentification;
   uint32_t senderrandno;
   unsigned char recsigneddata[RSA_KEY_SIZE];
   uint8_t senderidentification = 0;
   uint32_t anrandomno = 0;
   uint32_t gsrandno;
   unsigned char recencryptedkey[RSA_KEY_SIZE];
   

   //NS_LOG_FUNCTION (this << socket);
   Address sourceAddress;
  ///Read a single packet from the socket and retrieve the sender address.return pointer to packet
  ///output parameter that will return the address of the sender of the received packet, if any.
 ///Remains untouched if no packet is received
   Ptr<Packet> packet = socket->RecvFrom (sourceAddress);   
   
   TypeHeader tHeader (AERORPTYPE_AUTHENTICATIONREQUEST);
   packet->RemoveHeader (tHeader);
   if (!tHeader.IsValid ())
     {
       NS_LOG_DEBUG ("aerorp message " << packet->GetUid () << " with unknown type received: " << tHeader.Get () << ". Ignored");
       return;
     }

   /// authentication request packet
   if (tHeader.Get() == AERORPTYPE_AUTHENTICATIONREQUEST)
    {
    //the sender is the AN and received data from the AN and the receiver is GS
   //extract data from packet then validate it
   AuthenticationRequestHeader hdr;
   packet->RemoveHeader (hdr);

   //extract cert from the packet and save it to array and change it to X509 and verify it
   //pass it to  CheckPacketValidity()
   uint8_t extractedcertarray[CERT_SIZE];
   unsigned char certarray[CERT_SIZE];
   hdr.GetCertificate(extractedcertarray);
   for (int i =0 ; i< CERT_SIZE ; i++)
    {
      certarray[i] = ((byte *)(&extractedcertarray))[i]; 
    }
    unsigned char *buf1;
    buf1 = certarray;
    const unsigned char *p1 = buf1;
    p1 = buf1;
    sendercert = d2i_X509(NULL, &p1, CERT_SIZE);

   EVP_PKEY *senderpubkey = NULL;
   senderpubkey = X509_get_pubkey(sendercert);
   RSA *senderPairs = NULL;
   //the pairs just have the pub key of the sender
   senderPairs = EVP_PKEY_get1_RSA(senderpubkey);

    uint64_t intTimeStamp = hdr.GetTimeStamp();
    sendertimestamp.FromInteger(intTimeStamp,Time::S);
    //the identification of the GS received from the AN packet
    myIdentification = hdr.GetIdentification();

    senderrandno = hdr.GetRandomNo();

    hdr.GetSign(recsigneddata);

   unsigned char recANdata[AN_DATA_SIZE];

   //how can i use it to return signed data we can pass the array and its size
    SetANData(sendertimestamp , senderrandno , myIdentification , recANdata , AN_DATA_SIZE);


   if (CheckPacketValidity(sendercert,sendertimestamp,myIdentification,recANdata,AN_DATA_SIZE,recsigneddata,RSA_KEY_SIZE,senderpubkey) == true)
     {
   InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
   Ipv4Address sender = inetSourceAddr.GetIpv4 ();
   //senderidentification is the identification of the AN 
   //senderrandno is the rand no that we recive from the AN
   SendAuthenticationReplyPacket(sender,senderidentification,senderrandno,senderPairs);
     }
 
   }
   /// authentication reply packet
   if (tHeader.Get() == AERORPTYPE_AUTHENTICATIONREPLY)
    {
   // the sender is the GS and reciver is AN and received data is GS data
   AuthenticationReplyHeader replyhdr;
   packet->RemoveHeader (replyhdr);

   uint8_t extractedcertarray[CERT_SIZE];
   unsigned char certarray[CERT_SIZE];
   replyhdr.GetCertificate(extractedcertarray);
   for (int i =0 ; i< CERT_SIZE ; i++)
    {
      certarray[i] = ((byte *)(&extractedcertarray))[i]; 
    }
    unsigned char *buf1;
    buf1 = certarray;
    const unsigned char *p1 = buf1;
    p1 = buf1;
    sendercert = d2i_X509(NULL, &p1, CERT_SIZE);

   EVP_PKEY *senderpubkey = NULL;
   senderpubkey = X509_get_pubkey(sendercert);

   //extract data from packet then validate it

    uint64_t intTimeStamp = replyhdr.GetTimeStamp();
    sendertimestamp.FromInteger(intTimeStamp,Time::S);
    // identification of the AN
    myIdentification = replyhdr.GetIdentification();

   gsrandno = replyhdr.GetRandomNo();
   anrandomno = replyhdr.GetOldRandomNo();
   
   //set the shared key
   replyhdr.GetSharedKey(recencryptedkey);

   replyhdr.GetSign(recsigneddata);
   unsigned char recGSdata[GS_DATA_SIZE];
   //how can i use it to return signed data we can pass the array and its size
   SetGSData(sendertimestamp , anrandomno , gsrandno , myIdentification ,recencryptedkey, recGSdata , GS_DATA_SIZE);
   
   if (CheckPacketValidity
(sendercert,sendertimestamp,myIdentification,recGSdata,GS_DATA_SIZE,recsigneddata,RSA_KEY_SIZE,senderpubkey) && (VerifyRandomNo(anrandomno))== true )
     {
       DecryptSharedKey (recencryptedkey , RSA_KEY_SIZE);
       m_authenticationStatus = AUTH;
     }
    else 
     m_authenticationStatus = UNAUTH;

    }

  }
*/

  bool
  AeroRoutingProtocol :: CheckPacketValidity (X509 *clientcert , Time timestamp ,uint8_t identification,unsigned char data[], int datasize,unsigned char sign[], size_t signsize , EVP_PKEY* senderpubkey)
  {
  NS_LOG_FUNCTION (this << "Chaecking Packet Validity ");
   bool idstatus;
   bool timestatus;
   bool certstatus;
   bool signstatus;
   
   idstatus = VerifyIdentification(identification);
   certstatus = VerifyCert (clientcert);
   timestatus = VerifyTimeStamp(timestamp);
   signstatus = VerifySign (data,datasize,sign,signsize ,senderpubkey);

  NS_LOG_FUNCTION (this << "id status "<<idstatus<<"cert status "<<certstatus<<"time status "<<timestatus<<"sign status "<<signstatus );
   if ((certstatus && timestatus && signstatus && idstatus) == true)
    {
    return true;
    }
    return false;
  }


 bool 
 AeroRoutingProtocol:: VerifyCert (X509 *clientcert)
 {
  NS_LOG_FUNCTION (this << "Verify certificate Validity ");
    int status = 0;
    X509_STORE_CTX *ctx = X509_STORE_CTX_new();

     //store the trusted cert into ctx
     X509_STORE *store = X509_STORE_new();
     X509_STORE_add_cert(store, m_certificate-> GetCert());

     //put the trusted cert and cert then verify it
     X509_STORE_CTX_init(ctx,store, clientcert, NULL);
     status  = X509_verify_cert(ctx);

	 if (status == 1)
	 {
           return true;
	 }
	 else
	 {
           return false;
	 }
	 return false;
 }


 bool 
 AeroRoutingProtocol:: VerifyTimeStamp (Time timestamp)
 {
  NS_LOG_FUNCTION (this << "Validate Time Stamp ");
   //td should be intialized
   Time td(Seconds (5));
   //Time td(Seconds (300));
   NS_LOG_FUNCTION (this << "recv time stamp "<<timestamp+td<<" now "<<Simulator::Now ());
   if ((timestamp + td) >= Simulator::Now ())
    return true;
   else
   return false;
 }


 bool 
 AeroRoutingProtocol:: VerifyRandomNo (uint32_t myrandno)
 {
  NS_LOG_FUNCTION (this << "Validate Random NO ");
   if (myrandno == m_myRandomNo)
    {
   return true;
    }
   else
   {
   return false;
   }
   return false;
 }


 bool 
 AeroRoutingProtocol :: VerifyIdentification (uint8_t identification)
 {
  NS_LOG_FUNCTION (this << "Validate Identification ");
   if (identification == m_ipv4->GetObject<Node> ()->GetId () )
    {
   return true;
    }
   else
   {
   return false;
   }
   return false;
 }


  bool 
  AeroRoutingProtocol :: VerifySign (unsigned char data[], int datasize,unsigned char sign[], size_t signsize , EVP_PKEY* senderpubkey)
  {
  NS_LOG_FUNCTION (this << "Validate Signed Data ");
    EVP_MD_CTX *mdctx = NULL;
    mdctx = EVP_MD_CTX_create();

    EVP_DigestVerifyInit(mdctx, NULL, EVP_sha1(), NULL, senderpubkey);
    EVP_DigestVerifyUpdate(mdctx, data, datasize);
     if(1 == EVP_DigestVerifyFinal(mdctx, sign, signsize))
	{
  NS_LOG_FUNCTION (this << "Validate Signed Data END TRUE");
		return true;
	}
     else
       {
  NS_LOG_FUNCTION (this << "Validate Signed Data END FALSE");
	return false;
          }
        return false;
  }

  void 
  AeroRoutingProtocol :: DecryptSharedKey (unsigned char encryptedkey[], int size)
  {
  NS_LOG_FUNCTION (this << "Decrypt the Shared Key ");
    int padding = RSA_PKCS1_PADDING;
    RSA_private_decrypt(size,encryptedkey,m_sharedKey,m_rsa_keyPair,padding);
  }


  void 
  AeroRoutingProtocol :: SignData (unsigned char data[],int size , unsigned char sign[], RSA *keypair)
  {
  NS_LOG_FUNCTION (this << "Sign Data ");
    OpenSSL_add_all_ciphers();
    OpenSSL_add_all_digests();

    size_t signlen = NULL;
    //create private key
    EVP_PKEY *priv_key = NULL;
    priv_key = EVP_PKEY_new();
    EVP_PKEY_set1_RSA(priv_key,keypair);

    EVP_MD_CTX *ctx = NULL;
    ctx = EVP_MD_CTX_create();
    //intialize the digest algorithm and private key
    EVP_DigestSignInit(ctx, NULL, EVP_sha1(), NULL, priv_key);
    //add the data and its length to the context
    EVP_DigestSignUpdate(ctx, data,size);
    //measure the length of the digest almost the RSA_KEY_SIZE
    EVP_DigestSignFinal(ctx, NULL, &signlen);
    //final signed digest
    EVP_DigestSignFinal(ctx, sign, &signlen);    
  }

  void
  AeroRoutingProtocol :: SetHelloPText (uint32_t positionX, uint32_t positionY,uint32_t positionZ,uint8_t VelocitySign,uint32_t nodeVelocityX,uint32_t nodeVelocityY,uint32_t nodeVelocityZ,unsigned char pText [], int pTextSize)
  {
    for (int i = 0 ; i<4 ; i++)
    {
       pText[i] =  ((byte *)(&positionX))[i];
    }
        for (int j = 4 ; j<8 ; j++)
    {
       pText[j] =  ((byte *)(&positionY))[j];
    }
        for (int k = 8 ; k<12 ; k++)
    {
       pText[k] =  ((byte *)(&positionZ))[k];
    }
        for (int l = 12 ; l<13 ; l++)
    {
       pText[l] =  VelocitySign;
    }
        for (int m = 13 ; m<17 ; m++)
    {
       pText[m] =  ((byte *)(&nodeVelocityX))[m];
    }
        for (int n = 17 ; n<21 ; n++)
    {
       pText[n] =  ((byte *)(&nodeVelocityY))[n];
    }
        for (int o = 21 ; o<25 ; o++)
    {
       pText[o] =  ((byte *)(&nodeVelocityZ))[o];
    }
  }


 void
 AeroRoutingProtocol::SendSecureHello ()
 {
/*
  NS_LOG_FUNCTION (this);
   double positionX;
   double positionY;
   double positionZ;

   double velocityX;
   double velocityY;
   double velocityZ;
 
   Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();
 
   positionX = MM->GetPosition ().x;
   positionY = MM->GetPosition ().y;
   positionZ = MM->GetPosition ().z;
   //NS_LOG_LOGIC ("node x  " << positionX <<"node y " <<positionY<<"node z "<<positionZ);
   velocityX = MM->GetVelocity ().x;
   velocityY = MM->GetVelocity ().y;
   velocityZ = MM->GetVelocity ().z;

   //NS_LOG_LOGIC ("vel x  " << velocityX <<"vel y " <<velocityY<<"vel z "<<velocityZ); 
   uint16_t VelocitySign = 0;   
   uint32_t nodeVelocityX = 0;
   uint32_t nodeVelocityY = 0;
   uint32_t nodeVelocityZ = 0;

   unsigned char helloPText [HELLO_PACKET_SIZE];
   unsigned char aad[AAD_LENGTH];
   unsigned char iv[IV_LENGTH];
   unsigned char cipherText[HELLO_PACKET_SIZE];
   unsigned char tag [TAG_SIZE];

   if (velocityX < 0 ||velocityY < 0 || velocityZ < 0 )
    {
        if ((velocityX < 0) && (velocityY < 0) && (velocityZ < 0) )
         {
           VelocitySign = 1;
           nodeVelocityX = (-1 * velocityX);
           nodeVelocityY = (-1 * velocityY);
           nodeVelocityZ = (-1 * velocityZ);
         }
      if ((velocityX < 0) && (velocityY < 0) && (velocityZ >= 0))
         {
           VelocitySign = 2;
           nodeVelocityX = (-1 * velocityX);
           nodeVelocityY = (-1 * velocityY);
           nodeVelocityZ = velocityZ;
         }
       if ((velocityX < 0) && (velocityY >=0) && (velocityZ < 0))
         {
           VelocitySign = 3;
           nodeVelocityX = (-1 * velocityX);
           nodeVelocityY = velocityY;
           nodeVelocityZ = (-1 * velocityZ);
         }
       if ((velocityX >= 0) && (velocityY < 0) && (velocityZ < 0))
         {
           VelocitySign = 4;
           nodeVelocityX = velocityX;
           nodeVelocityY = (-1 * velocityY);
           nodeVelocityZ = (-1 * velocityZ);
         }
       if ((velocityX < 0) && (velocityY >= 0) && (velocityZ >= 0))
         {
           VelocitySign = 5;
           nodeVelocityX = (-1 * velocityX);
           nodeVelocityY = velocityY;
           nodeVelocityZ = velocityZ;
         }
       if ((velocityX >= 0) && (velocityY < 0) && (velocityZ >= 0))
         {
           VelocitySign = 6;
           nodeVelocityX = velocityX;
           nodeVelocityY = (-1 * velocityY);
           nodeVelocityZ = velocityZ;
         }
       if ((velocityX >= 0) && (velocityY >= 0) && (velocityZ < 0))
         {
           VelocitySign = 7;
           nodeVelocityX = velocityX;
           nodeVelocityY = velocityY;
           nodeVelocityZ = (-1 * velocityZ);
         }

     }
    else
     {
           VelocitySign = 0;
           nodeVelocityX = velocityX;
           nodeVelocityY = velocityY;
           nodeVelocityZ = velocityZ;
     }

   //NS_LOG_LOGIC ("vel sign "<<static_cast<uint16_t> (VelocitySign)); 
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
     {
       Ptr<Socket> socket = j->first;
       Ipv4InterfaceAddress iface = j->second;

       Ptr<Packet> packet = Create<Packet> ();
       TypeHeader tHeader (AERORPTYPE_SECURE_HELLO);
       packet->AddHeader (tHeader);
   SetHelloPText(positionX,positionY,positionZ,VelocitySign,nodeVelocityX,nodeVelocityY,nodeVelocityZ,helloPText,HELLO_PACKET_SIZE);

       uint32_t noofbytes = PeekHeader(tHeader);
       packet->PeekHeader(tHeader);

       m_gcm->Encryption(helloPText,HELLO_PACKET_SIZE,aad,AAD_LENGTH,m_sharedKey,SHARED_KEY_SIZE,iv,IV_LENGTH,cipherText,tag);
       SecureHelloHeader secureHelloHeader (cipherText,tag);
 
       packet->AddHeader (secureHelloHeader);
       // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
       Ipv4Address destination;
       if (iface.GetMask () == Ipv4Mask::GetOnes ())
         {
           destination = Ipv4Address ("255.255.255.255");
         }
       else
         {
           destination = iface.GetBroadcast ();
         }

       m_txHelloTrace (packet);
       socket->SendTo (packet, 0, InetSocketAddress (destination, AERORP_PORT));
 
     }
*/
       /*
        * payload packet
        * uint32_t size = 1024;
        * Ptr<Packet> p = Create<Packet> (size);
        * uint8_t *buffer = ...;
        * uint32_t size = ...;
        * Ptr<Packet> p = Create<Packet> (buffer, size);
        */
/*
   TypeHeader tHeader (AERORPTYPE_HELLO);

    if (tHeader.Get() == AERORPTYPE_HELLO)
    {
         //p->RemoveHeader (tHeader);
	 HelloHeader hdr;
         //p->RemoveHeader (hdr);
cout << "Before serializing...";  
	hdr.Print(std::cout); 
cout << "done\n";        
	 Buffer helloBuf;
	 helloBuf.AddAtEnd(26);
cout << "Serializing header...";  	 
         hdr.Serialize(helloBuf.Begin()); 
cout << "done\n";  	 
	 uint8_t helloSerBuf[26];        
	 helloBuf.CopyData(helloSerBuf, 25);
cout << "After serializing...\n";
   for (int j =0 ; j< 25 ; j++)
    {
     printf("0x%.2x ", helloSerBuf[j]);
    }
    cout << endl;


    }
*/
/*
          //the sender is the AN and received data from the AN and the receiver is GS

  
     1-make check the packet data content CopyData()
     2- i want print the packet data to know the content of the data 
     3-check for the header and try to have a copy of it
     4- try yo make IV and add it to the packet as sequence no

   * i have a problem with the source its ip add is 102.102.102.102 > 10.1.1.255
    unsigned char AAD[20];
    int aadlen =20;
    haeder.Serialize(AAD);
   Ipv4Address src = m_ipv4->GetAddress (1, 0).GetLocal ();
   (*header).SetSource (src);
   NS_LOG_FUNCTION (this << header << (oif ? oif->GetIfIndex () : 0));
    
    unsigned char ptext[20];
    p->CopyData(ptext,size);

        * uint32_t size = 1024;
        * Ptr<Packet> securepacket = Create<Packet> (size);
        * uint8_t *buffer = ...;
        * uint32_t size = ...;
        * Ptr<Packet> securepacket = Create<Packet> (buffer, size);

     Ipv4Address src = m_ipv4->GetAddress (1, 0).GetLocal ();
     header.SetSource (m_ipv4->GetAddress (1, 0).GetLocal ());
   Buffer buf;
   buf = header;
   Iterator i(buf);
   
   unsigned char aad[20];
   header->Deserialize(i);
   */
    //question what is the this pointer mean????? and what does it mean nslog

 }


}
}
