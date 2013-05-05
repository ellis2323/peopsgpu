#pragma once
#ifndef GFX_MATRIX_H
#define GFX_MATRIX_H

/**********************************************************************

   FPSE Plugin: OpenGL Plugin for FPSE
   This file is copyright (c) 2012-2013 Laurent Mallet.
   License: GPL v3.  See License.txt.

   Created by Mallet Laurent on 27/01/13.
   Copyright (c) 2013 Mallet Laurent. All rights reserved.
   
   Original Plugin written by Pete Bernert

*******************************************************************/


#include "gfxCommon.h"

// MARK: matrix (OpenGL convention is column oriented. DirectX is row oriented)
f32 *createMatrix(void);

f32* createIdentityMatrix(void);

f32* createNullMatrix(void);

void identityMatrix(f32* matrix);

void nullMatrix(f32* matrix);

void projectionMatrix(f32* matrix, f32 right, f32 left, f32 top, f32 bottom, f32 zfar, f32 znear);

void multiplyMatrix(f32* matrix, const f32* a, const f32* b);

void copyMatrix(f32* dst, const f32* src);



#endif
