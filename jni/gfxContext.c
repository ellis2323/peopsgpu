/*****************************************************************

   FPSE Plugin: OpenGL Plugin for FPSE
   This file is copyright (c) 2012-2013 Laurent Mallet.
   License: GPL v3.  See License.txt.

   Created by Mallet Laurent on 19/08/12.
   Copyright (c) 2013 Mallet Laurent. All rights reserved.
   
   Original Plugin written by Pete Bernert

*******************************************************************/

#include "gfxMatrix.h"
#include "gfxContext.h"
#include "gfxFBO.h"
#include "gfxGL.h"
#include "gfxTexture.h"
#include <math.h>

#define TAG "ELLIS"

static Context *sContext = NULL;
static float *sMv;
static float *sProj;

Context *getContext() {
#ifdef DEBUG
    if (sContext==NULL) {
        logInfo(TAG, "No context created");
        return NULL;
    }
#endif
    return sContext;
}

void createContext() {
    sMv = createIdentityMatrix();
    sProj = createIdentityMatrix();
    sContext = (Context*)malloc(sizeof(Context));
    sContext->mSwapMat = NULL;
    sContext->mOrientation = E_ORIENTATION_UNKNOWN;
    sContext->mX = 0;
    sContext->mY = 0;
    sContext->mScreenWidth = 0;
    sContext->mScreenHeight = 0;
    sContext->mFBO = NULL;
    logInfo(TAG, "Create Context");
}

void destroyContext() {
    free(sMv);
    free(sProj);
}

void setScreenSize(Context* ctx, s32 width, s32 height)  {
    logInfo(TAG, "Set screen size: %d %d", width, height);
    ctx->mScreenWidth = width;
    ctx->mScreenHeight = height;
}

void useFBOInContext(Context* ctx, s32 widthFBO, s32 heightFBO) {
    ctx->mSwapMat = createMaterial();
    sContext->mSwapMat->mType = 1;
    sContext->mFBO = createFBO(widthFBO, heightFBO, false);
    sContext->mSwapMat->mTexturePtrId = sContext->mFBO->mTexturePtrId;
    useFBO(sContext->mFBO);
    
}

bool hasFBOInContext() {
    if (sContext==NULL) return false;
    if (sContext->mFBO) return true;
    return false;
}

void setOrientation(Context *ctx, E_ORIENTATION_MODE mode) {
    ctx->mOrientation = mode;
}

void resizeViewPortContext(s32 x, s32 y, s32 width, s32 height) {
    sContext->mX = x;
    sContext->mY = y;
    sContext->mScreenWidth = width;
    sContext->mScreenHeight = height;
    logInfo(TAG, "Screen VP %d %d %d %d", x, y, width, height);
}

void resizeFBOContext(s32 width, s32 height) {
    logInfo(TAG, "Resize Context %d %d", width, height);
#ifdef DEBUG
    if (sContext==NULL) {
        logInfo(TAG, "No context created");
        return;
    }
#endif
    destroyFBO(sContext->mFBO);
    createFBO(width, height, false);
    useFBO(sContext->mFBO);
}

f32 color = 0;

static OGLVertex v[4];
static u16 indices[6];
static bool sUseTexturing = false;
static s32 sLastTextureId = -1;
void swapContext1() {
#ifdef DEBUG
    if (sContext==NULL) {
        logInfo(TAG, "No context created");
        return;
    }
#endif
    if (sContext->mFBO) {
        // save texturing state
        sUseTexturing = usingTexturing();
        if (sUseTexturing) {
            sLastTextureId = getCurrentTid();
        } else {
            sLastTextureId = -1;
        }
        
        f32 vy = 0;
        if (sContext->mOrientation==E_ORIENTATION_PORTRAIT) {
            f32 ratio = ((f32)sContext->mScreenWidth) / ((f32)sContext->mScreenHeight);
            f32 ratio2 = 3.f/4.f;
            vy = 1 - 2.f*ratio*ratio2;
        } else {
            vy = -1.f;
        }
        
        // Bottom Right
        v[0].x = 1;
        v[0].y = vy;
        v[0].z = 0;
        v[0].sow = 1;
        v[0].tow = 0;
        v[0].c.lcol = 0xFFFFFFFF;
        // Upper Right
        v[1].x = 1;
        v[1].y = 1;
        v[1].z = 0;
        v[1].sow = 1;
        v[1].tow = 1;
        v[1].c.lcol = 0xFFFFFFFF;
        // Upper Left
        v[2].x = -1;
        v[2].y = 1;
        v[2].z = 0;
        v[2].sow = 0;
        v[2].tow = 1;
        v[2].c.lcol = 0xFFFFFFFF;
        // Bottom Left
        v[3].x = -1;
        v[3].y = vy;
        v[3].z = 0;
        v[3].sow = 0;
        v[3].tow = 0;
        v[3].c.lcol = 0xFFFFFFFF;

        indices[0] = 0;
        indices[1] = 1;
        indices[2] = 2;
        indices[3] = 0;
        indices[4] = 2;
        indices[5] = 3;
        
        // saves MV & P matrix
        getModelViewMatrix(sMv);
        getProjectionMatrix(sProj);
        // swith FBO
        useFBO(NULL);
        
        // reset modelview to identity
        f32 modelView[16];
        identityMatrix(modelView);
        setModelViewMatrix(modelView);

        // init projection with psx resolution
        f32 projection[16];
        projectionMatrix(projection, -1, 1, -1, 1, -1, 1);
        setProjectionMatrix(projection);

        color += .01;
        color = fmod(color, 1.);
        glClearColor(0,0,0,0);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_ALPHA_TEST);

        glEnable(GL_TEXTURE_2D);
        Texture* tex = getTexture(sContext->mSwapMat->mTexturePtrId);
        glBindTexture(GL_TEXTURE_2D, tex->mTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisable(GL_SCISSOR_TEST);
    
        glVertexPointer(3, GL_FLOAT, sizeof(OGLVertex), &v[0].x);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(OGLVertex), &v[0].c.lcol);
        glTexCoordPointer(2, GL_FLOAT, sizeof(OGLVertex), &v[0].sow);
    
        glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_SHORT, indices);

    }
}

/*
 * This part restore
 */
void swapContext2() {
    if (sContext->mFBO) {        
        // Back to FBO
        useFBO(sContext->mFBO);
        
 
        // restore MVP matrix
        setProjectionMatrix(sProj);
        setModelViewMatrix(sMv);

        // restore
        resetDrawCmd();
        restoreClearColor();
        restoreColor();
        restoreDrawMode();
        restoreDepthTest();
        restoreAlphaTest();
        restoreUseBlending();
        

        // restore texturing
        if (sUseTexturing) {
            useTexturing(true);
            bindTexture(sLastTextureId);
        } else {
             bindTexture(0);
        }
    }
}


