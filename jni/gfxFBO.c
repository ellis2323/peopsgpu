//
//  gfxFBO.c
//  fpse [NEW]
//
//  Created by Mallet Laurent on 13/01/13.
//  Copyright (c) 2013 Mallet Laurent. All rights reserved.
//

#include "gfxFBO.h"
#include "gfxTexture.h"
#include "gfxContext.h"
#include "gfxGL.h"

#define TAG "ELLIS"


// !!!: OGLES1
#if defined(GL_OGLES1)

static FBO *sFBOUsed = NULL;

FBO *createFBO(s32 width, s32 height, bool hd) {
    // save current FBO
    FBO *oldFBO = getFBOUsed();
    
    GLuint framebuffer;
    GLuint depthRenderbuffer;
    
    width = width;
    height = height;

    s32 texturePtrId = createTexture(0, 0);
    setTexture(texturePtrId, width, height, 3, NULL);
    Texture *tex = getTexture(texturePtrId);
    
    // create depth renderbuffer
    glGenRenderbuffersOES(1, &depthRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, depthRenderbuffer);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, width, height);
    
    // bind framebuffer & attach texture
    glGenFramebuffersOES(1, &framebuffer);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, framebuffer);
    glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_TEXTURE_2D, tex->mTextureId, 0);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, depthRenderbuffer);
    
    // check binding
    if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
        logError(TAG, "CRITICAL ERROR: FBO no complete");
        return 0;
    }

    // save info into structure
    FBO *fbo = (FBO *)malloc(sizeof(FBO));
    fbo->mFBO = framebuffer;
    fbo->mWidth = width;
    fbo->mHeight = height;
    fbo->mTexturePtrId = texturePtrId;
    fbo->mDepthRenderBuffer = depthRenderbuffer;
    
    // restore FBO
    useFBO(oldFBO);
    return fbo;
}

void destroyFBO(FBO *fbo) {
    if (fbo==NULL) {
        logError(TAG, "Cannot destroy Default FBO");
        return;
    }
    GLuint drbId = fbo->mDepthRenderBuffer;
    GLuint fboId = fbo->mFBO;
    // switch to default FBO
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, 0);
    // delete texture
    destroyTexture(fbo->mTexturePtrId);
    // delete depth buffer
    glDeleteRenderbuffersOES(1, &drbId);
    // delete framebuffer
    glDeleteFramebuffersOES(1, &fboId);
}

void useFBO(FBO *fbo) {
    sFBOUsed = fbo;
    if(fbo) {
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, fbo->mFBO);
        glViewport(0,0,fbo->mWidth, fbo->mHeight);
    } else {
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, 0);
        Context  *ctx = getContext();
        glViewport(0, 0, ctx->mScreenWidth, ctx->mScreenHeight);
    }
}

FBO *getFBOUsed() {
    return sFBOUsed;
}

bool checkFBO(FBO *fbo) {
    if (fbo==NULL) return true;
    if (checkTexturePtrId(fbo->mTexturePtrId)) {
        logError(TAG, "checkFBO failed. TextureId is invalid!!!");
        return false;
    }
    return true;
}

#elif defined(GL_OGLES2)

static FBO *sFBOUsed = NULL;

FBO *createFBO(s32 width, s32 height, bool hd) {
    // save current FBO
    FBO *oldFBO = getFBOUsed();
    
    GLuint framebuffer;
    GLuint depthRenderbuffer;
    
    width = width;
    height = height;

    s32 texturePtrId = createTexture(0, 0);
    setTexture(texturePtrId, width, height, 3, NULL);
    Texture *tex = getTexture(texturePtrId);
    
    // create depth renderbuffer
    glGenRenderbuffers(1, &depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
    
    // bind framebuffer & attach texture
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->mTextureId, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        logError(TAG, "CRITICAL ERROR: FBO no complete");
        return 0;
    }

    FBO *fbo = (FBO *)malloc(sizeof(FBO));
    fbo->mFBO = framebuffer;
    fbo->mWidth = width;
    fbo->mHeight = height;
    fbo->mTexturePtrId = texturePtrId;
    fbo->mDepthRenderBuffer = depthRenderbuffer;
    
    // restore FBO
    useFBO(oldFBO);
    return fbo;
}

void destroyFBO(FBO *fbo) {
    if (fbo==NULL) {
        logError(TAG, "Cannot destroy Default FBO");
        return;
    }
    GLuint drbId = fbo->mDepthRenderBuffer;
    GLuint fboId = fbo->mFBO;
    // switch to default FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // delete texture
    destroyTexture(fbo->mTexturePtrId);
    // delete depth buffer
    glDeleteRenderbuffers(1, &drbId);
    // delete framebuffer
    glDeleteFramebuffers(1, &fboId);
}

void useFBO(FBO *fbo) {
    sFBOUsed = fbo;
    if(fbo) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo->mFBO);
        glViewport(0,0,fbo->mWidth, fbo->mHeight);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        Context  *ctx = getContext();
        glViewport(0, 0, ctx->mWidth, ctx->mHeight);
    }
}

FBO *getFBOUsed() {
    return sFBOUsed;
}

bool checkFBO(FBO *fbo) {
    if (fbo==NULL) return true;
    if (checkTexturePtrId(fbo->mTexturePtrId)) {
        logError(TAG, "checkFBO failed. TextureId is invalid!!!");
        return false;
    }
    return true;
}


#else 

#pragma error "NOT POSSIBLE"

#endif
