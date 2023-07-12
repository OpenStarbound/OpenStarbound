/* crypto/sha/sha512.c */
/* ====================================================================
 * Copyright (c) 2004 The OpenSSL Project.  All rights reserved
 * according to the OpenSSL license [found in ../../LICENSE].
 * ====================================================================
 */
/*
 * IMPLEMENTATION NOTES.
 *
 * As you might have noticed 32-bit hash algorithms:
 *
 * - permit SHA_LONG to be wider than 32-bit (case on CRAY);
 * - optimized versions implement two transform functions: one operating
 *   on [aligned] data in host byte order and one - on data in input
 *   stream byte order;
 * - share common byte-order neutral collector and padding function
 *   implementations, ../md32_common.h;
 *
 * Neither of the above applies to this SHA-512 implementations. Reasons
 * [in reverse order] are:
 *
 * - it's the only 64-bit hash algorithm for the moment of this writing,
 *   there is no need for common collector/padding implementation [yet];
 * - by supporting only one transform function [which operates on
 *   *aligned* data in input stream byte order, big-endian in this case]
 *   we minimize burden of maintenance in two ways: a) collector/padding
 *   function is simpler; b) only one transform function to stare at;
 * - SHA_LONG64 is required to be exactly 64-bit in order to be able to
 *   apply a number of optimizations to mitigate potential performance
 *   penalties caused by previous design decision;
 *
 * Caveat lector.
 *
 * Implementation relies on the fact that "long long" is 64-bit on
 * both 32- and 64-bit platforms. If some compiler vendor comes up
 * with 128-bit long long, adjustment to sha.h would be required.
 * As this implementation relies on 64-bit integer type, it's totally
 * inappropriate for platforms which don't support it, most notably
 * 16-bit platforms.
 *                  <appro@fy.chalmers.se>
 */
#include <string.h>
#include "../include/external_calls.h"
#include "sha512.h"

#define UINT64(X)   X##ULL

void SHA512_Transform (SHA512_CTX *ctx, const void *in);

void SHA512_Init (SHA512_CTX *c)
{
    c->h[0]=UINT64(0x6a09e667f3bcc908);
    c->h[1]=UINT64(0xbb67ae8584caa73b);
    c->h[2]=UINT64(0x3c6ef372fe94f82b);
    c->h[3]=UINT64(0xa54ff53a5f1d36f1);
    c->h[4]=UINT64(0x510e527fade682d1);
    c->h[5]=UINT64(0x9b05688c2b3e6c1f);
    c->h[6]=UINT64(0x1f83d9abfb41bd6b);
    c->h[7]=UINT64(0x5be0cd19137e2179);

    c->Nl=0;
    c->Nh=0;
    c->num=0;
    c->md_len=SHA512_DIGEST_LENGTH;
}

void SHA512_Final (unsigned char *md, SHA512_CTX *c)
{
    unsigned char *p=(unsigned char *)c->u.p;
    size_t n=c->num;

    p[n]=0x80;  /* There always is a room for one */
    n++;
    if (n > (SHA512_CBLOCK-16))
        mem_fill (p+n,0,SHA512_CBLOCK-n), n=0,
        SHA512_Transform (c,p);

    mem_fill (p+n,0,SHA512_CBLOCK-16-n);
#ifdef  ECP_CONFIG_BIG_ENDIAN
    c->u.d[SHA_LBLOCK-2] = c->Nh;
    c->u.d[SHA_LBLOCK-1] = c->Nl;
#else
    p[SHA512_CBLOCK-1]  = (unsigned char)(c->Nl);
    p[SHA512_CBLOCK-2]  = (unsigned char)(c->Nl>>8);
    p[SHA512_CBLOCK-3]  = (unsigned char)(c->Nl>>16);
    p[SHA512_CBLOCK-4]  = (unsigned char)(c->Nl>>24);
    p[SHA512_CBLOCK-5]  = (unsigned char)(c->Nl>>32);
    p[SHA512_CBLOCK-6]  = (unsigned char)(c->Nl>>40);
    p[SHA512_CBLOCK-7]  = (unsigned char)(c->Nl>>48);
    p[SHA512_CBLOCK-8]  = (unsigned char)(c->Nl>>56);
    p[SHA512_CBLOCK-9]  = (unsigned char)(c->Nh);
    p[SHA512_CBLOCK-10] = (unsigned char)(c->Nh>>8);
    p[SHA512_CBLOCK-11] = (unsigned char)(c->Nh>>16);
    p[SHA512_CBLOCK-12] = (unsigned char)(c->Nh>>24);
    p[SHA512_CBLOCK-13] = (unsigned char)(c->Nh>>32);
    p[SHA512_CBLOCK-14] = (unsigned char)(c->Nh>>40);
    p[SHA512_CBLOCK-15] = (unsigned char)(c->Nh>>48);
    p[SHA512_CBLOCK-16] = (unsigned char)(c->Nh>>56);
#endif

    SHA512_Transform (c,p);

    if (md) for (n=0; n < SHA512_DIGEST_LENGTH/8; n++)
    {
        M64 m;
        m.u64 = c->h[n];
        *(md++) = m.u8.b7;
        *(md++) = m.u8.b6;
        *(md++) = m.u8.b5;
        *(md++) = m.u8.b4;
        *(md++) = m.u8.b3;
        *(md++) = m.u8.b2;
        *(md++) = m.u8.b1;
        *(md++) = m.u8.b0;
    }
}

