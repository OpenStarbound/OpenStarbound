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
#include "../include/external_calls.h"
#include "curve25519_mehdi.h"

typedef struct
{
    U_WORD X[K_WORDS];   /* x = X/Z */
    U_WORD Z[K_WORDS];   /*  */
} XZ_POINT;

extern const U_WORD _w_P[K_WORDS];
extern EDP_BLINDING_CTX edp_custom_blinding;

/* x coordinate of base point */
const U8 ecp_BasePoint[32] = { 
    9,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0 };
/* Y = X + X */
void ecp_MontDouble(XZ_POINT *Y, const XZ_POINT *X)
{
    U_WORD A[K_WORDS], B[K_WORDS];
    /*  x2 = (x+z)^2 * (x-z)^2 */
    /*  z2 = ((x+z)^2 - (x-z)^2)*((x+z)^2 + ((A-2)/4)((x+z)^2 - (x-z)^2)) */
    ecp_AddReduce(A, X->X, X->Z);       /* A = (x+z) */
    ecp_SubReduce(B, X->X, X->Z);       /* B = (x-z) */
    ecp_SqrReduce(A, A);                /* A = (x+z)^2 */
    ecp_SqrReduce(B, B);                /* B = (x-z)^2 */
    ecp_MulReduce(Y->X, A, B);          /* x2 = (x+z)^2 * (x-z)^2 */
    ecp_SubReduce(B, A, B);             /* B = (x+z)^2 - (x-z)^2 */
    /* (486662-2)/4 = 121665 */
    ecp_WordMulAddReduce(A, A, 121665, B);
    ecp_MulReduce(Y->Z, A, B);          /* z2 = (B)*((x+z)^2 + ((A-2)/4)(B)) */
}

/* return P = P + Q, Q = 2Q */
void ecp_Mont(XZ_POINT *P, XZ_POINT *Q, IN const U_WORD *Base)
{
    U_WORD A[K_WORDS], B[K_WORDS], C[K_WORDS], D[K_WORDS], E[K_WORDS];
    /* x3 = ((x1-z1)(x2+z2) + (x1+z1)(x2-z2))^2*zb     zb=1 */
    /* z3 = ((x1-z1)(x2+z2) - (x1+z1)(x2-z2))^2*xb     xb=Base */
    ecp_SubReduce(A, P->X, P->Z);   /* A = x1-z1 */
    ecp_AddReduce(B, P->X, P->Z);   /* B = x1+z1 */
    ecp_SubReduce(C, Q->X, Q->Z);   /* C = x2-z2 */
    ecp_AddReduce(D, Q->X, Q->Z);   /* D = x2+z2 */
    ecp_MulReduce(A, A, D);         /* A = (x1-z1)(x2+z2) */
    ecp_MulReduce(B, B, C);         /* B = (x1+z1)(x2-z2) */
    ecp_AddReduce(E, A, B);         /* E = (x1-z1)(x2+z2) + (x1+z1)(x2-z2) */
    ecp_SubReduce(B, A, B);         /* B = (x1-z1)(x2+z2) - (x1+z1)(x2-z2) */
    ecp_SqrReduce(P->X, E);         /* x3 = ((x1-z1)(x2+z2) + (x1+z1)(x2-z2))^2 */
    ecp_SqrReduce(A, B);            /* A = ((x1-z1)(x2+z2) - (x1+z1)(x2-z2))^2 */
    ecp_MulReduce(P->Z, A, Base);   /* z3 = ((x1-z1)(x2+z2) - (x1+z1)(x2-z2))^2*Base */

    /* x4 = (x2+z2)^2 * (x2-z2)^2 */
    /* z4 = ((x2+z2)^2 - (x2-z2)^2)*((x2+z2)^2 + 121665((x2+z2)^2 - (x2-z2)^2)) */
    /* C = (x2-z2) */
    /* D = (x2+z2) */
    ecp_SqrReduce(A, D);            /* A = (x2+z2)^2 */
    ecp_SqrReduce(B, C);            /* B = (x2-z2)^2 */
    ecp_MulReduce(Q->X, A, B);      /* x4 = (x2+z2)^2 * (x2-z2)^2 */
    ecp_SubReduce(B, A, B);         /* B = (x2+z2)^2 - (x2-z2)^2 */
    ecp_WordMulAddReduce(A, A, 121665, B);
    ecp_MulReduce(Q->Z, A, B);      /* z4 = B*((x2+z2)^2 + 121665*B) */
}

