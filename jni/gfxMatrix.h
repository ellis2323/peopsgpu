#pragma once
#ifndef GFX_MATRIX_H
#define GFX_MATRIX_H
//
//  gfxMatrix.h
//  fpse [NEW]
//
//  Created by Mallet Laurent on 27/01/13.
//  Copyright (c) 2013 Mallet Laurent. All rights reserved.
//

#include "gfxCommon.h"

// MARK: matrix (OpenGL convention is column oriented. DirectX is row oriented)

f32* createIdentityMatrix();

f32* createNullMatrix();

void identityMatrix(f32* matrix);

void nullMatrix(f32* matrix);

void projectionMatrix(f32* matrix, f32 right, f32 left, f32 top, f32 bottom, f32 zfar, f32 znear);

void multiplyMatrix(f32* matrix, const f32* a, const f32* b);

void copyMatrix(f32* dst, const f32* src);



#endif
