//
//  gpuShader.c
//  Basketball2-iPhone
//
//  Created by Mallet Laurent on 19/08/12.
//  Copyright (c) 2012 IOPixel. All rights reserved.
//

#include "gfxShader.h"
#include "gpuPlugin.h"

#if defined (GL_OGLES2)

#define TAG "ELLIS"

#ifdef DEBUG
#define ELOQUENT 1
#define MALLOC_DEBUG 1
#include "rmalloc.h"
#endif

// internal functions
int compile(GLSLProgram* obj, const char* name);
int link(GLSLProgram* obj, const char* name);

static s32 sGLSLProgramUsedCounter = 0 ;
static s32 sLastUsedGLSLProgram = -1;


// MARK: GLSLProgram

GLuint loadShader(const char* filename, const char* shaderSrc, GLenum type);
char* loadFileContent(const char* filename);
int getAttribsAndUniforms(GLSLProgram* obj, const char* name);

int offsetof_GLSLVertex_Coords = 0;
int offsetof_S3DVertex_Color = 0;
int offsetof_S3DVertex_TCoords = 0;


GLSLProgram* createGLSLProgram(const char* name) {    
#ifdef DEBUG
    logInfo(TAG, "Try to load GLSL Program %s ready", name);
#endif
    GLSLProgram* obj = (GLSLProgram*)malloc(sizeof(GLSLProgram));
    obj->mVertexShader = 0;
    obj->mFragmentShader = 0;
    obj->mProgramObject = glCreateProgram();
    if(!compile(obj, name)) return NULL;
    if(!link(obj, name)) return NULL;
    getAttribsAndUniforms(obj, name);
#ifdef DEBUG
    logInfo(TAG, "GLSL Program %s ready", name);
#endif
    return obj;
}

void useGLSLProgram(GLSLProgram* obj) {
    sGLSLProgramUsedCounter++;
    if (((s32)obj->mProgramObject) != sLastUsedGLSLProgram) {
        glUseProgram(obj->mProgramObject);
    }
    GLSLPrograms* prg = getGLSLPrograms();

    if (obj->mMvpLoc != -1) {
        multiplyMatrix(prg->mMvp, prg->mMproj, prg->mMmv);
        glUniformMatrix4fv(obj->mMvpLoc, 1, GL_FALSE, prg->mMvp);
    }
    
    if (obj->mUseAlphaTestLoc != -1) {
        glUniform1i(obj->mUseAlphaTestLoc, prg->mUseAlphaTest);
    }
    if (obj->mAlphaTestTypeLoc != -1) {
        glUniform1i(obj->mAlphaTestTypeLoc, prg->mAlphaTestType);
    }
    if (obj->mAlphaTestValueLoc != -1) {
        glUniform1f(obj->mAlphaTestValueLoc, prg->mAlphaTestValue);
    }

    // set Texture 0
    u32 tex0 = prg->mTextureName;
    if ((obj->mTex0Loc != -1)) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex0);
        glUniform1i(obj->mTex0Loc ,0);
    }
}

s32 getProgramsUsed() {
    return sGLSLProgramUsedCounter;
}

void resetProgramsCounter() {
    sGLSLProgramUsedCounter = 0;
}

void enableVertexAttribArray(GLSLProgram* obj, u8* vertices) {
    if (obj->mPositionLoc != -1) {
        glVertexAttribPointer(obj->mPositionLoc, 3, GL_FLOAT, GL_FALSE, sizeof(GLSLVertex), vertices + offsetof_GLSLVertex_Coords);
        glEnableVertexAttribArray(obj->mPositionLoc);
    }
    if (obj->mColorLoc != -1) {
        glVertexAttribPointer(obj->mColorLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GLSLVertex), vertices + offsetof_S3DVertex_Color);
        glEnableVertexAttribArray(obj->mColorLoc);
    }
    if (obj->mTexCoordLoc != -1) {
        glVertexAttribPointer(obj->mTexCoordLoc, 2, GL_FLOAT, GL_FALSE, sizeof(GLSLVertex), vertices + offsetof_S3DVertex_TCoords);
        glEnableVertexAttribArray(obj->mTexCoordLoc);
    }
}


