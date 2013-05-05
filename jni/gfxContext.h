#pragma once
#ifndef GPU_CONTEXT_H
#define GPU_CONTEXT_H

/*****************************************************************

   FPSE Plugin: OpenGL Plugin for FPSE
   This file is copyright (c) 2012-2013 Laurent Mallet.
   License: GPL v3.  See License.txt.

   Created by Mallet Laurent on 19/08/12.
   Copyright (c) 2013 Mallet Laurent. All rights reserved.
   
   Original Plugin written by Pete Bernert

*******************************************************************/

#include "gfxCommon.h"
#include "gfxFBO.h"
#include "gfxCommand.h"

enum ORIENTATION_MODE {
    E_ORIENTATION_UNKNOWN = 0,
    E_ORIENTATION_PORTRAIT = 1,
    E_ORIENTATION_LANDSCAPE
};

typedef enum ORIENTATION_MODE E_ORIENTATION_MODE;

struct SContext {
    // screen viewport
    s32 mX;
    s32 mY;
    s32 mScreenWidth;
    s32 mScreenHeight;
    // FBO
    FBO *mFBO;
    // Material to swap
    Material *mSwapMat;
    // Orientation: Portrait or Landscape
    E_ORIENTATION_MODE mOrientation;
};
typedef struct SContext Context;

//! create a context 
void createContext(void);
//! destroy
void destroyContext(void);
//! get context
Context *getContext(void);

//! set screen size
void setScreenSize(Context* ctx, s32 width, s32 height);
//! use fbo & define size
void useFBOInContext(Context* ctx, s32 widthFBO, s32 heightFBO);
//! orientation
void setOrientation(Context *ctx, E_ORIENTATION_MODE mode);

//! hasFBO in context
bool hasFBOInContext(void);

//! resize screen viewport
void resizeViewPortContext(s32 x, s32 y, s32 width, s32 height);
//! resize FBO
void resizeFBOContext(s32 width, s32 height);



//! select Screen & copy FBO -> Screen
void swapContext1(void);
//! select FBO
void swapContext2(void);

#endif // GPU_CONTEXT_H