void SHA512_Update (SHA512_CTX *c, const void *_data, size_t len)
{
    SHA_LONG64 l;
    unsigned char *p=c->u.p;
    const unsigned char *data=(const unsigned char *)_data;

    if (len==0) return;

    l = (c->Nl+(((SHA_LONG64)len)<<3))&UINT64(0xffffffffffffffff);
    if (l < c->Nl) c->Nh++;
    if (sizeof(len)>=8) c->Nh+=(((SHA_LONG64)len)>>61);
    c->Nl=l;

    if (c->num != 0)
    {
        size_t n = SHA512_CBLOCK - c->num;

        if (len < n)
        {
            memcpy (p+c->num,data,len), c->num += (unsigned int)len;
            return;
        }
        else    
        {
            memcpy (p+c->num,data,n), c->num = 0;
            len-=n, data+=n;
            SHA512_Transform (c,p);
        }
    }

    while (len >= SHA512_CBLOCK)
    {
        SHA512_Transform (c,data);//,len/SHA512_CBLOCK),
        data += SHA512_CBLOCK;
        len  -= SHA512_CBLOCK;
    }

    if (len != 0)
        memcpy (p,data,len), c->num = (int)len;
}

static const SHA_LONG64 K512[80] = 
{
    UINT64(0x428a2f98d728ae22),UINT64(0x7137449123ef65cd),
    UINT64(0xb5c0fbcfec4d3b2f),UINT64(0xe9b5dba58189dbbc),
    UINT64(0x3956c25bf348b538),UINT64(0x59f111f1b605d019),
    UINT64(0x923f82a4af194f9b),UINT64(0xab1c5ed5da6d8118),
    UINT64(0xd807aa98a3030242),UINT64(0x12835b0145706fbe),
    UINT64(0x243185be4ee4b28c),UINT64(0x550c7dc3d5ffb4e2),
    UINT64(0x72be5d74f27b896f),UINT64(0x80deb1fe3b1696b1),
    UINT64(0x9bdc06a725c71235),UINT64(0xc19bf174cf692694),
    UINT64(0xe49b69c19ef14ad2),UINT64(0xefbe4786384f25e3),
    UINT64(0x0fc19dc68b8cd5b5),UINT64(0x240ca1cc77ac9c65),
    UINT64(0x2de92c6f592b0275),UINT64(0x4a7484aa6ea6e483),
    UINT64(0x5cb0a9dcbd41fbd4),UINT64(0x76f988da831153b5),
    UINT64(0x983e5152ee66dfab),UINT64(0xa831c66d2db43210),
    UINT64(0xb00327c898fb213f),UINT64(0xbf597fc7beef0ee4),
    UINT64(0xc6e00bf33da88fc2),UINT64(0xd5a79147930aa725),
    UINT64(0x06ca6351e003826f),UINT64(0x142929670a0e6e70),
    UINT64(0x27b70a8546d22ffc),UINT64(0x2e1b21385c26c926),
    UINT64(0x4d2c6dfc5ac42aed),UINT64(0x53380d139d95b3df),
    UINT64(0x650a73548baf63de),UINT64(0x766a0abb3c77b2a8),
    UINT64(0x81c2c92e47edaee6),UINT64(0x92722c851482353b),
    UINT64(0xa2bfe8a14cf10364),UINT64(0xa81a664bbc423001),
    UINT64(0xc24b8b70d0f89791),UINT64(0xc76c51a30654be30),
    UINT64(0xd192e819d6ef5218),UINT64(0xd69906245565a910),
    UINT64(0xf40e35855771202a),UINT64(0x106aa07032bbd1b8),
    UINT64(0x19a4c116b8d2d0c8),UINT64(0x1e376c085141ab53),
    UINT64(0x2748774cdf8eeb99),UINT64(0x34b0bcb5e19b48a8),
    UINT64(0x391c0cb3c5c95a63),UINT64(0x4ed8aa4ae3418acb),
    UINT64(0x5b9cca4f7763e373),UINT64(0x682e6ff3d6b2b8a3),
    UINT64(0x748f82ee5defb2fc),UINT64(0x78a5636f43172f60),
    UINT64(0x84c87814a1f0ab72),UINT64(0x8cc702081a6439ec),
    UINT64(0x90befffa23631e28),UINT64(0xa4506cebde82bde9),
    UINT64(0xbef9a3f7b2c67915),UINT64(0xc67178f2e372532b),
    UINT64(0xca273eceea26619c),UINT64(0xd186b8c721c0c207),
    UINT64(0xeada7dd6cde0eb1e),UINT64(0xf57d4f7fee6ed178),
    UINT64(0x06f067aa72176fba),UINT64(0x0a637dc5a2c898a6),
    UINT64(0x113f9804bef90dae),UINT64(0x1b710b35131c471b),
    UINT64(0x28db77f523047d84),UINT64(0x32caab7b40c72493),
    UINT64(0x3c9ebe0a15c9bebc),UINT64(0x431d67c49c100d4c),
    UINT64(0x4cc5d4becb3e42b6),UINT64(0x597f299cfc657e2a),
    UINT64(0x5fcb6fab3ad6faec),UINT64(0x6c44198c4a475817) 
};

