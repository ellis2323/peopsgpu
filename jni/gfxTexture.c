//
//  gfxTexture.c
//  fpse [NEW]
//
//  Created by Mallet Laurent on 13/01/13.
//  Copyright (c) 2013 Mallet Laurent. All rights reserved.
//

#include "gfxTexture.h"
#include "gfxFBO.h"
#include "gfxGL.h"

#define TAG "ELLIS"

#if defined(GL_OGLES2) || defined(GL_OGLES1)

#define MAX_TEXTURES_PTR 255
static TexturePtr *sTexturesPtrArray = NULL;
static s32 sLastTidUsed = 0;

s32 findFreeTexturePtr();

/// Initialize the TexturePtr array.
void initTextures() {
    sTexturesPtrArray = (TexturePtr *)malloc(sizeof(TexturePtr)*MAX_TEXTURES_PTR);
    for (s32 i=0; i<MAX_TEXTURES_PTR; ++i) {
        sTexturesPtrArray[i] = NULL;
    }
    logInfo(TAG, "Init Textures done [%d]! ", MAX_TEXTURES_PTR);
}

/// Create a texture and return the index in the TexturePtr array.
s32 createTexture(s8 filters, s8 clampTypes) {
    u32 texture;
    // search a free TexturePtr
    s32 freeId = findFreeTexturePtr();
    if (freeId==-1) return -1;
    // create a texture
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    switch (filters) {
        case 0:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            break;
        case 1:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            break;
        default:
            logError(TAG, "ERROR: filter mode unknown");
    }
    switch (clampTypes) {
        case 0:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            break;
        case 1:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            break;
        default:
            logError(TAG, "ERROR: clamp types");
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Texture struct
    sTexturesPtrArray[freeId] = (Texture *)malloc(sizeof(Texture));
    sTexturesPtrArray[freeId]->mTextureId = texture;
    sTexturesPtrArray[freeId]->mFilters = filters;
    sTexturesPtrArray[freeId]->mClampTypes = clampTypes;
    sTexturesPtrArray[freeId]->mFormat = 0;
    sTexturesPtrArray[freeId]->mWidth = 0;
    sTexturesPtrArray[freeId]->mHeight = 0;
    logInfo(TAG, "createTexture tid:%d glid:%d f:%d c:%d", freeId, texture, filters, clampTypes);
    sLastTidUsed = freeId;
    return freeId;
}

s32 convertFilterToGL(s8 filter) {
    switch (filter) {
        case 0:
            return GL_NEAREST;
        case 1:
            return GL_LINEAR;
        default:
            logError(TAG, "ERROR: GL filter type unknown %d", filter);
            return -1;
    }
}

void setTexture(s32 tId, s32 width, s32 height, s32 format, u8 *data) {
    sLastTidUsed = tId;
    Texture *tex = getTexture(tId);
    if (tex==NULL) return;
    glBindTexture(GL_TEXTURE_2D, tex->mTextureId);
    switch (format) {
        case 1:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, data);
            break;
        case 2:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            break;
        case 3:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            break;
        case 0:
        default:
            logError(TAG, "ERROR: setTexture invalid format");
            break;
    }
    tex->mFormat = format;
    glBindTexture(GL_TEXTURE_2D, 0);
    logInfo(TAG, "setTexture tid:%d [%d %d %d]", tId, width, height, format);
}

void setSubTexture(s32 tId, s32 x, s32 y, s32 width, s32 height, s32 format, u8 *data) {
    sLastTidUsed = tId;
    Texture *tex = getTexture(tId);
    if (tex==NULL) return;
    glBindTexture(GL_TEXTURE_2D, tex->mTextureId);
    tex->mFormat = format;
    switch (format) {
        case 1:
            glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, data);
            break;
        case 2:
            glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);        
            break;
        case 3:
            glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
            break;
        case 0:
        default:
            logError(TAG, "ERROR: seTexture invalid format");
            break;
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    logInfo(TAG, "setSubTexture tid:%d [%d %d %d]", tId, width, height, format);
}

void copySubTexture(s32 tId, s32 offsetX, s32 offsetY, s32 x, s32 y, s32 width, s32 height) {
    sLastTidUsed = tId;
    Texture *tex = getTexture(tId);
    if (tex==NULL) return;
    glBindTexture(GL_TEXTURE_2D, tex->mTextureId);
    glCopyTexSubImage2D( GL_TEXTURE_2D, 0, offsetX, offsetY, x, y, width, height);
}

/// Texture id
void destroyTexture(s32 tId) {
    Texture *texture = sTexturesPtrArray[tId];
    if(texture==NULL) {
        logError(TAG, "Cannot destroy texture with tid: %d", tId);
        return;
    }
    u32 textureId = texture->mTextureId;
    glDeleteTextures(1, &textureId);
    free(texture);
    sTexturesPtrArray[tId] = NULL;
}

Texture *getTexture(s32 texturePtrId) {
    Texture *res = NULL;
    if (texturePtrId>=0 && texturePtrId<MAX_TEXTURES_PTR) {
        res = sTexturesPtrArray[texturePtrId];
    }
#ifdef DEBUG
    if (res==NULL) logInfo(TAG, "Texture not found tid: %d", texturePtrId);
#endif
    return res;
}

void bindTexture(s32 tId) {
    logInfo(TAG, "bindTexture: %d", tId);
    if (tId==0) {
        sLastTidUsed = 0;
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }
    Texture *tex = getTexture(tId);
    if (tex==NULL) {
        logError(TAG, "TexturePtrId %d doesn't exist", tId);
        sLastTidUsed = 0;
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    } else if (tex->mFormat<=0) {
        sLastTidUsed = 0;
        glBindTexture(GL_TEXTURE_2D, 0);
        logError(TAG, "TexturePtrId %d is not defined", tId);
        return;
    }
    sLastTidUsed = tId;
    glBindTexture(GL_TEXTURE_2D, tex->mTextureId);
}

s32 getCurrentTid() {
    return sLastTidUsed;
}

void useTexturing(bool flag) {
    if (flag) {
        glEnable(GL_TEXTURE_2D);
    } else {
        glDisable(GL_TEXTURE_2D);
    }
}

void setFilterForCurrentTexture(s8 filter) {
    switch (filter) {
    case 0:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        break;
    break;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;
    default:
        logError(TAG, "Filter for texture not supported %d", filter);
        break;
    }
}


bool checkTexture() {
    // get current texture bound
    GLint current_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_texture);
    // Check FBO Texture is not bound texture
    FBO *fbo = getFBOUsed();
    if (fbo && checkFBO(fbo)) {
        if (getTexture(fbo->mTexturePtrId)->mTextureId==current_texture) {
            glBindTexture(GL_TEXTURE_2D, 0);
            logError(TAG, "checkTexture failed because Texture Bound is the FBO texture!!!");
            return false;
        }
    }
    for (s32 i=0; i<MAX_TEXTURES_PTR; ++i) {
        if (sTexturesPtrArray[i]!=NULL) {
            if (sTexturesPtrArray[i]->mTextureId==current_texture) {
                return true;
            }
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    logError(TAG, "checkTexture failed");
    return false;
}

bool checkTexturePtrId(s32 tid) {
    if (tid<0 || tid>=MAX_TEXTURES_PTR) return false;
    if (sTexturesPtrArray[tid]==NULL) return false;
    return true;
}

s32 findFreeTexturePtr() {
    for (s32 i=1; i<MAX_TEXTURES_PTR; ++i) {
        if (sTexturesPtrArray[i]==NULL) {
            return i;
        }
    }
    
    return -1;
}


#endif