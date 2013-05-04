#include "gfxCommand.h"
#include "gfxTexture.h"
#include "gfxGL.h"

#define TAG "ELLIS"

#if defined(GL_OGLES2) || defined(GL_OGLES1)

#define MAX_MATERIAL_QTY 1024

u32 sMatUID = 1;
Material *sMaterials;

u32 sCurrentBlendingMode = 7;
E_TRIBOOL_TYPE sBlendingState = TRIBOOL_UNKNOWN;
bool sDebugCmdFlag = false;
E_CMD_TYPE sDebugCmdType = CMD_TYPE_NOOP;
Box sViewportBox;

s32 findFreeMaterial(void);
GLenum convertBlendFactor(E_BLEND_FACTOR f);
bool hasError(void);

static bool sUseDepthTest = false;
static s32  sDepthTestMode = -1;
static bool sUseBlending = false;

void initCommonGL() {
    logInfo(TAG, "Init common GL: create %d materials", MAX_MATERIAL_QTY);
    sMaterials = (Material*)malloc(sizeof(Material)*MAX_MATERIAL_QTY);
    for (s32 i=0; i<MAX_MATERIAL_QTY; ++i) {
        sMaterials[i].mUID = i;
        sMaterials[i].mVersion = 0;
        sMaterials[i].mTexturePtrId = 0;
    }
}

s32 findFreeMaterial(void) {
    for (s32 i=1; i<MAX_MATERIAL_QTY; ++i) {
        if (sMaterials[i].mVersion==0) {
            return i;
        }
    }
    return -1;
}

void setViewport(s32 x, s32 y, s32 width, s32 height) {
#ifdef DEBUG
    if (x<0 || y<0 || width<=0 || height<=0) logError(TAG, "Invalid viewport [%d %d %d %d]", x, y, width, height);
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
#ifdef GL_OGLES1
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_OES, &fbo);
#elif defined(GL_OGLES2)
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
#endif
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



void useDithering(bool flag) {
    if (flag) {
        glEnable(GL_DITHER);
    } else {
        glDisable(GL_DITHER);
    }
}

void useBlending(bool flag) {
    if (flag) {
        sUseBlending = true;
        glEnable(GL_BLEND);
    } else {
        sUseBlending = false;
        glDisable(GL_BLEND);
    }
}

void setTransMode(u8 mode) {
    switch (mode) {
        case 0:
            sUseBlending = false;
            glDisable(GL_BLEND);
            break;
        case 1:
            sUseBlending = true;
            glEnable(GL_BLEND);
            break;
        default:
            logInfo(TAG, "Trans Mode unkown");
            break;
    }
}

void restoreUseBlending(void) {
    if (sUseBlending) {
        glDisable(GL_BLEND);
    } else {
        glEnable(GL_BLEND);
    }
}

GLenum convertBlendFactor(E_BLEND_FACTOR f) {
    switch (f) {
        case BF_ZERO:
            return GL_ZERO;
        case BF_ONE:
            return GL_ONE;
        case BF_SRC_ALPHA:
            return GL_SRC_ALPHA;
        case BF_ONE_MINUS_SRC_COLOR:
            return GL_ONE_MINUS_SRC_COLOR;
        case BF_ONE_MINUS_SRC_ALPHA:
            return GL_ONE_MINUS_SRC_ALPHA;
        default:
            logError(TAG, "Unknown blend factor %d", f);
            return BF_ZERO;
    }
}

void setBlendFunc(E_BLEND_FACTOR src, E_BLEND_FACTOR dst) {
    GLenum s = convertBlendFactor(src);
    GLenum d = convertBlendFactor(dst);
    glBlendFunc(s,d);
}

void useDepthTest(bool flag) {
    if (flag) {
        sUseDepthTest = true;
        glEnable(GL_DEPTH_TEST);
    } else {
        sUseDepthTest = false;
        glDisable(GL_DEPTH_TEST);
    }
}

void setDepthTest(E_DEPTH_TEST test) {
    switch (test) {
        case DEPTH_TEST_NEVER:
            sDepthTestMode = 0;
            glDepthFunc(GL_NEVER);
        break;
        case DEPTH_TEST_LESS:
            sDepthTestMode = 1;
            glDepthFunc(GL_LESS);
        break;
        case DEPTH_TEST_EQUAL:
            sDepthTestMode = 2;
            glDepthFunc(GL_EQUAL);
        break;
        case DEPTH_TEST_LEQUAL:
            sDepthTestMode = 3;
            glDepthFunc(GL_LEQUAL);
        break;
        case DEPTH_TEST_GREATER:
            sDepthTestMode = 4;
            glDepthFunc(GL_GREATER);
        break;
        case DEPTH_TEST_NOTEQUAL:
            sDepthTestMode = 5;
            glDepthFunc(GL_NOTEQUAL);
        break;
        case DEPTH_TEST_GEQUAL:
            sDepthTestMode = 6;
            glDepthFunc(GL_GEQUAL);
        break;
        case DEPTH_TEST_ALWAYS:
            sDepthTestMode = 7;
            glDepthFunc(GL_ALWAYS);
        break;
        default:
            logError(TAG, "Depth Func not supported [%d]", test);
        break;
    }
}

void setDepthMode(u8 mode) {
    switch (mode) {
        case 0:
            sUseDepthTest = false;
            sDepthTestMode = 0;
            glDisable(GL_DEPTH_TEST);
            break;
        case 1:
            sDepthTestMode = 4;
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_GREATER);
            break;
        default:
            logInfo(TAG, "Depth Mode unkown");
            break;
    }
}

void restoreDepthTest(void) {
    useDepthTest(sUseDepthTest);
    setDepthTest(sDepthTestMode);
}

void setClearColor(f32 r, f32 g, f32 b, f32 a) {
    glClearColor(r,g,b,a);
}

void clearBuffers(bool colorBuffer, bool depthBuffer, bool stencilBuffer) {
    GLbitfield flags = 0;
    if (colorBuffer) {
        flags |= GL_COLOR_BUFFER_BIT;
    }
    if (depthBuffer) {
        flags |= GL_DEPTH_BUFFER_BIT;
    }
    if (stencilBuffer) {
        flags |= GL_STENCIL_BUFFER_BIT;
    }
    glClear(flags);

}

void readPixels(s32 x,s32 y, s32 width, s32 height, s8 format, u8 *dst) {
    switch(format) {
        case 2:
            glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, dst);
            break;
        case 3:
            glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, dst);
        case 0:
        default:
            logError(TAG, "Format not supported or implemented %d", format);
    }
}



bool hasError(void) {
    GLenum error = glGetError();
    if (error == GL_NO_ERROR) {
        return false;
    }
    return true;
}



Material *createMaterial() {
    s32 index = findFreeMaterial();
    if (index<1) return NULL;
    Material *mat = &(sMaterials[index]);
    mat->mUID = index;
    mat->mVersion = 1;
    mat->mType = 0;
    mat->mDepthMode = 0;
    mat->mTransMode = 0;
    mat->mTexturePtrId = -1;
    return mat;
}

Material *getMaterial(s32 index) {
    Material *mat = &(sMaterials[index]);
    if (mat->mVersion==0) return NULL;
    return mat;
}

void destroyMaterial(s32 index) {
    logInfo(TAG, "destroyMaterial %d", index);
    Material *mat = &(sMaterials[index]);
    if (mat) {
        mat->mVersion = 0;
    }
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