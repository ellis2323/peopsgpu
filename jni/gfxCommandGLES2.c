#include "gfxCommand.h"

#if defined(GL_OGLES2)

#include "gpuShader.h"
#include "gfxGL.h"

extern E_TRIBOOL_TYPE sBlendingState;
extern u32 sCurrentBlendingMode;


void initGL() {

}


// MARK: New generation code

void drawPointOpaque(OGLVertex *vertices, u16 *indices, s32 count) {
#ifdef DEBUG
    if (isDebuggedCommand(CMD_TYPE_POINT_OPAQUE)) { drawDebugPoint(vertices, indices, count); return; }
#endif
    GLSLPrograms* programs = getGLSLPrograms();
    GLSLProgram* prg =  programs->mGouraudProgram;
    enableVertexAttribArray(prg, (u8 *)vertices);
    useGLSLProgram(prg);
    useBlending(false);
    glDrawElements(GL_POINTS, count, GL_UNSIGNED_SHORT, indices);
}

void drawPointTrans(OGLVertex *vertices, u16 *indices, s32 count) {
#ifdef DEBUG
    if (isDebuggedCommand(CMD_TYPE_POINT_TRANS)) { drawDebugPoint(vertices, indices, count); return; }
#endif
    GLSLPrograms* programs = getGLSLPrograms();
    GLSLProgram* prg =  programs->mGouraudProgram;
    enableVertexAttribArray(prg, (u8 *)vertices);
    useGLSLProgram(prg);
    useBlending(true);
    glDrawElements(GL_POINTS, count, GL_UNSIGNED_SHORT, indices);
}

void drawLineOpaque(OGLVertex *vertices, u16 *indices, s32 count) {
#ifdef DEBUG
    if (isDebuggedCommand(CMD_TYPE_LINE_OPAQUE)) { drawDebugLine(vertices, indices, count); return; }
#endif
    GLSLPrograms* programs = getGLSLPrograms();
    GLSLProgram* prg =  programs->mGouraudProgram;
    enableVertexAttribArray(prg, (u8 *)vertices);
    useGLSLProgram(prg);
    useBlending(false);
    glDrawElements(GL_LINES, count, GL_UNSIGNED_SHORT, indices);
}

void drawLineTrans(OGLVertex *vertices, u16 *indices, s32 count) {
#ifdef DEBUG
    if (isDebuggedCommand(CMD_TYPE_LINE_TRANS)) { drawDebugLine(vertices, indices, count); return; }
#endif
    GLSLPrograms* programs = getGLSLPrograms();
    GLSLProgram* prg =  programs->mGouraudProgram;
    enableVertexAttribArray(prg, (u8 *)vertices);
    useGLSLProgram(prg);
    useBlending(true);
    glDrawElements(GL_LINES, count, GL_UNSIGNED_SHORT, indices);
}

// Poly 3

void drawTriOpaque(OGLVertex *vertices, u16 *indices, s32 count) {
#ifdef DEBUG
    if (isDebuggedCommand(CMD_TYPE_TRI_OPAQUE)) { drawDebugTri(vertices, indices, count); return; }
#endif
    GLSLPrograms* programs = getGLSLPrograms();
    GLSLProgram* prg =  programs->mGouraudProgram;
    enableVertexAttribArray(prg, (u8 *)vertices);
    useGLSLProgram(prg);
    useBlending(false);
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indices);
}

void drawTriTrans(OGLVertex *vertices, u16 *indices, s32 count) {
#ifdef DEBUG
    if (isDebuggedCommand(CMD_TYPE_TRI_TRANS)) { drawDebugTri(vertices, indices, count); return; }
#endif
    GLSLPrograms* programs = getGLSLPrograms();
    GLSLProgram* prg =  programs->mGouraudProgram;
    enableVertexAttribArray(prg, (u8 *)vertices);
    useGLSLProgram(prg);
    useBlending(true);
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indices);
}

// Poly Tex

void drawTexTriOpaque(OGLVertex *vertices, u16 *indices, s32 count) {
#ifdef DEBUG
    if (isDebuggedCommand(CMD_TYPE_TEXTRI_OPAQUE)) { drawDebugTri(vertices, indices, count); return; }
#endif
    GLSLPrograms* programs = getGLSLPrograms();
    GLSLProgram* prg =  programs->mTProgram;
    enableVertexAttribArray(prg, (u8 *)vertices);
    useGLSLProgram(prg);
    useBlending(false);
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indices);
}

void drawTexTriTrans(OGLVertex *vertices, u16 *indices, s32 count) {
#ifdef DEBUG
    if (isDebuggedCommand(CMD_TYPE_TEXTRI_TRANS)) { drawDebugTri(vertices, indices, count); return; }
#endif
    GLSLPrograms* programs = getGLSLPrograms();
    GLSLProgram* prg =  programs->mTP1Program;
    enableVertexAttribArray(prg, (u8 *)vertices);
    useGLSLProgram(prg);
    useBlending(false);
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indices);

    prg =  programs->mTP2Program;
    useGLSLProgram(prg);
    useBlending(true);
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indices);
}