/* Constant-time measure: */
/* Use different set of parameters for bit=0 or bit=1 with no conditional jump */
/* */
#define ECP_MONT(n) j = (k >> n) & 1; ecp_Mont(PP[j], QP[j], X)

/* -------------------------------------------------------------------------- */
/* Return point Q = k*P */
/* K in a little-endian byte array */
void ecp_PointMultiply(
    OUT U8 *PublicKey, 
    IN const U8 *BasePoint, 
    IN const U8 *SecretKey, 
    IN int len)
{
    int i, j, k;
    U_WORD X[K_WORDS];
    XZ_POINT P, Q, *PP[2], *QP[2];

    ecp_BytesToWords(X, BasePoint);

    /* 1: P = (2k+1)G, Q = (2k+2)G */
    /* 0: Q = (2k+1)G, P = (2k)G */

    /* Find first non-zero bit */
    while (len-- > 0)
    {
        k = SecretKey[len];
        for (i = 0; i < 8; i++, k <<= 1)
        {
            /* P = kG, Q = (k+1)G */
            if (k & 0x80)
            {
                /* We have first non-zero bit 
                // This is always bit 254 for keys created according to the spec.
                // Start with randomized base point 
                */

                ecp_Add(P.Z, X, edp_custom_blinding.zr);    /* P.Z = random */
                ecp_MulReduce(P.X, X, P.Z);
                ecp_MontDouble(&Q, &P);

                PP[1] = &P; PP[0] = &Q;
                QP[1] = &Q; QP[0] = &P;

                /* Everything we reference in the below loop are on the stack
                // and already touched (cached) 
                */

                while (++i < 8) { k <<= 1; ECP_MONT(7); }
                while (len > 0)
                {
                    k = SecretKey[--len];
                    ECP_MONT(7);
                    ECP_MONT(6);
                    ECP_MONT(5);
                    ECP_MONT(4);
                    ECP_MONT(3);
                    ECP_MONT(2);
                    ECP_MONT(1);
                    ECP_MONT(0);
                }

                ecp_Inverse(Q.Z, P.Z);
                ecp_MulMod(X, P.X, Q.Z);
                ecp_WordsToBytes(PublicKey, X);
                return;
            }
        }
    }
    /* K is 0 */
    mem_fill(PublicKey, 0, 32);
}

/* -- DH key exchange interfaces ----------------------------------------- */

/* Return R = a*P where P is curve25519 base point */
void x25519_BasePointMultiply(OUT U8 *r, IN const U8 *sk)
{
    Ext_POINT S;
    U_WORD t[K_WORDS];

    ecp_BytesToWords(t, sk);
    edp_BasePointMult(&S, t, edp_custom_blinding.zr);

    /* Convert ed25519 point to x25519 point */
    
    /* u = (1 + y)/(1 - y) = (Z + Y)/(Z - Y) */

    ecp_AddReduce(S.t, S.z, S.y);
    ecp_SubReduce(S.z, S.z, S.y);
    ecp_Inverse(S.z, S.z);
    ecp_MulMod(S.t, S.t, S.z);
    ecp_WordsToBytes(r, S.t);
}

/* Return public key associated with sk */
void curve25519_dh_CalculatePublicKey_fast(
    unsigned char *pk,          /* [32-bytes] OUT: Public key */
    unsigned char *sk)          /* [32-bytes] IN/OUT: Your secret key */
{
    ecp_TrimSecretKey(sk);
    /* Use faster method */
    x25519_BasePointMultiply(pk, sk);
}

/* Return public key associated with sk */
void curve25519_dh_CalculatePublicKey(
    unsigned char *pk,          /* [32-bytes] OUT: Public key */
    unsigned char *sk)          /* [32-bytes] IN/OUT: Your secret key */
{
    ecp_TrimSecretKey(sk);
    ecp_PointMultiply(pk, ecp_BasePoint, sk, 32);
}

/* Create a shared secret */
void curve25519_dh_CreateSharedKey(
    unsigned char *shared,      /* [32-bytes] OUT: Created shared key */
    const unsigned char *pk,    /* [32-bytes] IN: Other side's public key */
    unsigned char *sk)          /* [32-bytes] IN/OUT: Your secret key */
{
    ecp_TrimSecretKey(sk);
    ecp_PointMultiply(shared, pk, sk, 32);
}
