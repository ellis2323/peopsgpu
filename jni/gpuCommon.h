#pragma once
#ifndef GPU_COMMON_H
#define GPU_COMMON_H

#include <stdlib.h>
#include <stdio.h>

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t u8;
typedef int8_t s8;
typedef char c8;

typedef uint16_t u16;
typedef int16_t s16;

typedef uint32_t u32;
typedef int32_t s32;

typedef uint64_t u64;
typedef int64_t s64;

typedef float f32;
typedef double f64;

#define DEBUG 1

#ifdef ANDROID
    
    #ifdef GL_OGLES1
    #include <GLES/gl.h>
    #include <GLES/glext.h>
    #endif
    
    #ifdef GL_OGLES2
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>
    #endif
    
#else

#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

#endif

#ifdef ANDROID
#include <android/log.h>

#ifdef DEBUG
#define logInfo(TAG, ...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define logWarn(TAG, ...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)
#define logError(TAG, ...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#else
#define logInfo(TAG, ...)
#define logWarn(TAG, ...)
#define logError(TAG, ...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#endif // DEBUG
#endif // ANDROID

#if !defined(_WINDOWS) && !defined(__NANOGL__)
#define glOrtho(x,y,z,xx,yy,zz) glOrthof(x,y,z,xx,yy,zz)
#endif

#if 0
#define glError() { \
    logInfo("ELLIS", "GL call %s:%u\n", __FILE__, __LINE__); \
    GLenum err = glGetError(); \
    while (err != GL_NO_ERROR) { \
        logInfo("ELLIS", "glError: %d caught at %s:%u\n", err, __FILE__, __LINE__); \
        err = glGetError(); \
    } \
}
#else
#define glError()
#endif

#define fpoint(x) x

// struct of Color
union UColor {
    u8 col[4];
    u32 lcol;
};

typedef union UColor OGLColor;
typedef OGLColor GLSLColor;

// struct of Vertex
struct SVertex {
    f32 x;
    f32 y;
    f32 z;

    f32 sow;
    f32 tow;

    OGLColor c;
};

typedef struct SVertex OGLVertex;
typedef OGLVertex GLSLVertex;

// struct of command
struct SCoord {
    s16 lx;
    s16 ly;
};

typedef struct SCoord PSXCoord;

void initVertex(GLSLVertex *v);
void initVertices(GLSLVertex *v, s32 count);
void logVertex(const GLSLVertex *v);

#endif // GPU_COMMON_H