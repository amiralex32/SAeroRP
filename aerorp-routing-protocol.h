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
 #ifndef AERORPROUTINGPROTOCOL_H
 #define AERORPROUTINGPROTOCOL_H

#define RSA_KEY_SIZE 256 //bytes
#define SHARED_KEY_SIZE 32 //bytes
#define CERT_SIZE 727
#define RN_SIZE 4 //bytes
#define AN_DATA_SIZE    13 //RN (4) + TS (8) + ID (1)
//RN (4) + TS (8) + ID (1) + RN2 (4) + ESK (RSA_KEY_SIZE)
#define GS_DATA_SIZE    (AN_DATA_SIZE + RN_SIZE + RSA_KEY_SIZE) 
#define SIGNED_DATA_SIZE RSA_KEY_SIZE
#define TAG_SIZE 16 //byte
#define HELLO_PACKET_SIZE 25 //byte
#define GS_PACKET_SIZE 44 //byte
#define AAD_LENGTH  8  //byte
#define IV_LENGTH 16 //byte
#define PACKET_DATA_LEN_BEFORE_ENCRYPTON 1000 //byte
#define PACKET_DATA_LEN_AFTER_ENCRYPTON 1016 //byte
#define PACKET_DATA_LEN_FINAL 1024 //byte


 #include <iostream>
 #include "ns3/ground-station.h"
 #include "ns3/gcm-converter.h"
 #include "ns3/nstime.h"
 #include "ns3/random-variable-stream.h"
 #include "ns3/god.h"
 #include "ns3/aerorp-packet.h"
 #include "ns3/aerorp-ntable.h"
 #include "ns3/aerorp-ptable.h"
 #include "ns3/aerorp-pqueue.h"
 #include "ns3/node.h"
 #include "ns3/output-stream-wrapper.h"
 #include "ns3/ipv4-routing-protocol.h"
 #include "ns3/ipv4-interface.h"
 #include "ns3/ipv4-l3-protocol.h"
 #include "ns3/ip-l4-protocol.h"
 #include "ns3/certificate-authority.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <openssl/rsa.h>
 #include <openssl/conf.h>
 #include <openssl/x509.h>
 #include <cryptopp/config.h>
 #include <map>


namespace ns3
{
namespace AeroRP
{	
/**
 * \ingroup aerorp
 * 
 * \brief aerorp routing protocol
 */
class AeroRoutingProtocol : public Ipv4RoutingProtocol
{
public:
  //return the routing type
  static TypeId GetTypeId (void);
  //identify the port of AODV
  static const uint32_t AERORP_PORT;

  /// c-tor
  AeroRoutingProtocol ();
  virtual ~AeroRoutingProtocol();
  /**
   * This method is called by Object::Dispose or by the object's 
   * destructor, whichever comes first.
   *
   * Subclasses are expected to implement their real destruction
   * code in an overriden version of this method and chain
   * up to their parent's implementation once they are done.
   * i.e., for simplicity, the destructor of every subclass should
   * be empty and its content should be moved to the associated
   * DoDispose method.
   *
   * It is safe to call GetObject from within this method.
   */
  virtual void DoDispose ();
  /**
   * \brief Query routing cache for an existing route, for an outbound packet
   *
   * This lookup is used by transport protocols.  It does not cause any
   * packet to be forwarded, and is synchronous.  Can be used for
   * multicast or unicast.  The Linux equivalent is ip_route_output()
   *
   * \param p packet to be routed.  Note that this method may modify the packet.
   *          Callers may also pass in a null pointer. 
   * \param header input parameter (used to form key to search for the route)
   * \param oif Output interface Netdevice.  May be zero, or may be bound via
   *            socket options to a particular output interface.
   * \param sockerr Output parameter; socket errno 
   *
   * \returns a code that indicates what happened in the lookup
   */

  ///\name From Ipv4RoutingProtocol
  //\{ method that inhereted from ipv4routingrotocol that return ptr of type ipv4route
  Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
  /**
   * \brief Route an input packet (to be forwarded or locally delivered)
   *
   * This lookup is used in the forwarding process.  The packet is
   * handed over to the Ipv4RoutingProtocol, and will get forwarded onward
   * by one of the callbacks.  The Linux equivalent is ip_route_input().
   * There are four valid outcomes, and a matching callbacks to handle each.
   *
   * \param p received packet
   * \param header input parameter used to form a search key for a route
   * \param idev Pointer to ingress network device
   * \param ucb Callback for the case in which the packet is to be forwarded
   *            as unicast
   * \param mcb Callback for the case in which the packet is to be forwarded
   *            as multicast
   * \param lcb Callback for the case in which the packet is to be locally
   *            delivered
   * \param ecb Callback to call if there is an error in forwarding
   * \returns true if the Ipv4RoutingProtocol takes responsibility for 
   *          forwarding or delivering the packet, false otherwise
   * \ it is inhereted from the ipv4routing protocol
   */ 
  bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                   LocalDeliverCallback lcb, ErrorCallback ecb);

