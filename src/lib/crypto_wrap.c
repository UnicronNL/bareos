/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2012-2012 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version three of the GNU Affero General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
/*
 * crypto_wrap.c Encryption key wrapping support functions
 *
 * crypto_wrap.c was based on sample code used in multiple
 * other projects and has the following copyright:
 *
 * - AES Key Wrap Algorithm (128-bit KEK) (RFC3394)
 *
 * Copyright (c) 2003-2004, Jouni Malinen <jkmaline@cc.hut.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * Adapted to Bacula by Marco van Wieringen, March 2012
 */

#include "bacula.h"

#ifdef HAVE_CRYPTO /* Is encryption enabled? */
#ifdef HAVE_OPENSSL /* How about OpenSSL? */

#include <openssl/aes.h>

/*
 * @kek: key encryption key (KEK)
 * @n: length of the wrapped key in 64-bit units; e.g., 2 = 128-bit = 16 bytes
 * @plain: plaintext key to be wrapped, n * 64 bit
 * @cipher: wrapped key, (n + 1) * 64 bit
 */
void aes_wrap(uint8_t *kek, int n, uint8_t *plain, uint8_t *cipher)
{
   uint8_t *a, *r, b[16];
   int i, j;
   AES_KEY key;

   a = cipher;
   r = cipher + 8;

   /*
    * 1) Initialize variables.
    */
   memset(a, 0xa6, 8);
   memcpy(r, plain, 8 * n);

   AES_set_encrypt_key(kek, 128, &key);

   /*
    * 2) Calculate intermediate values.
    * For j = 0 to 5
    *     For i=1 to n
    *      B = AES(K, A | R[i])
    *      A = MSB(64, B) ^ t where t = (n*j)+i
    *      R[i] = LSB(64, B)
    */
   for (j = 0; j <= 5; j++) {
      r = cipher + 8;
      for (i = 1; i <= n; i++) {
         memcpy(b, a, 8);
         memcpy(b + 8, r, 8);
         AES_encrypt(b, b, &key);
         memcpy(a, b, 8);
         a[7] ^= n * j + i;
         memcpy(r, b + 8, 8);
         r += 8;
      }
   }

   /* 3) Output the results.
    *
    * These are already in @cipher due to the location of temporary
    * variables.
    */
}

/*
 * @kek: key encryption key (KEK)
 * @n: length of the wrapped key in 64-bit units; e.g., 2 = 128-bit = 16 bytes
 * @cipher: wrapped key to be unwrapped, (n + 1) * 64 bit
 * @plain: plaintext key, n * 64 bit
 */
int aes_unwrap(uint8_t *kek, int n, uint8_t *cipher, uint8_t *plain)
{
   uint8_t a[8], *r, b[16];
   int i, j;
   AES_KEY key;

   /*
    * 1) Initialize variables.
    */
   memcpy(a, cipher, 8);
   r = plain;
   memcpy(r, cipher + 8, 8 * n);

   AES_set_decrypt_key(kek, 128, &key);

   /*
    * 2) Compute intermediate values.
    * For j = 5 to 0
    *     For i = n to 1
    *      B = AES-1(K, (A ^ t) | R[i]) where t = n*j+i
    *      A = MSB(64, B)
    *      R[i] = LSB(64, B)
    */
   for (j = 5; j >= 0; j--) {
      r = plain + (n - 1) * 8;
      for (i = n; i >= 1; i--) {
         memcpy(b, a, 8);
         b[7] ^= n * j + i;

         memcpy(b + 8, r, 8);
         AES_decrypt(b, b, &key);
         memcpy(a, b, 8);
         memcpy(r, b + 8, 8);
         r -= 8;
      }
   }

   /*
    * 3) Output results.
    *
    * These are already in @plain due to the location of temporary
    * variables. Just verify that the IV matches with the expected value.
    */
   for (i = 0; i < 8; i++) {
      if (a[i] != 0xa6) {
         return -1;
      }
   }

   return 0;
}

#else /* HAVE_OPENSSL */
#error No encryption library available
#endif /* HAVE_OPENSSL */

#else /* HAVE_CRYPTO */

/*
 * @kek: key encryption key (KEK)
 * @n: length of the wrapped key in 64-bit units; e.g., 2 = 128-bit = 16 bytes
 * @plain: plaintext key to be wrapped, n * 64 bit
 * @cipher: wrapped key, (n + 1) * 64 bit
 */
void aes_wrap(uint8_t *kek, int n, uint8_t *plain, uint8_t *cipher)
{
   memcpy(cipher, plain, n * 8);
}

/*
 * @kek: key encryption key (KEK)
 * @n: length of the wrapped key in 64-bit units; e.g., 2 = 128-bit = 16 bytes
 * @cipher: wrapped key to be unwrapped, (n + 1) * 64 bit
 * @plain: plaintext key, n * 64 bit
 */
int aes_unwrap(uint8_t *kek, int n, uint8_t *cipher, uint8_t *plain)
{
   memcpy(cipher, plain, n * 8);
   return 0;
}

#endif /* HAVE_CRYPTO */
