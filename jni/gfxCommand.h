#pragma once
#ifndef GFX_COMMAND_H
#define GFX_COMMAND_H

#include "gpuCommon.h"

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

enum CLAMP_TYPE {
    CLAMP_TYPE_EDGE, CLAMP_TYPE_REPEAT,
};
typedef enum CLAMP_TYPE E_CLAMP_TYPE;

enum TRIBOOL_TYPE {
    TRIBOOL_FALSE=0, TRIBOOL_TRUE, TRIBOOL_UNKNOWN,
};
typedef enum TRIBOOL_TYPE E_TRIBOOL_TYPE;


// Public API
void debugCommand(bool flag);
void changeDebuggedCommand();
bool isDebuggedCommand(E_CMD_TYPE type);

// Public API: primitives
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
void useBlending(E_TRIBOOL_TYPE flag);
void drawDebugPoint(OGLVertex *vertices, u16 *indices, s32 count);
void drawDebugLine(OGLVertex *vertices, u16 *indices, s32 count);
void drawDebugTri(OGLVertex *vertices, u16 *indices, s32 count);

// Private API: OGLES2
#if defined(GL_OGLES2)
void setClearColor(f32 r, f32 g, f32 b, f32 a);
void clearBuffers(bool colorBuffer, bool depthBuffer, bool stencilBuffer);
#endif

#endif // GFX_COMMAND_H