#pragma once
#ifndef GFX_FBO_H
#define GFX_FBO_H

//
//  gfxFBO.h
//  fpse [NEW]
//
//  Created by Mallet Laurent on 13/01/13.
//  Copyright (c) 2013 Mallet Laurent. All rights reserved.
//

#include "gfxCommon.h"



struct SFBO {
    int mFBO;
    int mWidth;
    int mHeight;
    int mTexturePtrId;
    int mDepthRenderBuffer;
};
typedef struct SFBO FBO;

/// create a FBO in 16bits or in 32bits
FBO *createFBO(s32 width, s32 height, bool hd);

/// destroy FBO
void destroyFBO(FBO *fbo);

/// use FBO
/// if null use Offscreen
void useFBO(FBO *fbo);

/// Current Used FBO
FBO *getFBOUsed();

/// Check FBO
bool checkFBO();

#endif // GFX_FBO_H
