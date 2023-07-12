/* The MIT License (MIT)
 * 
 * Copyright (c) 2015 mehdi sotoodeh
 * 
 * Permission is hereby granted, free of charge, to any person obtaining 
 * a copy of this software and associated documentation files (the 
 * "Software"), to deal in the Software without restriction, including 
 * without limitation the rights to use, copy, modify, merge, publish, 
 * distribute, sublicense, and/or sell copies of the Software, and to 
 * permit persons to whom the Software is furnished to do so, subject to 
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __curve25519_dh_key_exchange_h__
#define __curve25519_dh_key_exchange_h__

#ifdef __cplusplus
extern "C" {
#endif

/* Return public key associated with sk */
/* sk will be trimmed on return */
void curve25519_dh_CalculatePublicKey(
    unsigned char *pk,          /* [32-bytes] OUT: Public key */
    unsigned char *sk);         /* [32-bytes] IN/OUT: Your secret key */

/* Faster alternative for curve25519_dh_CalculatePublicKey */
/* sk will be trimmed on return */
void curve25519_dh_CalculatePublicKey_fast(
    unsigned char *pk,          /* [32-bytes] OUT: Public key */
    unsigned char *sk);         /* [32-bytes] IN/OUT: Your secret key */

/* sk will be trimmed on return */
void curve25519_dh_CreateSharedKey(
    unsigned char *shared,      /* [32-bytes] OUT: Created shared key */
    const unsigned char *pk,    /* [32-bytes] IN: Other side's public key */
    unsigned char *sk);         /* [32-bytes] IN/OUT: Your secret key */

#ifdef __cplusplus
}
#endif
#endif  /* __curve25519_dh_key_exchange_h__ */