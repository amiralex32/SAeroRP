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

#include "cryptopp/hex.h"
using CryptoPP::HexEncoder;
using CryptoPP::HexDecoder;

#include "cryptopp/cryptlib.h"
using CryptoPP::BufferedTransformation;
using CryptoPP::AuthenticatedSymmetricCipher;

#include "cryptopp/filters.h"
using CryptoPP::StringSink;
using CryptoPP::StringSource;
using CryptoPP::AuthenticatedEncryptionFilter;
using CryptoPP::AuthenticatedDecryptionFilter;

#include "cryptopp/aes.h"
using CryptoPP::AES;

#include "cryptopp/gcm.h"
using CryptoPP::GCM;
using CryptoPP::GCM_TablesOption;

#include "assert.h"

#include <iostream>
 #include "gcm-converter.h"
 #include "ns3/packet.h"
#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/enum.h"
#include "ns3/simulator.h"
 #include "ns3/log.h"
#include <map>

 using namespace std;
 NS_LOG_COMPONENT_DEFINE ("AeroRPGcmConverter");

 namespace ns3 {
 namespace AeroRP {

 int
 GCMConverter::Encryption (unsigned char plaintext[], int ptextsize,unsigned char aad[], int aadlen, unsigned char key[],int keysize,unsigned char iv[],int ivsize, unsigned char ciphertext[], unsigned char tag[])
{
  int len;
  int ciphertext_len;
  EVP_CIPHER_CTX *ctx;
  ctx = EVP_CIPHER_CTX_new();

  /* Create and initialise the context */
  ctx = EVP_CIPHER_CTX_new();
  /* Initialise the encryption operation. */
  EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
  /* Set IV length if default 12 bytes (96 bits) is not appropriate */
  EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 16, NULL);
  /* Initialise key and IV */
  EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv);
  /* Provide any AAD data.*/
  EVP_EncryptUpdate(ctx, NULL, &len, aad, aadlen);
  /* Provide the message to be encrypted, and obtain the encrypted output*/
  EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, ptextsize);
  ciphertext_len = len;
  /* Finalise the encryption. Normally ciphertext bytes may be written at this stage*/
  if (1 == EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
    {
     NS_LOG_FUNCTION (this << " success final encryption " );
    } 
  else
    {
     NS_LOG_FUNCTION (this << " failed to encrypt " );
    }
  ciphertext_len += len;
  /* Get the tag */
  if (1 == EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag))
   {
     NS_LOG_FUNCTION (this << " success create auth tag "<<" cipher len "<<ciphertext_len );
   }
  else
   {
     NS_LOG_FUNCTION (this << " failed to create auth tag " );
   }

  EVP_CIPHER_CTX_free(ctx);
  return ciphertext_len;
}

  int 
  GCMConverter::Dencryption (unsigned char ciphertext[], int ctextsize,unsigned char aad[], int aadlen, unsigned char tag[], unsigned char key[],int keysize,unsigned char iv[],int ivsize, unsigned char plaintext[])
   {
    int ret = -1;
    int len;
    int plaintext_len = 0;
    EVP_CIPHER_CTX *ctx;
    ctx = EVP_CIPHER_CTX_new();
    //Initialize the encryption operation
    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
    //Set IV length should be more than 12 byte or 96 bit normally 16
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, ivsize, NULL);
    //Initialize key and IV
    EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv);
    //add AAD data
    EVP_DecryptUpdate(ctx, NULL, &len, aad, aadlen);
    //Decrypt the message
    EVP_DecryptUpdate(ctx, plaintext, &len , ciphertext, ctextsize);
    plaintext_len = len;
    //add the tag that will be verified
    if (1== EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag))
      {
        NS_LOG_FUNCTION (this << " success compare of auth tag "  );
      }
    else
      {
        NS_LOG_FUNCTION (this << " auth tag is not the same "  );
      }
    //finalize the Decryption
    ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

    EVP_CIPHER_CTX_free(ctx);

      if(ret > 0)
	{
	 plaintext_len += len;
         NS_LOG_FUNCTION (this << " final decryption success "<<ret <<" ptext len is "<<plaintext_len );
	 return ret;
	}
     else
	{
            return -1;
	}
   }
 
}
}