void disableVertexAttribArray(GLSLProgram* obj) {
    if (obj->mPositionLoc != -1) {
        glDisableVertexAttribArray(obj->mPositionLoc);
    }
}

// MARK: GLSLPrograms

static GLSLPrograms* sGLSLProgramsInstance = NULL;

void initGLSLPrograms() {
    GLSLVertex v;
    offsetof_GLSLVertex_Coords = (u8*)&v.x - (u8*)&v;
    offsetof_S3DVertex_Color = (u8*)&v.c.col - (u8*)&v;
    offsetof_S3DVertex_TCoords = (u8*)&v.sow - (u8*)&v;
}

void createGLSLPrograms() {
    initGLSLPrograms();
    sGLSLProgramsInstance = (GLSLPrograms*)malloc(sizeof(GLSLPrograms));
    sGLSLProgramsInstance->mMvp = createIdentityMatrix();
    sGLSLProgramsInstance->mMmv = createIdentityMatrix();
    sGLSLProgramsInstance->mMproj = createIdentityMatrix();
    
    sGLSLProgramsInstance->mDebugProgram = createGLSLProgram("glsl_debug");
    sGLSLProgramsInstance->mFlatProgram = createGLSLProgram("glsl_blank");
    sGLSLProgramsInstance->mGouraudProgram = createGLSLProgram("glsl_color");
    
    sGLSLProgramsInstance->mTProgram = createGLSLProgram("glsl_texture");
    sGLSLProgramsInstance->mTP1Program = createGLSLProgram("glsl_texture_p1");
    sGLSLProgramsInstance->mTP2Program = createGLSLProgram("glsl_texture_p2");
    
    sGLSLProgramsInstance->mTGProgram = createGLSLProgram("glsl_texturegouraud");
    sGLSLProgramsInstance->mTGP1Program = createGLSLProgram("glsl_texturegouraud_p1");
    sGLSLProgramsInstance->mTGP2Program = createGLSLProgram("glsl_texturegouraud_p2");
}

GLSLPrograms* getGLSLPrograms() {
#ifdef DEBUG
    if (sGLSLProgramsInstance==NULL) {
        logError(TAG, "Call createGLSLPrograms before!");
        return NULL;
    }
#endif
    return sGLSLProgramsInstance;
}

void useAlphaTestForPrg(GLSLPrograms* obj, bool use) {
    obj->mUseAlphaTest = use;
}

void setAlphaTestForPrg(GLSLPrograms* obj, E_ALPHA_TEST type, f32 value) {
    obj->mAlphaTestType = type;
    obj->mAlphaTestValue = value;
}

void setColorForPrg(GLSLPrograms* obj, u32 color) {
    obj->mColor = color;
}

void setTextureForPrg(GLSLPrograms* obj, u32 textureName) {
    obj->mTextureName = textureName;
}

void setModelViewForPrg(GLSLPrograms* obj, const GLfloat* mv) {
    copyMatrix(obj->mMmv, mv);
}

void setProjectionForPrg(GLSLPrograms* obj, const GLfloat* mproj) {
    copyMatrix(obj->mMproj, mproj);
}

// MARK: GLSLProgram private

