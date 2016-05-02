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
 #ifndef AERORPCA_H
 #define AERORPCA_H

 #define RSA_KEY_SIZE 256 //bytes
 #define SHARED_KEY_SIZE 32 //bytes

 #include <iostream>
 #include "ns3/output-stream-wrapper.h"
 #include "ns3/header.h"
 #include "ns3/ipv4-address.h"
 #include "ns3/nstime.h"
 #include "ns3/enum.h"
 #include "ns3/simulator.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <openssl/asn1.h>
 #include <openssl/ssl.h>
 #include <openssl/rsa.h>
 #include <openssl/conf.h>
 #include <openssl/x509.h>
 #include <map>

 namespace ns3 {
 namespace AeroRP {


 class CertificateAuthority : public Object
  {
 public:

  CertificateAuthority();
  ~CertificateAuthority();

  static CertificateAuthority* GetCertificateAuthority();
  static TypeId GetTypeId (void);

  X509 *CreateCertificate (X509_REQ *req);
  void CreateMyCertificate();
  

  void GenerateMyKeyPairs ( );
  void GenerateSharedKey (unsigned char key[], int size );

  void SetPublicKey ();
  X509* GetCert();



 private:
 
   static CertificateAuthority *ca;
   X509             *m_myCert;
   RSA              *m_caKeyPairs;
   EVP_PKEY         *m_pukey;
   X509_NAME        *m_issuerName;
   unsigned char     m_sharedKey[SHARED_KEY_SIZE];
   uint8_t           m_serial;
  };
}
}
#endif /* AERORPCA_H */
