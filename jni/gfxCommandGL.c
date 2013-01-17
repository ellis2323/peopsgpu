#include "gfxCommand.h"
#include "gfxTexture.h"
#include "gfxGL.h"

#define TAG "ELLIS"

#if defined(GL_OGLES2) || defined(GL_OGLES1)

u32 sMatUID = 1;

u32 sCurrentBlendingMode = 7;
E_TRIBOOL_TYPE sBlendingState = TRIBOOL_UNKNOWN;
bool sDebugCmdFlag = false;
E_CMD_TYPE sDebugCmdType = CMD_TYPE_NOOP;

Box sViewportBox;
void setViewport(s32 x, s32 y, s32 width, s32 height) {
#ifdef DEBUG
    if (x<=0 || y<=0 || width<=0 || height<=0) logError(TAG, "Invalid viewport [%d %d %d %d]", x, y, width, height);
#endif
    sViewportBox.mX = x;
    sViewportBox.mY = y;
    sViewportBox.mWidth = width;
    sViewportBox.mHeight = height;
    glViewport(x, y, width, height);
}

void checkScissor() {
    f32 sB[4];
    s32 vp[4];
    GLboolean b;
    s32 fbo;
    
    
    glGetFloatv(GL_SCISSOR_BOX, sB);
    glGetBooleanv(GL_SCISSOR_TEST, &b);
    logInfo(TAG, "Scissor: %d", b);
    if (b) {
        logInfo(TAG, "Scissor box: %f %f %f %f", sB[0], sB[1], sB[2], sB[3]);
    }
    glGetIntegerv(GL_VIEWPORT,vp);
    logInfo(TAG, "View port: %d %d %d %d", vp[0], vp[1], vp[2], vp[3]);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_OES, &fbo);
    logInfo(TAG, "FBO: %d", fbo);
    glGetBooleanv(GL_BLEND, &b);
    logInfo(TAG, "Blend:%d", b);
    glGetBooleanv(GL_DITHER, &b);
    logInfo(TAG, "Dither:%d", b);
}

void useScissor(bool flag) {
    if (flag) {
        glEnable(GL_SCISSOR_TEST);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
}

void setScissor(s32 x, s32 y, s32 width, s32 height) {
    if (x<0 || y<0 || width <0 || height<0) logError(TAG, "Invalid Scissor box [%d %d %d %d]",x, y, width, height);
    glScissor(x, y, width, height);
}

f32 sMatrixProjection[16];
void setProjectionMatrix(f32 *matrix) {
    memcpy(sMatrixProjection, matrix, 16*sizeof(f32));
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(sMatrixProjection);
}

void setProjectionOrtho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    sMatrixProjection[0] = 2.0f / (right-left);
    sMatrixProjection[1] = 0;
    sMatrixProjection[2] = 0;
    sMatrixProjection[3] = 0;
    
    sMatrixProjection[4] = 0;
    sMatrixProjection[5] = 2.0f / (top-bottom);
    sMatrixProjection[6] = 0;
    sMatrixProjection[7] = 0;
    
    sMatrixProjection[8] = 0;
    sMatrixProjection[9] = 0;
    sMatrixProjection[10] = -2.0f / (far - near);
    sMatrixProjection[11] = 0;
    
    sMatrixProjection[12] = - (right+left) / (right-left);
    sMatrixProjection[13] = - (top+bottom) / (top-bottom);
    sMatrixProjection[14] = - (far+near) / (far-near);
    sMatrixProjection[15] = 1.0f;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthof(left, right, bottom, top, near, far);
}

Material *createMaterial() {
    Material *mat = (Material *)malloc(sizeof(Material));
    mat->mUid = sMatUID;
    sMatUID ++;
    mat->mVersion = 1;
    mat->mType = 0;
    mat->mDepthMode = 0;
    mat->mTransMode = 0;
    mat->mTexturePtrId = -1;
    return mat;
}

void destroyMaterial(Material *mat) {
    free(mat);
}


void debugCommand(bool flag) {
    if (flag) { logInfo(TAG, "debug: TRUE"); } else { logInfo(TAG, "debug: FALSE"); }
    sDebugCmdFlag = flag;
}

void changeDebuggedCommand() {
    sDebugCmdType++;
    sDebugCmdType %= (s32)CMD_TYPE_COUNT;
    switch (sDebugCmdType) {
        case CMD_TYPE_NOOP:
            logInfo(TAG, "No command selected");
            break;
        case CMD_TYPE_POINT_OPAQUE:
            logInfo(TAG, "POINT OPAQUE selected");
            break;
        case CMD_TYPE_POINT_TRANS:
            logInfo(TAG, "POINT TRANS selected");
            break;
        case CMD_TYPE_LINE_OPAQUE:
            logInfo(TAG, "LINE OPAQUE selected");
            break;
        case CMD_TYPE_LINE_TRANS:
            logInfo(TAG, "LINE TRANS selected");
            break;
        case CMD_TYPE_TRI_OPAQUE:
            logInfo(TAG, "OPAQUE TRIANGLE selected");
            break;
        case CMD_TYPE_TRI_TRANS:
            logInfo(TAG, "TRANS TRIANGLE selected");
            break;
        case CMD_TYPE_TEXTRI_OPAQUE:
            logInfo(TAG, "TEXTURED OPAQUE TRIANGLE selected");
            break;
        case CMD_TYPE_TEXTRI_TRANS:
            logInfo(TAG, "TEXTURED TRANS TRIANGLE selected");
            break;
        case CMD_TYPE_GOUTEXTRI_OPAQUE:
            logInfo(TAG, "GOURAUD TEXTURED OPAQUE TRIANGLE selected");
            break;
        case CMD_TYPE_GOUTEXTRI_TRANS:
            logInfo(TAG, "GOURAUD TEXTURED TRANS TRIANGLE selected");
            break;
        default:
            logInfo(TAG, "Unkown command selected %d", sDebugCmdType);
    }
}

bool isDebuggedCommand(E_CMD_TYPE type) {
    if (!sDebugCmdFlag) return false;
    if (type==sDebugCmdType) return true;
    return false;
}



#endif // GL_OGLES2 or GL_OGLES1