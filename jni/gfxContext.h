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
    s32 mWidth;
    s32 mHeight;
    FBO *mFBO;
    Material *mSwapMat;
};
typedef struct SContext Context;

void createContext(s32 width, s32 height);
Context *getContext();
void resizeContext(s32 width, s32 height);
void swapContext1();
void swapContext2();

#endif // GPU_CONTEXT_H
