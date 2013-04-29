//
//  gpuContext.c
//  fpse [NEW]
//
//  Created by Mallet Laurent on 06/01/13.
//  Copyright (c) 2013 Mallet Laurent. All rights reserved.
//

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

void createContext(s32 width, s32 height, s32 widthFBO, s32 heightFBO) {
    sMv = createIdentityMatrix();
    sProj = createIdentityMatrix();
    sContext = (Context*)malloc(sizeof(Context));
    sContext->mSwapMat = createMaterial();
    sContext->mSwapMat->mType = 1;
    sContext->mX = 0;
    sContext->mY = 0;
    sContext->mScreenWidth = width;
    sContext->mScreenHeight = height;
    sContext->mFBO = createFBO(widthFBO, heightFBO, false);
    sContext->mSwapMat->mTexturePtrId = sContext->mFBO->mTexturePtrId;
    useFBO(sContext->mFBO);
    logInfo(TAG, "Create Context -- %d %d", width, height);
}

void destroyContext() {
    free(sMv);
    free(sProj);
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
        
        // Bottom Right
        v[0].x = 1;
        v[0].y = -1;
        v[0].z = 0;
        v[0].sow = 1;
        v[0].tow = 1;
        v[0].c.lcol = 0xFFFFFFFF;
        // Upper Right
        v[1].x = 1;
        v[1].y = 1;
        v[1].z = 0;
        v[1].sow = 1;
        v[1].tow = 0;
        v[1].c.lcol = 0xFFFFFFFF;
        // Upper Left
        v[2].x = -1;
        v[2].y = 1;
        v[2].z = 0;
        v[2].sow = 0;
        v[2].tow = 0;
        v[2].c.lcol = 0xFFFFFFFF;
        // Bottom Left
        v[3].x = -1;
        v[3].y = -1;
        v[3].z = 0;
        v[3].sow = 0;
        v[3].tow = 1;
        v[3].c.lcol = 0xFFFFFFFF;

        indices[0] = 0;
        indices[1] = 1;
        indices[2] = 2;
        indices[3] = 0;
        indices[4] = 2;
        indices[5] = 3;
        

        getModelViewMatrix(sMv);
        getProjectionMatrix(sProj);
        useFBO(NULL);
        
        // reset modelview to identity
        f32 modelView[16];
        identityMatrix(modelView);
        setModelViewMatrix(modelView);

        // init projection with psx resolution
        f32 projection[16];
        projectionMatrix(projection, -1, 1, 1, -1, -1, 1);
        setProjectionMatrix(projection);

        color += .01;
        color = fmod(color, 1.);
        glClearColor(0,0,0,0);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

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
        //drawTrianglesOutOfContext(sContext->mSwapMat, v, indices, 2);

    }
}

/*
 * This part restore
 */
void swapContext2() {
    if (sContext->mFBO) {        
        // restore
        useFBO(sContext->mFBO);
        // -- glClearColor(0,0,0,0);
        // -- glClear(GL_COLOR_BUFFER_BIT);
 
        setProjectionMatrix(sProj);
        setModelViewMatrix(sMv);
    
        // restore context
        loadMaterial();
    
        if (sUseTexturing) {
            useTexturing(true);
            bindTexture(sLastTextureId);
        } else {
             bindTexture(0);
        }
    }
}


