#include "gfxCommand.h"
#include "gfxTexture.h"
#include "gfxContext.h"
#include "gpuExternals.h"
#include "gpuPlugin.h"
#include "gfxGL.h"

#define TAG "ELLIS"

#if defined(GL_OGLES1)

void initGL() {

    glClearDepthf(1.0f);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glHint(GL_GENERATE_MIPMAP_HINT, GL_FASTEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
    glHint(GL_POINT_SMOOTH_HINT, GL_FASTEST);

    glDisable(GL_FOG);
    glDisable(GL_LIGHTING);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POINT_SMOOTH);

    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
    glFrontFace( GL_CW );
    
    glShadeModel(GL_SMOOTH);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    
    glPixelStorei(GL_PACK_ALIGNMENT,1);
}

void useAlphaTest(bool flag) {
    if (flag) {
        glEnable(GL_ALPHA_TEST);
    } else {
        glDisable(GL_ALPHA_TEST);
    }
}

void setAlphaTest(E_ALPHA_TEST test, f32 value) {
    switch (test) {
        case ALPHA_TEST_NEVER:
            glAlphaFunc(GL_NEVER, value);
        break;
        case ALPHA_TEST_LESS:
            glAlphaFunc(GL_LESS, value);
        break;
        case ALPHA_TEST_EQUAL:
            glAlphaFunc(GL_EQUAL, value);
        break;
        case ALPHA_TEST_LEQUAL:
            glAlphaFunc(GL_LEQUAL, value);
        break;
        case ALPHA_TEST_GREATER:
            glAlphaFunc(GL_GREATER, value);
        break;
        case ALPHA_TEST_NOTEQUAL:
            glAlphaFunc(GL_NOTEQUAL, value);
        break;
        case ALPHA_TEST_GEQUAL:
            glAlphaFunc(GL_GEQUAL, value);
        break;
        case ALPHA_TEST_ALWAYS:
            glAlphaFunc(GL_ALWAYS, value);
        break;
        default:
            logError(TAG, "Alpha Func not supported [%d]", test);
        break;
    }
}

f32 sMatrixProjection[16];
void setProjectionMatrix(f32 *matrix) {
    memcpy(sMatrixProjection, matrix, 16*sizeof(f32));
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(sMatrixProjection);
}

void setProjectionOrtho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    if (((right-left)==0) || ((top-bottom)==0) || ((far-near)==0)) {
        logError(TAG, "Invalid projection Reset");
        left=0; right=256;
        bottom=256; top=0;
        near=-1; far=1;
    }
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

u32 sCurrentColor;
void setColor(GLSLColor color) {
    sCurrentColor=color.lcol;
    glColor4ub(color.col[0],color.col[1],color.col[2],color.col[3]);
}

GLSLColor getColor() {
    GLSLColor v;
    v.lcol = sCurrentColor;
    return v;
}

void setDrawMode(E_DRAWTYPE m) {
    switch (m) {
        case DRAWTYPE_FLAT:
            glShadeModel(GL_FLAT);
            return;
        case DRAWTYPE_SMOOTH:
            glShadeModel(GL_SMOOTH);
            return;
        default:
            logError(TAG, "DrawType not supported or implemented %d", m);
            break;
    }
}

// Cache Information
// - sCSVERTEX is for Client State of Vertices. sCSCOLOR of Colors Vertices. sCSTEXTURE of TexCoord Vertices
// - sCTextureId is for Texture Id
s32 sCSVERTEX = -1;
s32 sCSCOLOR = -1;
s32 sCSTEXTURE = -1;
static s32 sCTextureId = -1;
static s8 sCTextureFilters = -1;
static s8 sCTextureClampTypes = -1;


/*
 ShadeModel is SMOOTH.
 Flat or Gouraud are equal method.
 */