  /**
   * \param interface the index of the interface we are being notified about
   *
   * Protocols are expected to implement this method to be notified of the state change of
   * an interface in a node.
   */
  virtual void NotifyInterfaceUp (uint32_t interface);
  /**
   * \param interface the index of the interface we are being notified about
   *
   * Protocols are expected to implement this method to be notified of the state change of
   * an interface in a node.
   */
  virtual void NotifyInterfaceDown (uint32_t interface);
  //virtual void AddHeaders (Ptr<Packet> p, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route);


  /**
   * \param interface the index of the interface we are being notified about
   * \param address a new address being added to an interface
   *
   * Protocols are expected to implement this method to be notified whenever
   * a new address is added to an interface. Typically used to add a 'network route' on an
   * interface. Can be invoked on an up or down interface.
   */
  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
  /**
   * \param interface the index of the interface we are being notified about
   * \param address a new address being added to an interface
   *
   * Protocols are expected to implement this method to be notified whenever
   * a new address is removed from an interface. Typically used to remove the 'network route' of an
   * interface. Can be invoked on an up or down interface.
   */
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
  /**
   * \param ipv4 the ipv4 object this routing protocol is being associated with
   * 
   * Typically, invoked directly or indirectly from ns3::Ipv4::SetRoutingProtocol
   */
  virtual void SetIpv4 (Ptr<Ipv4> ipv4);
//this method is used from the ipv4routing protocol just to print out the route
  virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const;
  /*
   *\breif give decision for what to do when receive a control packet
   */
  virtual void RecvAERORP (Ptr<Socket> socket);
  /*
   *\breif update neighbour table 
   */
  virtual void UpdateRouteToNeighbor (Ipv4Address sender, Vector Pos , Vector Vel );
  /*
   *\breif update position table for all the nodes from the ground station control packets
   */
  virtual void UpdateRouteToNodes (Ipv4Address node, Vector Pos, Vector vel );
  /*
   *\breif send hello packet
   */
   virtual void SendHello ();
  /*
   *\breif send hello packet from attacker
   */
   virtual void SendBadHello ();
  /*
   *\breif send control packets
   */
   virtual void SendGroundStationPackets ();
   /*
    *\breif check if the message come to me or not
    */
   virtual bool IsMyOwnAddress (Ipv4Address src);
	
   Ptr<Ipv4> m_ipv4;
	/// Raw socket per each IP interface, map socket -> iface address (IP   mask)
    /// what is ip interface address and socket and ipv4route?????
   std::map< Ptr<Socket>, Ipv4InterfaceAddress > m_socketAddresses;
   std::map< Ptr<Socket>, Ipv4InterfaceAddress > m_authSocketAddresses;
   /// Loopback device used to defer RREQ until packet will be fully formed
   Ptr<NetDevice> m_lo;
   ///define the hello interval
   Time HelloInterval;
   
   uint32_t GetProtocolNumber (void) const;
   void SetDownTarget (IpL4Protocol::DownTargetCallback callback);
   IpL4Protocol::DownTargetCallback GetDownTarget (void) const;

   void SetGroundStation();
   void SetAttacker();

   void SetSecurityMode (bool mode);
   void SetAttackMode (bool mode);
/// Start protocol operation
  void Start (uint32_t noofnodes);
private:
  
  /// Queue packet and send route request
  void DeferredRouteOutput (Ptr<const Packet> p, const Ipv4Header & header, UnicastForwardCallback ucb, ErrorCallback ecb);
  /// timer for the hello packet.
  void HelloTimerExpire ();
  /// timer for the ground station packet.
  void GSTimerExpire ();
  /// Queue packet and send route request
  Ptr<Ipv4Route> LoopbackRoute (const Ipv4Header & header, Ptr<NetDevice> oif);
  /// If route exists and valid, forward packet.
  bool Forwarding (Ptr<const Packet> p, const Ipv4Header & header, UnicastForwardCallback ucb, ErrorCallback ecb);
  /// Find socket with local interface address iface
  Ptr<Socket> FindSocketWithInterfaceAddress (Ipv4InterfaceAddress iface) const;
  //Check packet from deffered route output queue and send if position is already available
  //returns true if the IP should be erased from the list (was sent/droped)
  bool SendPacketFromQueue (Ipv4Address dst);
 
  //Calls SendPacketFromQueue and re-schedules
  void CheckQueue ();
  
   uint32_t MaxQueueLen;  ///< The maximum number of packets that we allow a routing protocol to buffer.

   ///< The maximum period of time that a routing protocol is allowed to buffer a packet for.
   Time MaxQueueTime;
   RequestQueue m_queue;
 
   
   uint8_t LocationServiceName;
   NodesPositionTable m_nodesPosition;
   Ptr<NeighborTable> m_neighbors;
   std::list<Ipv4Address> m_queuedAddresses;
   Ptr<GroundStation> m_godGroundStation;
   Timer HelloIntervalTimer;
   Timer CheckQueueTimer;
   Timer GSTimer;
   Time GSInterval;
   bool m_isGroundStation;
   bool m_isAttacker;
 //define mode of operation if true secure aero will operate
   bool m_SecurityState;
 //define mode of operation if true attack will begin
   bool m_AttackState;
   IpL4Protocol::DownTargetCallback m_downTarget;