void drawGouTexTriOpaque(OGLVertex *vertices, u16 *indices, s32 count) {
#ifdef DEBUG
    if (isDebuggedCommand(CMD_TYPE_GOUTEXTRI_OPAQUE)) { drawDebugTri(vertices, indices, count); return; }
#endif
    GLSLPrograms* programs = getGLSLPrograms();
    GLSLProgram* prg =  programs->mTGProgram;
    enableVertexAttribArray(prg, (u8 *)vertices);
    useGLSLProgram(prg);
    useBlending(false);
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indices);
}

void drawGouTexTriTrans(OGLVertex *vertices, u16 *indices, s32 count) {
#ifdef DEBUG
    if (isDebuggedCommand(CMD_TYPE_GOUTEXTRI_TRANS)) { drawDebugTri(vertices, indices, count); return; }
#endif
    GLSLPrograms* programs = getGLSLPrograms();
    GLSLProgram* prg =  programs->mTGP1Program;
    enableVertexAttribArray(prg, (u8 *)vertices);
    useGLSLProgram(prg);
    useBlending(false);
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indices);

    prg =  programs->mTGP2Program;
    useGLSLProgram(prg);
    useBlending(true);
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indices);
}

void mali400() {
}

// MARK: PRIVATE METHODS

void drawDebugPoint(OGLVertex *vertices, u16 *indices, s32 count) {
    GLSLPrograms* programs = getGLSLPrograms();
    GLSLProgram* prg =  programs->mDebugProgram;
    enableVertexAttribArray(prg, (u8 *)vertices);
    useGLSLProgram(prg);
    useBlending(false);
    glDrawElements(GL_POINTS, count, GL_UNSIGNED_SHORT, indices);
}

void drawDebugLine(OGLVertex *vertices, u16 *indices, s32 count) {
    GLSLPrograms* programs = getGLSLPrograms();
    GLSLProgram* prg =  programs->mDebugProgram;
    enableVertexAttribArray(prg, (u8 *)vertices);
    useGLSLProgram(prg);
    useBlending(false);
    glDrawElements(GL_LINES, count, GL_UNSIGNED_SHORT, indices);
}

void drawDebugTri(OGLVertex *vertices, u16 *indices, s32 count) {
    GLSLPrograms* programs = getGLSLPrograms();
    GLSLProgram* prg =  programs->mDebugProgram;
    enableVertexAttribArray(prg, (u8 *)vertices);
    useGLSLProgram(prg);
    useBlending(false);
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indices);
}


static u32 sBlendingMode = 0;
void setBlendingMode(u32 mode) {
    sBlendingMode = mode & 0x3;
}

u32 getBlendingMode() {
    return sBlendingMode;
}

void useBlendingE(E_TRIBOOL_TYPE flag) {
    if (flag==TRIBOOL_TRUE) {
        // activate blending
        if (sBlendingState!=TRIBOOL_TRUE) {
            sBlendingState = TRIBOOL_TRUE;
            glEnable(GL_BLEND);
        }
        // blend mode
        u32 mode = getBlendingMode();
        //logInfo(TAG, "BLENDING MODE: %d", mode);
        if (sCurrentBlendingMode!=mode) {
            sCurrentBlendingMode = mode;
            switch (mode) {
                case 0:
                    logInfo(TAG, "Mode : %d", mode);
                    glBlendColor(0.5f, 0.5f, 0.5f, 0.5f);
                    glBlendFunc(GL_CONSTANT_COLOR, GL_CONSTANT_COLOR);
                    glBlendEquation(GL_FUNC_ADD);
                    break;
                case 1:
                    logInfo(TAG, "Mode : %d", mode);
                    glBlendFunc(GL_ONE, GL_ONE);
                    glBlendEquation(GL_FUNC_ADD);
                    break;
                case 2:
                    logInfo(TAG, "Mode : %d", mode);
                    glBlendFunc(GL_ONE, GL_ONE);
                    glBlendEquation(GL_FUNC_SUBTRACT);
                    break;
                case 3:
                    logInfo(TAG, "Mode : %d", mode);
                    glBlendColor(0.25f, 0.25f, 0.25f, 0.25f);
                    glBlendFunc(GL_ONE, GL_CONSTANT_COLOR);
                    glBlendEquation(GL_FUNC_ADD);
                    break;
                default:
                    logError(TAG, "Blending Mode unknown");
                    break;
            }
        }
    } else if (flag==TRIBOOL_FALSE) {
        if (sBlendingState!=TRIBOOL_FALSE) {
            sBlendingState = TRIBOOL_FALSE;
            glDisable(GL_BLEND);
        }
    } else {
        logError(TAG, "useBlending Not allowed with this value");
    }
}




#endif // GL_OGLES2
