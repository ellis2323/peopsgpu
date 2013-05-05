/**********************************************************************

   FPSE Plugin: OpenGL Plugin for FPSE
   This file is copyright (c) 2012-2013 Laurent Mallet.
   License: GPL v3.  See License.txt.

   Created by Mallet Laurent on 27/01/13..
   Copyright (c) 2013 Mallet Laurent. All rights reserved.
   
   Original Plugin written by Pete Bernert

*******************************************************************/

#include "gfxMatrix.h"

f32 *createMatrix() {
    f32 *res = (f32*)malloc(sizeof(f32)*16);
    return res;
}

f32* createIdentityMatrix() {
    f32 *res = (f32*)malloc(sizeof(f32)*16);
    identityMatrix(res);
    return res;
}

f32* createNullMatrix() {
    f32 *res = (f32*)malloc(sizeof(f32)*16);
    nullMatrix(res);
    return res;
}

void multiplyMatrix(f32* matrix, const f32* a, const f32* b) {
    matrix[0]  = a[0] * b[0]  + a[4] * b[1]  + a[8] * b[2]   + a[12] * b[3];
    matrix[1]  = a[1] * b[0]  + a[5] * b[1]  + a[9] * b[2]   + a[13] * b[3];
    matrix[2]  = a[2] * b[0]  + a[6] * b[1]  + a[10] * b[2]  + a[14] * b[3];
    matrix[3]  = a[3] * b[0]  + a[7] * b[1]  + a[11] * b[2]  + a[15] * b[3];

    matrix[4]  = a[0] * b[4]  + a[4] * b[5]  + a[8] * b[6]   + a[12] * b[7];
    matrix[5]  = a[1] * b[4]  + a[5] * b[5]  + a[9] * b[6]   + a[13] * b[7];
    matrix[6]  = a[2] * b[4]  + a[6] * b[5]  + a[10] * b[6]  + a[14] * b[7];
    matrix[7]  = a[3] * b[4]  + a[7] * b[5]  + a[11] * b[6]  + a[15] * b[7];

    matrix[8]  = a[0] * b[8]  + a[4] * b[9]  + a[8] * b[10]  + a[12] * b[11];
    matrix[9]  = a[1] * b[8]  + a[5] * b[9]  + a[9] * b[10]  + a[13] * b[11];
    matrix[10] = a[2] * b[8]  + a[6] * b[9]  + a[10] * b[10] + a[14] * b[11];
    matrix[11] = a[3] * b[8]  + a[7] * b[9]  + a[11] * b[10] + a[15] * b[11];

    matrix[12] = a[0] * b[12] + a[4] * b[13] + a[8] * b[14]  + a[12] * b[15];
    matrix[13] = a[1] * b[12] + a[5] * b[13] + a[9] * b[14]  + a[13] * b[15];
    matrix[14] = a[2] * b[12] + a[6] * b[13] + a[10] * b[14] + a[14] * b[15];
    matrix[15] = a[3] * b[12] + a[7] * b[13] + a[11] * b[14] + a[15] * b[15];
}

void identityMatrix(f32* matrix) {
    for (s32 i=0; i<16; ++i) {
        matrix[i] = 0;
    }
    matrix[0] = 1.0f;
    matrix[5] = 1.0f;
    matrix[10] = 1.0f;
    matrix[15] = 1.0f;
}

void nullMatrix(f32* matrix) {
    for (s32 i=0; i<16; ++i) {
        matrix[i] = 0;
    }
}

void projectionMatrix(f32* matrix, f32 left, f32 right, f32 bottom, f32 top, f32 znear, f32 zfar) {
    matrix[0] = 2.0f / (right-left);
    matrix[1] = 0;
    matrix[2] = 0;
    matrix[3] = 0;
    
    matrix[4] = 0;
    matrix[5] = 2.0f / (top-bottom);
    matrix[6] = 0;
    matrix[7] = 0;
    
    matrix[8] = 0;
    matrix[9] = 0;
    matrix[10] = -2.0f / (zfar - znear);
    matrix[11] = 0;
    
    matrix[12] = - (right+left) / (right-left);
    matrix[13] = - (top+bottom) / (top-bottom);
    matrix[14] = - (zfar+znear) / (zfar-znear);
    matrix[15] = 1.0f;
}

void copyMatrix(f32* dst, const f32* src) {
    int i;
    for (i=0; i<16; ++i) {
        dst[i] = src[i];
    }
}