  TracedCallback<Ptr<const Packet> > m_txHelloTrace;
  TracedCallback<Ptr<const Packet> > m_txGsTrace;

  TracedCallback<Ptr<const Packet> > m_rxHelloTrace;
  TracedCallback<Ptr<const Packet> > m_rxGsTrace;

  TracedCallback<Ptr<const Packet> > m_rxSHelloTrace;
  TracedCallback<Ptr<const Packet> > m_rxSGsTrace;

  TracedCallback<Ptr<const Packet> > m_txSGsTrace;
  TracedCallback<Ptr<const Packet> > m_txSHelloTrace;
  
  //begin of authentication and key transport
   // attributes for 
 enum STATE {
	  UNAUTH	= 0,
	  AUTH		= 1,
             };

   X509_REQ            *m_myCertReq;
   X509                *m_myCert;
   X509_NAME           *m_name;
   RSA                 *m_rsa_keyPair;//RSA structure contain both private and public key
   EVP_PKEY            *m_puk;
   uint32_t             m_myRandomNo;
   uint8_t              m_myIdentification;
   unsigned char        m_sharedKey[SHARED_KEY_SIZE];
   STATE                m_authenticationStatus;

 
   unsigned char       ANdata[AN_DATA_SIZE];
   unsigned char       GSdata[GS_DATA_SIZE];
   unsigned char       encryptedSharedKey [RSA_KEY_SIZE];

   unsigned char       GCMAADTOBECHECKED[AAD_LENGTH];

   Timer AuthenticationTimer;
   Time AuthenticationInterval; 

   Ptr<UniformRandomVariable> m_randomVariable;

   CertificateAuthority * m_certificate;
   GCMConverter  m_gcm;

   void AuthenticationIntialize ();
   void AuthenticationDestroy ();
   // behaviors for making authentication and key transport
   // those functions used to intialize the cert and keys should be used in the constructor before 
   // the begeining of the x509 protocol
   void GenerateRSAKeyPair ();
   void SetPublicKey ();
   X509_REQ *MakeSignedCertReq();
   void SetCert ();

   void SetIdentification(uint8_t identification);
   //just for ground station which will send it through authenticationreply packet
   void SetSharedKey ();

   // those functions used to made the packet of x509 protocol
   uint32_t  GenerateRandomNumber ();

   void AuthenticationTimerExpire();
   //used in authenticationrequest packet
   void SendAuthenticationRequestPacket ();
   void SetANData (uint64_t timer , uint32_t randno , uint8_t identifier , unsigned char ANdata[] , int size );

   //used in authenticationreply packet
   void SendAuthenticationReplyPacket (Ipv4Address source , uint8_t identification , uint32_t anrandno,RSA *receiverpubkey);
   void SetGSData (uint64_t timer , uint32_t gsrandno ,  uint32_t anrandno ,uint8_t identifier , unsigned char encryptedshkey[], unsigned char GSdata[] , int size );
   void EncryptSharedKey (RSA *receiverpubkey);

   //after receive the authentication packet we verify authenticity of the packet
   //void ReceiveAuthenticationPacket (Ptr<Socket> socket);

   //verify the packets
   bool CheckPacketValidity (X509 *clientcert , Time timestamp ,uint8_t identification,unsigned char data[], int datasize,unsigned char sign[], size_t signsize , EVP_PKEY* senderpubkey);
   bool VerifyCert (X509 *clientcert);
   bool VerifyTimeStamp(Time timestamp);
   bool VerifyRandomNo (uint32_t myrandno);
   bool VerifyIdentification (uint8_t identification);
   bool VerifySign(unsigned char data[], int datasize,unsigned char sign[], size_t signsize , EVP_PKEY* senderpubkey);

   //used to extract the shared key from the authenticationreply packet
   void DecryptSharedKey (unsigned char encryptedkey[], int size);
   //used to sign all the data
   void SignData(unsigned char data[],int size , unsigned char sign[], RSA *keypair);
 
   //begin of network security 
   //Encrypt the HELLO Message
   void SetHelloPText (uint32_t positionX, uint32_t positionY,uint32_t positionZ,uint8_t VelocitySign,uint32_t nodeVelocityX,uint32_t nodeVelocityY,uint32_t nodeVelocityZ,unsigned char pText [], int pTextSize);

  /*
   *\breif send secure hello packet
   */
   virtual void SendSecureHello ();
  /*
   *\breif send secure control packets
   */
   //virtual void SendSecureGroundStationPackets ();


};

  std::ostream& operator<< (std::ostream& os, const Packet &);

}
}
#endif /* AERORPROUTINGPROTOCOL_H */
