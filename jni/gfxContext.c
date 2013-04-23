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
#include <math.h>

#define TAG "ELLIS"

Context *sContext = NULL;

Context *getContext() {
#ifdef DEBUG
    if (sContext==NULL) {
        logInfo(TAG, "No context created");
        return NULL;
    }
#endif
    return sContext;
}

void createContext(s32 width, s32 height) {
    sContext = (Context*)malloc(sizeof(Context));
    sContext->mSwapMat = createMaterial();
    sContext->mSwapMat->mType = 1;
    sContext->mWidth = width;
    sContext->mHeight = height;
    sContext->mFBO = createFBO(width, height, false);
    sContext->mSwapMat->mTexturePtrId = sContext->mFBO->mTexturePtrId;
    useFBO(sContext->mFBO);
    logInfo(TAG, "Create Context -- %d %d", width, height);
}

void resizeContext(s32 width, s32 height) {
    logInfo(TAG, "Resize Context %d %d", width, height);
#ifdef DEBUG
    if (sContext==NULL) {
        logInfo(TAG, "No context created");
        return;
    }
#endif
    destroyFBO(sContext->mFBO);
    createFBO(width, height, false);
    sContext->mWidth = width;
    sContext->mHeight = height;
    useFBO(sContext->mFBO);
    //useFBO(NULL);
}

f32 color = 0;
static float mv[16];
static float proj[16];
static OGLVertex v[4];
static u16 indices[6];
void swapContext1() {
#ifdef DEBUG
    if (sContext==NULL) {
        logInfo(TAG, "No context created");
        return;
    }
#endif
    if (sContext->mFBO) {
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
        

        getModelViewMatrix(mv);
        getProjectionMatrix(proj);
        
        useFBO(NULL);

        
        f32 modelView[16];
        identityMatrix(modelView);
        setModelViewMatrix(modelView);
        // init projection with psx resolution
        f32 projection[16];
        projectionMatrix(projection, -1, 1, 1, -1, -1, 1);
        setProjectionMatrix(projection);
        
        color += .01;
        color = fmod(color, 1.);
        glClearColor(color,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT);
 
        drawTriangles(sContext->mSwapMat, v, indices, 2);
    }
}

void swapContext2() {
        // restore
        useFBO(sContext->mFBO);
        glClearColor(0,0,0,0);
        glClear(GL_COLOR_BUFFER_BIT);
 
        setProjectionMatrix(proj);
        setModelViewMatrix(mv);
}


