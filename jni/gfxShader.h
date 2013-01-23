#pragma once
#ifndef GPU_SHADER_H
#define GPU_SHADER_H
//
//  gpuShader.h
//  Basketball2-iPhone
//
//  Created by Mallet Laurent on 19/08/12.
//  Copyright (c) 2012 IOPixel. All rights reserved.
//

#include "gfxCommon.h"
#include "gfxCommand.h"

#if defined (GL_OGLES2)

#include "gfxGL.h"
#include "gpuExternals.h"

struct SGLSLProgram {

    GLuint mProgramObject;
    GLuint mVertexShader;
    GLuint mFragmentShader;

    char* mVertexFilename;
    char* mFragmentFilename;
    
    GLint mPositionLoc;
    GLint mColorLoc;
    GLint mTexCoordLoc;
    
    GLint mMvpLoc;
    GLint mScreenSizeLoc;
    GLint mUseAlphaTestLoc;
    GLint mAlphaTestTypeLoc;
    GLint mAlphaTestValueLoc;
    
    GLint mTex0Loc;
    GLint mTex1Loc;
    GLint mTex2Loc;
    GLint mTex3Loc;
    
};

typedef struct SGLSLProgram GLSLProgram;

struct SGLSLPrograms {
    GLSLProgram *mDebugProgram;
    GLSLProgram *mFlatProgram;
    GLSLProgram *mGouraudProgram;
    
    GLSLProgram *mTProgram;
    GLSLProgram *mTP1Program;
    GLSLProgram *mTP2Program;
    
    GLSLProgram *mTGProgram;
    GLSLProgram *mTGP1Program;
    GLSLProgram *mTGP2Program;
    
    u32 mColor;
    u32 mTextureName;
    
    bool mUseAlphaTest;
    E_ALPHA_TEST mAlphaTestType;
    f32 mAlphaTestValue;
    
    GLfloat *mMvp;
    GLfloat *mMmv;
    GLfloat *mMproj;
};

typedef struct SGLSLPrograms GLSLPrograms;

typedef OGLVertex GLSLVertex;

typedef OGLColor GLSLColor;

// MARK: Matrix functions

    //! create an identity matrix
    f32 *createIdentityMatrix();
    //! create a null matrix
    f32* createNullMatrix();
    //! Identity matrix
    void identityMatrix(GLfloat* matrix);
    //! create an ortho projection matrix
    void projectionMatrix(GLfloat* matrix, GLfloat right, GLfloat left, GLfloat top, GLfloat bottom, GLfloat zfar, GLfloat znear);
    //! matrix must be distinct of a and b
    void multiplyMatrix(GLfloat* matrix, const GLfloat* a, const GLfloat* b);
    //! copy
    void copyMatrix(GLfloat* dst, const GLfloat* src);

// MARK: GLSLProgram functions

    GLSLProgram* createGLSLProgram(const char* name);
    
    void useGLSLProgram(GLSLProgram* obj);

    void enableVertexAttribArray(GLSLProgram* obj, u8* vertices);
    
    void disableVertexAttribArray(GLSLProgram* obj);

// MARK: GLSLPrograms function

    void createGLSLPrograms();
    
    GLSLPrograms* getGLSLPrograms();
    
// MARK: GLSLColor functions 
    
    void setColorForPrg(GLSLPrograms* obj, u32 color);
    
    void useAlphaTestForPrg(GLSLPrograms* obj, bool use);

    void setAlphaTestForPrg(GLSLPrograms* obj, E_ALPHA_TEST type, f32 value);
    
    void setTextureForPrg(GLSLPrograms* obj, u32 textureName);
    
    void setModelViewForPrg(GLSLPrograms* obj, const GLfloat* mv);
    
    void setProjectionForPrg(GLSLPrograms* obj, const GLfloat* mproj);

// MARK: counter

    void resetProgramsCounter();
    
    s32 getProgramsUsed();

#endif // GL_OGLES2

#endif // GPU_SHADER_H