int getAttribsAndUniforms(GLSLProgram* obj, const char* name) {
    // attributes
    obj->mPositionLoc = glGetAttribLocation(obj->mProgramObject, "aPosition");
    obj->mColorLoc = glGetAttribLocation(obj->mProgramObject,"aColor");
    obj->mTexCoordLoc = glGetAttribLocation(obj->mProgramObject,"aTexCoord");

    // uniforms
    obj->mMvpLoc = glGetUniformLocation(obj->mProgramObject, "uMvp");
    obj->mScreenSizeLoc = glGetUniformLocation(obj->mProgramObject, "uScreenSize");
    obj->mUseAlphaTestLoc = glGetUniformLocation(obj->mProgramObject, "uUseAlphaTest");
    obj->mAlphaTestTypeLoc = glGetUniformLocation(obj->mProgramObject, "uAlphaTestType");
    obj->mAlphaTestValueLoc = glGetUniformLocation(obj->mProgramObject, "uAlphaTestValue");

    // samplers
    obj->mTex0Loc = glGetUniformLocation(obj->mProgramObject, "tex0");
    obj->mTex1Loc = glGetUniformLocation(obj->mProgramObject, "tex1");
    obj->mTex2Loc = glGetUniformLocation(obj->mProgramObject, "tex2");
    obj->mTex3Loc = glGetUniformLocation(obj->mProgramObject, "tex3");
    return 1;
}


int link(GLSLProgram* obj, const char* name) {
    glLinkProgram(obj->mProgramObject);
    // Check the link status
    GLint linked;
    glGetProgramiv(obj->mProgramObject, GL_LINK_STATUS, &linked);
    if(!linked) {
        GLint infoLen = 0;
        glGetProgramiv(obj->mProgramObject, GL_INFO_LOG_LENGTH, &infoLen);
        if(infoLen > 1) {
            char* infoLog = (char *)malloc(sizeof(char) * infoLen + 16);
            glGetProgramInfoLog(obj->mProgramObject, infoLen, NULL, infoLog);
            logError(TAG, "Error Cannot link: %s", name);
            logError(TAG, "Error: %s", infoLog);
            free(infoLog);
        }
        return 0;
    }
    return 1;
}

int compile(GLSLProgram* obj, const char* name) {
    asprintf(&obj->mVertexFilename, "%s.vsh", name);
    asprintf(&obj->mFragmentFilename, "%s.fsh", name);
    
    // load vertex file
    char* vContent = loadFileContent(obj->mVertexFilename);
    if (!vContent) {
        logError(TAG, "Cannot read %s", obj->mVertexFilename);
        return 0;
    }
    // load fragment file
    char* fContent = loadFileContent(obj->mFragmentFilename);
    if (!fContent) {
        logError(TAG, "Cannot read %s", obj->mFragmentFilename);
        return 0;
    }
    
    // try to compile vertex shader
    GLuint vs = loadShader(obj->mVertexFilename, vContent, GL_VERTEX_SHADER);
    if (!vs) {
        return 0;
    }
    // try to compile fragment shader
    GLuint fs = loadShader(obj->mFragmentFilename, fContent, GL_FRAGMENT_SHADER);
    if (!fs) {
        glDeleteShader(vs);
        return 0;
    }
    obj->mVertexShader = vs;
    obj->mFragmentShader = fs;
    
    // attach shaders
    glAttachShader(obj->mProgramObject, vs);
    glAttachShader(obj->mProgramObject, fs);
    
    return 1;
}

char* loadFileContent(const char* filename) {
    char* res = NULL;
    char path[2048];
    snprintf(path, 2048, "%s/%s", getShaderPath(), filename);
    logInfo(TAG, "PATH: %s", path);
    FILE *f = fopen(path, "rt");
    if (f) {
        fseek(f, 0L, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0L, SEEK_SET);
        res = (char *)malloc(sz+16);
        fread(res, sz, 1, f);
        res[sz]='\0';
        fclose(f);
    }
    return res;
}

GLuint loadShader(const char* filename, const char* shaderSrc, GLenum type) {
    GLuint shader;
    GLint compiled;
    // Create the shader object
    shader = glCreateShader(type);
    if(shader == 0)
        return 0;
    // Load the shader source
    glShaderSource(shader, 1, &shaderSrc, NULL);
    // Compile the shader
    glCompileShader(shader);
    // Check the compile status
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if(!compiled) {
        GLint infoLen = 0; glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if(infoLen > 1) {
            char* infoLog = (char *)malloc(sizeof(char) * infoLen);
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            logError(TAG, "Error compiling shader: %s", filename);
            logError(TAG, "Error: %s", infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}


#endif // GL_OGLES2
