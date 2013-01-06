#include "gfxCommand.h"
#include "gpuExternals.h"
#include "gpuPlugin.h"

#if defined(GL_OGLES1)

s32 sCSVERTEX = -1;
s32 sCSCOLOR = -1;
s32 sCSTEXTURE = -1;

FBO *createFBO(s32 width, s32 height, bool hd) {
    GLuint framebuffer;
    GLuint depthRenderbuffer;
    GLuint texture;
    // ids
    glGenFramebuffersOES(1, &framebuffer);
    glGenTextures(1, &texture);
    glGenRenderbuffersOES(1, &depthRenderbuffer);
    
    // create texture
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    if (hd) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8_OES, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
    }
    // create depth renderbuffer
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, depthRenderbuffer);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, width, height);

    // bind framebuffer & attach texture 
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, framebuffer);
    glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_TEXTURE_2D, texture, 0);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, depthRenderbuffer);

    if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
        return 0;
    }
    
    FBO *fbo = (FBO *)malloc(sizeof(FBO));
    fbo->mFBO = framebuffer;
    fbo->mTexture = texture;
    fbo->mDepthRenderBuffer = depthRenderbuffer;
    return fbo;
}

void destroyFBO(FBO *fbo) {
    
}

void useFBO(FBO *fbo) {
    if(fbo) {
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, fbo->mFBO);
    } else {
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, 0);
    }
}

void drawTriangles(Material *mat, OGLVertex *vertices, u16 *indices, s16 count) {
    setDepthMode(mat->mDepthMode);
    setTransMode(mat->mTransMode);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(3, GL_FLOAT, sizeof(OGLVertex), &vertices[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(OGLVertex), &vertices[0].c.lcol);
    
    glDrawElements(GL_TRIANGLES, 3*count, GL_UNSIGNED_SHORT, indices);
    
    sCSVERTEX=1;
    sCSCOLOR=1;
    sCSTEXTURE=0;

}

// MARK: Private

static u8 sLastDepthMode;
void setDepthMode(u8 mode) {
    if (mode==sLastDepthMode) return;
    sLastDepthMode = mode;
    switch (mode) {
        case 0:
        glDisable(GL_DEPTH_TEST);
        break;
        case 1:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_GREATER);
        break;
    default:
        logInfo(TAG, "Depth Mode unkown");
        break;
    }
}

static u8 sLastTransMode;
void setTransMode(u8 mode) {
    if (mode==sLastTransMode) return;
    sLastTransMode = mode;
    switch (mode) {
        case 0:
        glDisable(GL_BLEND);
        break;
        case 1:
        glEnable(GL_BLEND);
        break;
    default:
        logInfo(TAG, "Depth Mode unkown");
        break;
    }
}

extern GLubyte *texturepart;
void mali400() {
    Vertex v[4];
    Vertex2 v2[4];
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glAlphaFuncx(GL_NOTEQUAL,0);
    glDisable(GL_BLEND);
    glBlendFunc(770,770);
    glLoadIdentity();
    glOrtho(0,256,0, 0, -1, 1);
    glScissor(0,0,iResX,iResY);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0,0,0,1.0f);
    glClear(16384);
    glEnable(GL_SCISSOR_TEST);
    glLoadIdentity();
    glOrtho(0,256,251, 0, -1, 1);
    glLoadIdentity();
    glOrtho(0,368,502, 0, -1, 1);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0,0,0,128);
    glClear(16384);
    glEnable(GL_SCISSOR_TEST);
    glGenTextures(1, &gTexName);
    glBindTexture(GL_TEXTURE_2D, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, iClampType);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, iClampType);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0,GL_RGBA, GL_UNSIGNED_BYTE, texturepart);
    glEnable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glBindTexture(GL_TEXTURE_2D, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0,196,128,33,GL_RGBA, GL_UNSIGNED_BYTE, texturepart);
    v2[0].st.x=0.005207f;
    v2[0].st.y=0.770862f;
    v2[1].st.x=0.490906f;
    v2[1].st.y=0.770862f;
    v2[2].st.x=0.005207f;
    v2[2].st.y=0.885453f;
    v2[3].st.x=0.490906f;
    v2[3].st.y=0.885453f;
    v2[0].xyz.x=33.000000f;
    v2[0].xyz.y=70.000000f;
    v2[0].xyz.z=0.000000f;
    v2[1].xyz.x=159.000000f;
    v2[1].xyz.y=70.000000f;
    v2[1].xyz.z=0.000000f;
    v2[2].xyz.x=33.000000f;
    v2[2].xyz.y=100.000000f;
    v2[2].xyz.z=0.000000f;
    v2[3].xyz.x=159.000000f;
    v2[3].xyz.y=100.000000f;
    v2[3].xyz.z=0.000000f;
    v2[0].rgba.r=0;
    v2[0].rgba.g=0;
    v2[0].rgba.b=0;
    v2[0].rgba.a=0;
    v2[1].rgba.r=0;
    v2[1].rgba.g=0;
    v2[1].rgba.b=0;
    v2[1].rgba.a=0;
    v2[2].rgba.r=0;
    v2[2].rgba.g=0;
    v2[2].rgba.b=0;
    v2[2].rgba.a=0;
    v2[3].rgba.r=0;
    v2[3].rgba.g=0;
    v2[3].rgba.b=0;
    v2[3].rgba.a=0;
    glTexCoordPointer(2, GL_FLOAT, 24, &v2[0].st);
    glVertexPointer(3, GL_FLOAT, 24, &v2[0].xyz);
    glColorPointer(4, GL_UNSIGNED_BYTE, 24, &v2[0].rgba);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glGenTextures(1, &gTexName);
    glBindTexture(GL_TEXTURE_2D, 2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, iClampType);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, iClampType);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0,GL_RGBA, GL_UNSIGNED_BYTE, texturepart);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0,20,78,GL_RGBA, GL_UNSIGNED_BYTE, texturepart);
    glShadeModel(GL_FLAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glDisableClientState(GL_COLOR_ARRAY);
    v[0].st.x=0.003906f;
    v[0].st.y=0.003906f;
    v[1].st.x=0.074222f;
    v[1].st.y=0.003906f;
    v[2].st.x=0.003906f;
    v[2].st.y=0.300793f;
    v[3].st.x=0.074222f;
    v[3].st.y=0.300793f;
    v[0].xyz.x=27.000000f;
    v[0].xyz.y=385.000000f;
    v[0].xyz.z=0.000000f;
    v[1].xyz.x=45.000000f;
    v[1].xyz.y=385.000000f;
    v[1].xyz.z=0.000000f;
    v[2].xyz.x=27.000000f;
    v[2].xyz.y=461.000000f;
    v[2].xyz.z=0.000000f;
    v[3].xyz.x=45.000000f;
    v[3].xyz.y=461.000000f;
    v[3].xyz.z=0.000000f;
    glTexCoordPointer(2, GL_FLOAT, 20, &v[0].st);
    glVertexPointer(3, GL_FLOAT, 20, &v[0].xyz);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_SCISSOR_TEST);
    flipEGL();
}


#endif // GL_OGLES1