void drawTriGou(OGLVertex *vertices, u16 *indices, s16 count) {
    /*if (sCSVERTEX) glEnableClientState(GL_VERTEX_ARRAY);
    if (sCSCOLOR) glEnableClientState(GL_COLOR_ARRAY);
    if (sCSTEXTURE) glDisableClientState(GL_TEXTURE_COORD_ARRAY);*/
    
    glDisable(GL_TEXTURE_2D);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    glVertexPointer(3, GL_FLOAT, sizeof(OGLVertex), &vertices[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(OGLVertex), &vertices[0].c.lcol);
    
    glDrawElements(GL_TRIANGLES, 3*count, GL_UNSIGNED_SHORT, indices);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    sCSVERTEX=1;
    sCSCOLOR=1;
    sCSTEXTURE=0;
}

/*
 ShadeModel is SMOOTH.
 Texture or Gouraud Textured are equal method.
 */
void drawTexTriGou(Material *mat, OGLVertex *vertices, u16 *indices, s16 count) {
#ifdef DEBUG
    if (mat==NULL) logError(TAG, "drawTexTriGou with NULL MATERIAL!");
#endif
    /*if (sCSVERTEX) glEnableClientState(GL_VERTEX_ARRAY);
    if (sCSCOLOR) glEnableClientState(GL_COLOR_ARRAY);
    if (sCSTEXTURE) glEnableClientState(GL_TEXTURE_COORD_ARRAY);*/
    glShadeModel(GL_SMOOTH);
    glDisable(GL_BLEND);
    Texture *tex = getTexture(mat->mTexturePtrId);
    // bind to texture if needed
    //if (mat->mTexture.mTextureId != sCTextureId) {
    sCTextureId = tex->mTextureId;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, sCTextureId);
    //}
    
    // change Min&Mag filters
    //if (mat->mTexture.mFilters != sCTextureFilters) {
        sCTextureFilters = tex->mFilters;
    /*    switch (sCTextureFilters) {
            case 0:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                break;
            case 1:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                break;
            default:
                logError(TAG, "filter mode unknown");
        }*/
    //}
    
    //if (mat->mTexture.mClampTypes != sCTextureClampTypes) {
        sCTextureClampTypes = tex->mClampTypes;
     /*   switch (sCTextureClampTypes) {
            case 0:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                break;
            case 1:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                break;
        default:
            logError(TAG, "clamp types");
        }*/
    //}
    
    // change
    //glDisable(GL_TEXTURE_2D);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0,GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glVertexPointer(3, GL_FLOAT, sizeof(OGLVertex), &vertices[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(OGLVertex), &vertices[0].c.lcol);
    glTexCoordPointer(2, GL_FLOAT, sizeof(OGLVertex), &vertices[0].sow);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_ALPHA_TEST);
    glColor4f(1,1,1,1);
    glDisable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glAlphaFunc(GL_ALWAYS, 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_LIGHTING);
    glScissor(0,0,256,256);
    //checkTexture();
    
    glDrawElements(GL_TRIANGLES, 3*count, GL_UNSIGNED_SHORT, indices);
    
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    glDisable(GL_TEXTURE_2D);
    
        glClear(16384);
    glFinish();

    
    //logInfo(TAG, "draw drawTexTriGou");
    sCSVERTEX=1;
    sCSCOLOR=1;
    sCSTEXTURE=1;
}

void drawTriangles(Material *mat, OGLVertex *vertices, u16 *indices, s16 count) {
    setDepthMode(mat->mDepthMode);
    setTransMode(mat->mTransMode);
    
    switch (mat->mType) {
        case 0:
            drawTriGou(vertices, indices, count);
            break;
        case 1:
            drawTexTriGou(mat, vertices, indices, count);
            break;
        default:
            logInfo(TAG, "Unkown type of material");
    }
}

// MARK: Private

static u8 sLastDepthMode;
void setDepthMode(u8 mode) {
    //if (mode==sLastDepthMode) return;
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
    //if (mode==sLastTransMode) return;
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
    //Vertex v[4];
    Vertex2 v2[4];

    glGenTextures(1, &gTexName);
    glBindTexture(GL_TEXTURE_2D, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0,GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glEnable(GL_TEXTURE_2D);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
        

    glTexCoordPointer(2, GL_FLOAT, 24, &v2[0].st);
    glVertexPointer(3, GL_FLOAT, 24, &v2[0].xyz);
    glColorPointer(4, GL_UNSIGNED_BYTE, 24, &v2[0].rgba);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  
    glGenTextures(1, &gTexName);
    glBindTexture(GL_TEXTURE_2D, 2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0,GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    
}

#endif // GL_OGLES1

