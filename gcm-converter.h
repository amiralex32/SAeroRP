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
#ifndef AeroRPGCMCONVERTER_H
#define AeroRPGCMCONVERTER_H

 #include <iostream>
 #include "ns3/header.h"
 #include "ns3/ipv4-address.h"
 #include "ns3/packet.h"
 #include "ns3/nstime.h"
 #include "ns3/enum.h"
 #include "ns3/simulator.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <openssl/asn1.h>
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/x509_vfy.h>
 #include <map>

 namespace ns3 {
 namespace AeroRP {

/*
 * we use this class just to convert the parameters of the GCM mode into string for all packets
 * GCM parameters are key, IV (seq no) , AAD (packet header) and data to be ciphered
 * GCM output is cipher text (data) and auth tag
 * recollect again the packet header, seq no,encrypted data, auth tag 
 * we will encrypt 3 typs of packets hello,GS,Data
 */

 class GCMConverter
  {
 public:
  int Encryption (unsigned char plaintext[], int ptextsize,unsigned char aad[], int aadlen, unsigned char key[],int keysize,unsigned char iv[],int ivsize, unsigned char ciphertext[], unsigned char tag[]);

  int Dencryption (unsigned char ciphertext[], int ctextsize,unsigned char aad[], int aadlen, unsigned char tag[], unsigned char key[],int keysize,unsigned char iv[],int ivsize, unsigned char plaintext[]);

};
 
}
}
#endif /* AeroRPPacket_H*/
