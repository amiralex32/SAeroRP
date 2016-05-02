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

 #include "certificate-authority.h"
 #include <iostream>
 #include "ns3/log.h"

 NS_LOG_COMPONENT_DEFINE ("AeroRPCertificateAuthority");

 namespace ns3 {
 namespace AeroRP {

 NS_OBJECT_ENSURE_REGISTERED ( CertificateAuthority);


CertificateAuthority* CertificateAuthority::ca = NULL;

 TypeId
 CertificateAuthority::GetTypeId (void)
 {
   static TypeId tid = TypeId ("ns3::AeroRP::CertificateAuthority")
     .SetParent<Object>()
     .AddConstructor<CertificateAuthority> ();
   return tid;
 }


CertificateAuthority*  CertificateAuthority :: GetCertificateAuthority()
{
   if(NULL == ca){
    ca = new CertificateAuthority();  
   }
   return ca;
}


 CertificateAuthority :: CertificateAuthority () :m_serial (2)
 {
   m_myCert = X509_new();
   m_caKeyPairs = RSA_new();
   m_pukey  = EVP_PKEY_new();
   m_issuerName = X509_NAME_new();
   GenerateMyKeyPairs();
   CreateMyCertificate();
 }

 CertificateAuthority :: ~CertificateAuthority ()
 {
   X509_free(m_myCert);
   RSA_free(m_caKeyPairs);
   X509_NAME_free(m_issuerName);
 }


 X509* 
 CertificateAuthority :: CreateCertificate (X509_REQ *req)
 { 
  X509 *m_req_reply;
  m_req_reply = X509_new();
  X509_NAME *subject = NULL;
  EVP_PKEY *pkey = NULL;
 //extract public key from the request
  pkey = X509_REQ_get_pubkey(req);

  m_serial+=1;
  ASN1_INTEGER_set(X509_get_serialNumber(m_req_reply),m_serial);

  X509_gmtime_adj(X509_get_notBefore(m_req_reply), 0);
  X509_gmtime_adj(X509_get_notAfter(m_req_reply), 31536000L);

  X509_set_pubkey(m_req_reply, pkey);

  X509_NAME *issuerSubject = X509_get_subject_name(m_myCert);

  X509_set_issuer_name(m_req_reply, issuerSubject);

  //extract the subject of the request
  subject = X509_REQ_get_subject_name(req);
  X509_set_subject_name(m_req_reply, subject);

  X509_get_subject_name(m_req_reply);

  X509_sign(m_req_reply, m_pukey, EVP_sha1());
  return m_req_reply;
 }

void
 CertificateAuthority :: CreateMyCertificate ( )
 {
    NS_LOG_LOGIC ("Generating CA root certificate");
    // we use rsa pairs and assign it into evp_key
    SetPublicKey();
    // properties of the certificate
    //set the serial number
    ASN1_INTEGER_set(X509_get_serialNumber(m_myCert), 1);
    //set the time validity
    X509_gmtime_adj(X509_get_notBefore(m_myCert), 0);
    X509_gmtime_adj(X509_get_notAfter(m_myCert), 31536000L);
    //set the public key of the cert to be signed
    X509_set_pubkey(m_myCert, m_pukey);
    //this is a self-signed certificate, we set the name of the issuer to the name of the subject
     m_issuerName = X509_get_subject_name(m_myCert);

     X509_NAME_add_entry_by_txt(m_issuerName, "C",  MBSTRING_ASC,
	                            (unsigned char *)"CA", -1, -1, 0);
     X509_NAME_add_entry_by_txt(m_issuerName, "O",  MBSTRING_ASC,
	                            (unsigned char *)"MyCompany Inc.", -1, -1, 0);
     X509_NAME_add_entry_by_txt(m_issuerName, "CN", MBSTRING_ASC,
	                            (unsigned char *)"localhost", -1, -1, 0);
     //set the issuer name
     X509_set_issuer_name(m_myCert, m_issuerName);
     //sign the cert
     X509_sign(m_myCert, m_pukey, EVP_sha1());
/*
    FILE * fcert;
    fcert = fopen("CAcert", "wb");
   
    PEM_write_X509(
	     fcert,   
	     m_myCert 
	         );
   fclose(fcert);
*/
 }

 void
 CertificateAuthority :: GenerateMyKeyPairs ( )
 {
    NS_LOG_LOGIC ("Generating 2048 bit RSA key for CA");
    m_caKeyPairs = RSA_generate_key((8*RSA_KEY_SIZE),RSA_F4 , NULL , NULL);
 }

 void
 CertificateAuthority :: GenerateSharedKey (unsigned char key[], int size )
 {
  NS_LOG_FUNCTION ("CA creating shared key");
    for (int i = 0; i<size ; i++)
    {
	key[i] = i;
     }
 }

 void
 CertificateAuthority :: SetPublicKey ()
 {
   EVP_PKEY_assign_RSA(m_pukey,m_caKeyPairs);
 }

 X509*
 CertificateAuthority :: GetCert()
 {
   return m_myCert;
 }


}
}
