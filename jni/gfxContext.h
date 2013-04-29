#pragma once
#ifndef GPU_CONTEXT_H
#define GPU_CONTEXT_H

//  gpuContext.h
//  fpse [NEW]
//
//  Created by Mallet Laurent on 06/01/13.
//  Copyright (c) 2013 Mallet Laurent. All rights reserved.
//

#include "gfxCommon.h"
#include "gfxFBO.h"
#include "gfxCommand.h"

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
};
typedef struct SContext Context;

//! create a context with a screen size & FBO size
void createContext(s32 width, s32 height, s32 widthFBO, s32 heightFBO);
//! destroy
void destroyContext();
//! get context
Context *getContext();
//! resize screen viewport
void resizeViewPortContext(s32 x, s32 y, s32 width, s32 height);
//! resize FBO
void resizeFBOContext(s32 width, s32 height);



//! select Screen & copy FBO -> Screen
void swapContext1();
//! select FBO
void swapContext2();

#endif // GPU_CONTEXT_H
