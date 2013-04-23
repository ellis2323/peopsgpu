#pragma once
#ifndef GFX_COMMON_H
#define GFX_COMMON_H

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


/*#if 0
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
#endif*/

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

enum TRIBOOL_TYPE {
    TRIBOOL_FALSE=0, TRIBOOL_TRUE, TRIBOOL_UNKNOWN,
};
typedef enum TRIBOOL_TYPE E_TRIBOOL_TYPE;

void initVertex(GLSLVertex *v);
void initVertices(GLSLVertex *v, s32 count);
void logVertex(const GLSLVertex *v);

#endif // GFX_COMMON_H