#define B(x,j)    (((SHA_LONG64)(*(((const unsigned char *)(&x))+j)))<<((7-j)*8))
#define PULL64(x) (B(x,0)|B(x,1)|B(x,2)|B(x,3)|B(x,4)|B(x,5)|B(x,6)|B(x,7))
#define ROTR(x,s)   (((x)>>s) | (x)<<(64-s))

#define Sigma0(x)   (ROTR((x),28) ^ ROTR((x),34) ^ ROTR((x),39))
#define Sigma1(x)   (ROTR((x),14) ^ ROTR((x),18) ^ ROTR((x),41))
#define sigma0(x)   (ROTR((x),1)  ^ ROTR((x),8)  ^ ((x)>>7))
#define sigma1(x)   (ROTR((x),19) ^ ROTR((x),61) ^ ((x)>>6))

#define Ch(x,y,z)   (((x) & (y)) ^ ((~(x)) & (z)))
#define Maj(x,y,z)  (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define ROUND_00_15(i,a,b,c,d,e,f,g,h) do { \
    T1 += h + Sigma1(e) + Ch(e,f,g) + K512[i]; \
    h = Sigma0(a) + Maj(a,b,c); \
    d += T1; h += T1; } while (0)

#define ROUND_16_80(i,j,a,b,c,d,e,f,g,h,X) do { \
    s0 = X[(j+1)&0x0f]; s0 = sigma0(s0); \
    s1 = X[(j+14)&0x0f]; s1 = sigma1(s1); \
    T1 = X[(j)&0x0f] += s0 + s1 + X[(j+9)&0x0f]; \
    ROUND_00_15(i+j,a,b,c,d,e,f,g,h); } while (0)

