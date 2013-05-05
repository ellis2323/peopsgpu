#pragma once
#ifndef GFX_TEXTURE_H
#define GFX_TEXTURE_H

/**********************************************************************

   FPSE Plugin: OpenGL Plugin for FPSE
   This file is copyright (c) 2012-2013 Laurent Mallet.
   License: GPL v3.  See License.txt.

   Created by Mallet Laurent on 8/07/12.
   Copyright (c) 2013 Mallet Laurent. All rights reserved.
   
   Original Plugin written by Pete Bernert

*******************************************************************/

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
void initTextures(void);

/// Create a texture and return the index in the TexturePtr array.
s32 createTexture(s8 filters, s8 clampTypes);

/// Define Texture (1:RGB 16bits 2:RGB 24bits 3:32bits)
void setTexture(s32 tId, s32 width, s32 height, s32 format, u8 *data);

/// Define SubTexture
void setSubTexture(s32 tId, s32 x, s32 y, s32 width, s32 height, s32 format, u8 *data);

/// Copy part of Texture into the same Texture
void copySubTexture(s32 tId, s32 offsetX, s32 offsetY, s32 x, s32 y, s32 width, s32 height);

/// Destroy a texture
void destroyTexture(s32 tId);

/// Get Texture by index in the TexturePtr array.
Texture *getTexture(s32 TexturePtrId);

/// Bind Texture
void bindTexture(s32 tId);

/// Get last current tId
s32 getCurrentTid(void);

/// Use Texturing
void useTexturing(bool flag);

/// Use or not texturing now
bool usingTexturing(void);

/// Set Filter for the current Texture
void setFilterForCurrentTexture(s8 filter);

/// Convert gfx filter to GL
s32 convertFilterToGL(s8 filter);

/// Check Texture
bool checkTexture(void);

/// Check Texture PtrId
bool checkTexturePtrId(s32 tid);

#endif // GFX_TEXTURE_H
