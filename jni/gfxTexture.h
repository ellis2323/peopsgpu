#pragma once
#ifndef GFX_TEXTURE_H
#define GFX_TEXTURE_H

//
//  gfxTexture.h
//  fpse [NEW]
//
//  Created by Mallet Laurent on 13/01/13.
//  Copyright (c) 2013 Mallet Laurent. All rights reserved.
//

#include "gfxCommon.h"


enum FILTER_TYPE {
    FILTER_TYPE_NEAREST=0, FILTER_TYPE_LINEAR,
};
typedef enum FILTER_TYPE E_FILTER_TYPE;

enum CLAMP_TYPE {
    CLAMP_TYPE_EDGE=0, CLAMP_TYPE_REPEAT,
};
typedef enum CLAMP_TYPE E_CLAMP_TYPE;

///
/// Filters:
///   val    Min      Mag
///   0    NEAREST  NEAREST
///   1    LINEAR   LINEAR
///
///  CLamp Modes:
///   val   S        T
///   0    EDGE    EDGE
///   1    REPEAT  REPEAT
///
///  Format:
///   val
///   0    UNDEFINED
///   1    R5G6B5
///   2    R8G8B8
///   3    R8G8B8A8
///
struct STexture {
    s32 mTextureId;
    s32 mWidth;
    s32 mHeight;
    s8 mFormat;
    s8 mFilters;
    s8 mClampTypes;
};
typedef struct STexture Texture;

///
/// TexturePtr is a pointer of a texture.
///
typedef Texture* TexturePtr;

// ********************** Public API ********************** 

/// Initialize the TexturePtr array.
void initTextures();

/// Create a texture and return the index in the TexturePtr array.
s32 createTexture(s8 filters, s8 clampTypes);

/// Define Texture
void setTexture(s32 tId, s32 width, s32 height, s32 format, u8 *data);

/// Define SubTexture
void setSubTexture(s32 tId, s32 x, s32 y, s32 width, s32 height, u8 *data);

/// Destroy a texture
void destroyTexture(s32 tId);

/// Get Texture by index in the TexturePtr array.
Texture *getTexture(s32 TexturePtrId);

/// Bind Texture
void bindTexture(s32 tId);

/// Convert GL filter constants to gfx cst 
s8 convertGLFilter(s32 GLFilter);

/// Convert GL Clamp constants to gfx cst
s8 convertGLClamp(s32 GLClamp);

/// Check Texture
bool checkTexture();

/// Check Texture PtrId
bool checkTexturePtrId(s32 tid);

#endif // GFX_TEXTURE_H