void SHA512_Transform (SHA512_CTX *ctx, const void *in)
{
    const SHA_LONG64 *W = (SHA_LONG64*)in;
    SHA_LONG64  a,b,c,d,e,f,g,h,s0,s1,T1;
    SHA_LONG64  X[16];
    int i;

    a = ctx->h[0];  b = ctx->h[1];  c = ctx->h[2];  d = ctx->h[3];
    e = ctx->h[4];  f = ctx->h[5];  g = ctx->h[6];  h = ctx->h[7];

#ifdef ECP_CONFIG_BIG_ENDIAN
    T1 = X[0] = W[0];   ROUND_00_15(0,a,b,c,d,e,f,g,h);
    T1 = X[1] = W[1];   ROUND_00_15(1,h,a,b,c,d,e,f,g);
    T1 = X[2] = W[2];   ROUND_00_15(2,g,h,a,b,c,d,e,f);
    T1 = X[3] = W[3];   ROUND_00_15(3,f,g,h,a,b,c,d,e);
    T1 = X[4] = W[4];   ROUND_00_15(4,e,f,g,h,a,b,c,d);
    T1 = X[5] = W[5];   ROUND_00_15(5,d,e,f,g,h,a,b,c);
    T1 = X[6] = W[6];   ROUND_00_15(6,c,d,e,f,g,h,a,b);
    T1 = X[7] = W[7];   ROUND_00_15(7,b,c,d,e,f,g,h,a);
    T1 = X[8] = W[8];   ROUND_00_15(8,a,b,c,d,e,f,g,h);
    T1 = X[9] = W[9];   ROUND_00_15(9,h,a,b,c,d,e,f,g);
    T1 = X[10] = W[10]; ROUND_00_15(10,g,h,a,b,c,d,e,f);
    T1 = X[11] = W[11]; ROUND_00_15(11,f,g,h,a,b,c,d,e);
    T1 = X[12] = W[12]; ROUND_00_15(12,e,f,g,h,a,b,c,d);
    T1 = X[13] = W[13]; ROUND_00_15(13,d,e,f,g,h,a,b,c);
    T1 = X[14] = W[14]; ROUND_00_15(14,c,d,e,f,g,h,a,b);
    T1 = X[15] = W[15]; ROUND_00_15(15,b,c,d,e,f,g,h,a);
#else
    T1 = X[0]  = PULL64(W[0]);  ROUND_00_15(0,a,b,c,d,e,f,g,h);
    T1 = X[1]  = PULL64(W[1]);  ROUND_00_15(1,h,a,b,c,d,e,f,g);
    T1 = X[2]  = PULL64(W[2]);  ROUND_00_15(2,g,h,a,b,c,d,e,f);
    T1 = X[3]  = PULL64(W[3]);  ROUND_00_15(3,f,g,h,a,b,c,d,e);
    T1 = X[4]  = PULL64(W[4]);  ROUND_00_15(4,e,f,g,h,a,b,c,d);
    T1 = X[5]  = PULL64(W[5]);  ROUND_00_15(5,d,e,f,g,h,a,b,c);
    T1 = X[6]  = PULL64(W[6]);  ROUND_00_15(6,c,d,e,f,g,h,a,b);
    T1 = X[7]  = PULL64(W[7]);  ROUND_00_15(7,b,c,d,e,f,g,h,a);
    T1 = X[8]  = PULL64(W[8]);  ROUND_00_15(8,a,b,c,d,e,f,g,h);
    T1 = X[9]  = PULL64(W[9]);  ROUND_00_15(9,h,a,b,c,d,e,f,g);
    T1 = X[10] = PULL64(W[10]); ROUND_00_15(10,g,h,a,b,c,d,e,f);
    T1 = X[11] = PULL64(W[11]); ROUND_00_15(11,f,g,h,a,b,c,d,e);
    T1 = X[12] = PULL64(W[12]); ROUND_00_15(12,e,f,g,h,a,b,c,d);
    T1 = X[13] = PULL64(W[13]); ROUND_00_15(13,d,e,f,g,h,a,b,c);
    T1 = X[14] = PULL64(W[14]); ROUND_00_15(14,c,d,e,f,g,h,a,b);
    T1 = X[15] = PULL64(W[15]); ROUND_00_15(15,b,c,d,e,f,g,h,a);
#endif

    for (i=16;i<80;i+=16)
    {
        ROUND_16_80(i, 0,a,b,c,d,e,f,g,h,X);
        ROUND_16_80(i, 1,h,a,b,c,d,e,f,g,X);
        ROUND_16_80(i, 2,g,h,a,b,c,d,e,f,X);
        ROUND_16_80(i, 3,f,g,h,a,b,c,d,e,X);
        ROUND_16_80(i, 4,e,f,g,h,a,b,c,d,X);
        ROUND_16_80(i, 5,d,e,f,g,h,a,b,c,X);
        ROUND_16_80(i, 6,c,d,e,f,g,h,a,b,X);
        ROUND_16_80(i, 7,b,c,d,e,f,g,h,a,X);
        ROUND_16_80(i, 8,a,b,c,d,e,f,g,h,X);
        ROUND_16_80(i, 9,h,a,b,c,d,e,f,g,X);
        ROUND_16_80(i,10,g,h,a,b,c,d,e,f,X);
        ROUND_16_80(i,11,f,g,h,a,b,c,d,e,X);
        ROUND_16_80(i,12,e,f,g,h,a,b,c,d,X);
        ROUND_16_80(i,13,d,e,f,g,h,a,b,c,X);
        ROUND_16_80(i,14,c,d,e,f,g,h,a,b,X);
        ROUND_16_80(i,15,b,c,d,e,f,g,h,a,X);
    }

    ctx->h[0] += a; ctx->h[1] += b; ctx->h[2] += c; ctx->h[3] += d;
    ctx->h[4] += e; ctx->h[5] += f; ctx->h[6] += g; ctx->h[7] += h;
}