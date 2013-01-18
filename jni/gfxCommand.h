#pragma once
#ifndef GFX_COMMAND_H
#define GFX_COMMAND_H

#include "gfxCommon.h"

enum CMD_TYPE {
    CMD_TYPE_NOOP=0,
    CMD_TYPE_POINT_OPAQUE, CMD_TYPE_POINT_TRANS,
    CMD_TYPE_LINE_OPAQUE, CMD_TYPE_LINE_TRANS,
    CMD_TYPE_TRI_OPAQUE, CMD_TYPE_TRI_TRANS,
    CMD_TYPE_TEXTRI_OPAQUE, CMD_TYPE_TEXTRI_TRANS,
    CMD_TYPE_GOUTEXTRI_OPAQUE, CMD_TYPE_GOUTEXTRI_TRANS,
    CMD_TYPE_COUNT
};
typedef enum CMD_TYPE E_CMD_TYPE;

struct SBox {
    s32 mX;
    s32 mY;
    s32 mWidth;
    s32 mHeight;
};
typedef struct SBox Box;


/*
 Depth Mode:
 - 0: No ZBuffer
 
 Type:
 - 0: Flat
 
 Transparency Mode:
 - 0: Opaque
*/
struct SMaterial {
    u16 mUid;
    u16 mVersion;
    s32 mTexturePtrId;
    u8 mType;
    u8 mDepthMode;
    u8 mTransMode;
};
typedef struct SMaterial Material;

enum ALPHA_TEST {
    ALPHA_TEST_NEVER=0,
    ALPHA_TEST_LESS,
    ALPHA_TEST_EQUAL,
    ALPHA_TEST_LEQUAL,
    ALPHA_TEST_GREATER,
    ALPHA_TEST_NOTEQUAL,
    ALPHA_TEST_GEQUAL,
    ALPHA_TEST_ALWAYS,
};
typedef enum ALPHA_TEST E_ALPHA_TEST;

enum DEPTH_TEST {
    DEPTH_TEST_NEVER=0,
    DEPTH_TEST_LESS,
    DEPTH_TEST_EQUAL,
    DEPTH_TEST_LEQUAL,
    DEPTH_TEST_GREATER,
    DEPTH_TEST_NOTEQUAL,
    DEPTH_TEST_GEQUAL,    
    DEPTH_TEST_ALWAYS,
};
typedef enum DEPTH_TEST E_DEPTH_TEST;

// ********************** Public API ********************** 

// INIT

/// Init the OpenGL Device
void initGL();

// VIEWPORT

/// set Viewport Box
void setViewport(s32 x, s32 y, s32 width, s32 height);

// SCISSOR

/// Use or Not Scissor
void useScissor(bool flag);

/// Define the Scissor Box
void setScissor(s32 x, s32 y, s32 width, s32 height);

/// Check scissor box
void checkScissor();

// TRANSFORMATION

/// Define the projection matrix by a 4x4 matrix ie array of 16 float
void setProjectionMatrix(f32 *matrix);

/// Define an orthogonal projection matrix
void setProjectionOrtho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far);

// DITHER

/// Use or Not Dither
void useDithering(bool flag);

// BLENDING

/// Use or Not Blending
void useBlending(bool flag);

// ALPHA TEST

/// Use Alpha Test
void useAlphaTest(bool flag);

/// Set Alpha Func
void setAlphaTest(E_ALPHA_TEST test , f32 value);

// DEPTH TEST

/// Use Depth Test
void useDepthTest(bool flag);

/// Set Depth Test
void setDepthTest(E_DEPTH_TEST test);

// BUFFER

/// Set clear color
void setClearColor(f32 r, f32 g, f32 b, f32 a);

/// Clear Buffers
void clearBuffers(bool color, bool depth, bool stencil);

// MATERIAL

Material *createMaterial();
void destroyMaterial(Material *mat);

void debugCommand(bool flag);
void changeDebuggedCommand();
bool isDebuggedCommand(E_CMD_TYPE type);

// Public API: primitives

void drawTriangles(Material *mat, OGLVertex *vertices, u16 *indices, s16 count);


void drawPointOpaque(OGLVertex* vertices, u16 *indices, s32 count);
void drawPointTrans(OGLVertex* vertices, u16 *indices, s32 count);

void drawLineOpaque(OGLVertex* vertices, u16 *indices, s32 count);
void drawLineTrans(OGLVertex* vertices, u16 *indices, s32 count);

void drawTriTrans(OGLVertex* vertices, u16 *indices, s32 count);
void drawTriOpaque(OGLVertex* vertices, u16 *indices, s32 count);

void drawTexTriOpaque(OGLVertex* vertices, u16 *indices, s32 count);
void drawTexTriTrans(OGLVertex* vertices, u16 *indices, s32 count);

void drawGouTexTriOpaque(OGLVertex* vertices, u16 *indices, s32 count);
void drawGouTexTriTrans(OGLVertex* vertices, u16 *indices, s32 count);




// Private API: internal
void setDepthMode(u8 mode);
void setTransMode(u8 mode);

void drawDebugPoint(OGLVertex *vertices, u16 *indices, s32 count);
void drawDebugLine(OGLVertex *vertices, u16 *indices, s32 count);
void drawDebugTri(OGLVertex *vertices, u16 *indices, s32 count);

// Private API: OGLES2
#if defined(GL_OGLES2)
void setClearColor(f32 r, f32 g, f32 b, f32 a);
void clearBuffers(bool colorBuffer, bool depthBuffer, bool stencilBuffer);
#endif

#endif // GFX_COMMAND_H