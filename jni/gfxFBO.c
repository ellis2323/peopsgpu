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

static FBO *sFBOUsed = NULL;

// !!!: OGLES1
#if defined(GL_OGLES1)

FBO *createFBO(s32 width, s32 height, bool hd) {
    // save current FBO
    FBO *oldFBO = getFBOUsed();
    
    GLuint framebuffer;
    GLuint depthRenderbuffer;
    
    width = width / 2;
    height = height / 2;

    s32 texturePtrId = createTexture(0, 0);
    Texture *tex = getTexture(texturePtrId);
    if (tex==NULL) logError(TAG, "createFBO Invalid Texture");
    setTexture(texturePtrId, width, height, 1, NULL);
    
    // create depth renderbuffer
    glGenRenderbuffersOES(1, &depthRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, depthRenderbuffer);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, width, height);
    
    // bind framebuffer & attach texture
    glGenFramebuffersOES(1, &framebuffer);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, framebuffer);
    glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_TEXTURE_2D, tex->mTextureId, 0);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, depthRenderbuffer);
    
    if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
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
    
}

void useFBO(FBO *fbo) {
    sFBOUsed = fbo;
    if(fbo) {
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, fbo->mFBO);
        glViewport(0,0,fbo->mWidth, fbo->mHeight);
    } else {
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, 0);
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

#elif defined(GL_OGLES2)

#else 

#pragma error "NOT POSSIBLE"

#endif
