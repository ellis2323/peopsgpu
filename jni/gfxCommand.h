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
 Type:
 - 0: Flat
 
 Transparency Mode:
 - 0: Opaque
 
 Depth Mode:
 - 0: No Depth test
*/
struct SMaterial {
    s32 mTexturePtrId;
    s16 mUID;
    s16 mVersion;
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

enum BLEND_FACTOR {
    BF_ZERO=0,
    BF_ONE,
    BF_SRC_ALPHA,
    BF_ONE_MINUS_SRC_COLOR,
    BF_ONE_MINUS_SRC_ALPHA,
};
typedef enum BLEND_FACTOR E_BLEND_FACTOR;

enum DRAWTYPE {
    DRAWTYPE_FLAT = 0,
    DRAWTYPE_SMOOTH = 1,
};
typedef enum DRAWTYPE E_DRAWTYPE;
// ********************** Public API **********************

// INIT

/// Init the OpenGL Device [Common]
void initCommonGL(void);

/// Init the OpenGL Device [OGLES1 or OGLES2]
void initGL(void);

// VIEWPORT

/// set Viewport Box
void setViewport(s32 x, s32 y, s32 width, s32 height);

// SCISSOR

/// Use or Not Scissor
void useScissor(bool flag);

/// Define the Scissor Box
void setScissor(s32 x, s32 y, s32 width, s32 height);

/// Check scissor box
void checkScissor(void);

// TRANSFORMATION

/// Write the modelview matrix into a 4x4 matrix ie array of 16 float
void getModelViewMatrix(f32 *matrix);

/// Define the modelview matrix by 4x4 matrix ie array of 16 float
void setModelViewMatrix(f32 *matrix);

/// Write the projection matrix into a 4x4 matrix (Allocated)
void getProjectionMatrix(f32 *matrix);

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

/// Set blending function
void setBlendFunc(E_BLEND_FACTOR src, E_BLEND_FACTOR dst);

/// Restore Blending
void restoreUseBlending(void);

// ALPHA TEST

/// Use Alpha Test
void useAlphaTest(bool flag);

/// Set Alpha Func
void setAlphaTest(E_ALPHA_TEST test , f32 value);

/// Restore Alpha Test
void restoreAlphaTest(void);

// DEPTH TEST

/// Use Depth Test
void useDepthTest(bool flag);

/// Set Depth Test
void setDepthTest(E_DEPTH_TEST test);

/// Restore Depth Test
void restoreDepthTest(void);


// BUFFER

/// Set clear color
void setClearColor(f32 r, f32 g, f32 b, f32 a);

/// Clear Buffers
void clearBuffers(bool color, bool depth, bool stencil);

// READ PIXELS

/// Read Pixels
void readPixels(s32 x,s32 y, s32 width, s32 height, s8 format, u8 *dst);

// ERROR

/// Check error
bool hasError();

// MATERIAL

/// Create a material
Material *createMaterial(void);

/// Get Material
Material *getMaterial(s32 uid);

/// Destroy a material
void destroyMaterial(s32 uid);

//! load last Material saved
void loadMaterial(void);

//! save last Material used
void saveMaterial(Material* mat);

/// Set Draw Mode FLAT or SMOOTH
void setDrawMode(E_DRAWTYPE m);

/// Set color
void setColor(GLSLColor color);

/// Get color
GLSLColor getColor(void);

/// Restore color
void restoreColor(void);

void debugCommand(bool flag);
void changeDebuggedCommand(void);
bool isDebuggedCommand(E_CMD_TYPE type);

/// Draw a triangle
void drawTriangles(Material *mat, OGLVertex *vertices, u16 *indices, s16 count);

void PRIMdrawTexturedQuad(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3, OGLVertex* vertex4);
void PRIMdrawTexturedTri(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3);
void PRIMdrawTexGouraudTriColor(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3);
void PRIMdrawTexGouraudTriColorQuad(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3, OGLVertex* vertex4);
void PRIMdrawTri(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3);
void PRIMdrawTri2(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3, OGLVertex* vertex4);
void PRIMdrawGouraudTriColor(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3);
void PRIMdrawGouraudTri2Color(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3, OGLVertex* vertex4);
void PRIMdrawFlatLine(OGLVertex* vertex1, OGLVertex* vertex2,OGLVertex* vertex3, OGLVertex* vertex4);
void PRIMdrawGouraudLine(OGLVertex* vertex1, OGLVertex* vertex2,OGLVertex* vertex3, OGLVertex* vertex4);
void PRIMdrawQuad(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3, OGLVertex* vertex4);



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

#ifdef MALI
void mali400(void);
#endif

#endif // GFX_COMMAND_H