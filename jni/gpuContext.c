//
//  gpuContext.c
//  fpse [NEW]
//
//  Created by Mallet Laurent on 06/01/13.
//  Copyright (c) 2013 Mallet Laurent. All rights reserved.
//

#include "gpuContext.h"
#include <math.h>

Context *sContext;

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
    sContext->mFBO = createFBO(width, height, false);
    sContext->mSwapMat = createMaterial();
    sContext->mWidth = width;
    sContext->mHeight = height;
    //useFBO(NULL);
}

void resizeContext(s32 width, s32 height) {
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
    //useFBO(NULL);
}

f32 color = 0;
void swapContext() {
#ifdef DEBUG
    if (sContext==NULL) {
        logInfo(TAG, "No context created");
        return;
    }
#endif
    if (sContext->mFBO) {
        OGLVertex v[4];
        // Bottom Right
        v[0].x = 1;
        v[0].y = -1;
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
        v[3].y = -1;
        v[3].z = 0;
        v[3].sow = 0;
        v[3].tow = 0;
        v[3].c.lcol = 0xFFFFFFFF;

        u16 indices[6];
        indices[0] = 0;
        indices[1] = 1;
        indices[2] = 2;
        indices[3] = 0;
        indices[4] = 2;
        indices[5] = 3;
        
        //glOrtho(0,368,502, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        glMatrixMode(GL_PROJECTION);  // init projection with psx resolution
        glLoadIdentity();
        glOrthof(-10, 10, 10, -10, -1, 1);
        useFBO(NULL);
        color += .01;
        color = fmod(color, 1.);
        glClearColor(color,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT);
        drawTriangles(sContext->mSwapMat, v, indices, 2);
        useFBO(sContext->mFBO);
    }
    
}
