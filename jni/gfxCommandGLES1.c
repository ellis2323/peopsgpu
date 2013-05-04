#include "gfxCommand.h"
#include "gfxTexture.h"
#include "gfxContext.h"
#include "gpuExternals.h"
#include "gpuPlugin.h"
#include "gfxGL.h"
#include "gfxMatrix.h"
#include <math.h>

#define TAG "ELLIS"

#if defined(GL_OGLES1)

static f32 *sMatrixProjection;
static f32 *sMatrixModelView;

// PSX Context
static Material* sPrevMaterial = NULL;
static E_DRAWTYPE sPrevDrawType = DRAWTYPE_FLAT;
static bool sUsePrevAlphaTest = false;
static E_ALPHA_TEST sPrevAlphaTestEnum = ALPHA_TEST_ALWAYS;
static f32 sPrevAlphaTestValue = 1.0;
static u32 sCurrentColor;
// in material
static u8 sLastDepthMode;
static u8 sLastTransMode;

void drawTriGou(OGLVertex *vertices, u16 *indices, s16 count);
void drawTexTriGou(Material *mat, OGLVertex *vertices, u16 *indices, s16 count);
void mali400(void);

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

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    
    sMatrixModelView = createIdentityMatrix();
    sMatrixProjection = createIdentityMatrix();
}



void setDrawMode(E_DRAWTYPE m) {
    sPrevDrawType = m;
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

void useAlphaTest(bool flag) {
    sUsePrevAlphaTest = flag;
    if (flag) {
        glEnable(GL_ALPHA_TEST);
    } else {
        glDisable(GL_ALPHA_TEST);
    }
}

void setAlphaTest(E_ALPHA_TEST test, f32 value) {
    sPrevAlphaTestEnum = test;
    sPrevAlphaTestValue = value;
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

void getModelViewMatrix(f32 *matrix) {
    // replace this instruction
    // glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
    for (s32 i=0; i<16; ++i) {
        matrix[i] = sMatrixModelView[i];
    }
}

void setModelViewMatrix(f32 *matrix) {
    for (s32 i=0; i<16; ++i) {
        sMatrixModelView[i] = matrix[i];
    }    
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(matrix);
}

void getProjectionMatrix(f32 *matrix) {
    // replace this instruction
    // glGetFloatv(GL_PROJECTION, matrix);
    for (s32 i=0; i<16; ++i) {
        matrix[i] = sMatrixProjection[i];
    }
}

void setProjectionMatrix(f32 *matrix) {
    for (s32 i=0; i<16; ++i) {
        sMatrixProjection[i] = matrix[i];
    }
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(matrix);
}

void setProjectionOrtho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    logProjection("Projection ", left, right, bottom, top, near, far);
    if ((fabsf(right-left)<0.01f) || (fabsf(top-bottom)<0.01f) || (fabsf(far-near)<0.01f)) {
        logError(TAG, "Invalid projection Reset");
        if (fabsf(right-left)<0.01f) { left=0; right=256; }
        if (fabsf(top-bottom)<0.01f) { bottom=256; top=0; }
        if (fabsf(far-near)<0.01f)   { near=-1; far=1; }
        logProjection("Fix", left, right, bottom, top, near, far);
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

void setColor(GLSLColor color) {
    sCurrentColor=color.lcol;
    glColor4ub(color.col[0],color.col[1],color.col[2],color.col[3]);
}

GLSLColor getColor() {
    GLSLColor v;
    v.lcol = sCurrentColor;
    return v;
}

// Cache Information
// - sCSVERTEX is for Client State of Vertices.
// - sCSCOLOR of Colors Vertices.
// - sCSTEXTURE of TexCoord Vertices
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
    if (count == 0 || vertices == NULL || indices == NULL) return;

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
    
    glEnable(GL_TEXTURE_2D);
    Texture *tex = getTexture(mat->mTexturePtrId);
    // bind to texture if needed
    //if (mat->mTexture.mTextureId != sCTextureId) {
        sCTextureId = tex->mTextureId;
        glBindTexture(GL_TEXTURE_2D, sCTextureId);
    //}
    
    // change Min&Mag filters
    //if (mat->mTexture.mFilters != sCTextureFilters) {
        sCTextureFilters = tex->mFilters;
        switch (sCTextureFilters) {
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
        }
    //}
    
    //if (mat->mTexture.mClampTypes != sCTextureClampTypes) {
        sCTextureClampTypes = tex->mClampTypes;
        switch (sCTextureClampTypes) {
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
        }
    //}
    
    if (count == 0 || vertices == NULL || indices == NULL) return;

    // change
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_SCISSOR_TEST);
    
    glVertexPointer(3, GL_FLOAT, sizeof(OGLVertex), &vertices[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(OGLVertex), &vertices[0].c.lcol);
    glTexCoordPointer(2, GL_FLOAT, sizeof(OGLVertex), &vertices[0].sow);
    
    
    checkTexture();
    
    glDrawElements(GL_TRIANGLES, 3*count, GL_UNSIGNED_SHORT, indices);
    
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    glDisable(GL_TEXTURE_2D);
    
    //logInfo(TAG, "draw drawTexTriGou");
    sCSVERTEX=1;
    sCSCOLOR=1;
    sCSTEXTURE=1;
}

void loadMaterial() {
    // restore ctx
//   setDrawMode(sPrevDrawType);
//    useAlphaTest(sUsePrevAlphaTest);
//    setAlphaTest(sPrevAlphaTestEnum, sPrevAlphaTestValue);
    GLSLColor v;
    v.lcol = sCurrentColor;
    setColor(v);
    // restore material
    Material* mat = sPrevMaterial;
    if (mat == NULL) return;
    setDepthMode(mat->mDepthMode);
    setTransMode(mat->mTransMode);
    
    switch (mat->mType) {
        case 0:
            drawTriGou(NULL, NULL, 0);
            break;
        case 1:
            drawTexTriGou(mat, NULL, NULL, 0);
            break;
        default:
            logInfo(TAG, "Unkown type of material");
    }
}

void saveMaterial(Material* mat) {
    if (mat == NULL) return;
    sPrevMaterial = mat;
}


void drawTriangles(Material *mat, OGLVertex *vertices, u16 *indices, s16 count) {
    Context* ctx = getContext();
    if (ctx && ctx->mFBO) {
        saveMaterial(mat);
    }
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

unsigned int CSVERTEX=0,CSCOLOR=0,CSTEXTURE=0;

void SETCOL(OGLVertex x) {
    if(x.c.lcol!=ulOLDCOL) {
        ulOLDCOL=x.c.lcol;
        glColor4ub(x.c.col[0],x.c.col[1],x.c.col[2],x.c.col[3]);
    }
}


////////////////////////////////////////////////////////////////////////
// OpenGL primitive drawing commands
////////////////////////////////////////////////////////////////////////

void PRIMdrawTexturedQuad(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3, OGLVertex* vertex4)
{
    OGLVertex v[4];    
    v[0] = *vertex1;
    v[1] = *vertex2;
    v[2] = *vertex4;
    v[3] = *vertex3;
    
    if (CSCOLOR==1) glDisableClientState(GL_COLOR_ARRAY);
    if (CSTEXTURE==0) glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);
    
    glTexCoordPointer(2, GL_FLOAT, sizeof(v[0]), &v[0].sow);
    glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0].x);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    CSTEXTURE=CSVERTEX=1;
    CSCOLOR=0;
}

/////////////////////////////////////////////////////////

void PRIMdrawTexturedTri(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3)
{
    if (vertex1->x==0 && vertex1->y==0 && vertex2->x==0 && vertex2->y==0 && vertex3->x==0 && vertex3->y==0) return;

    OGLVertex v[3];
    v[0] = *vertex1;
    v[1] = *vertex2;
    v[2] = *vertex3;
        
    if (CSCOLOR==1) glDisableClientState(GL_COLOR_ARRAY);
    if (CSTEXTURE==0) glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, sizeof(v[0]), &v[0].sow);
    glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0].x);
    
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    CSTEXTURE=CSVERTEX=1;
    CSCOLOR=0;
}

/////////////////////////////////////////////////////////

void PRIMdrawTexGouraudTriColor(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3)
{
    if (vertex1->x==0&&vertex1->y==0&&vertex2->x==0&&vertex2->y==0&&vertex3->x==0&&vertex3->y==0) return;
    
    OGLVertex v[3];
    v[0] = *vertex1;
    v[1] = *vertex2;
    v[2] = *vertex3;
    
    if (CSTEXTURE==0) glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);
    if (CSCOLOR==0) glEnableClientState(GL_COLOR_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, sizeof(v[0]), &v[0].sow);
    glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(v[0]), &v[0].c);
    
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    CSTEXTURE=CSVERTEX=CSCOLOR=1;
}

/////////////////////////////////////////////////////////

void PRIMdrawTexGouraudTriColorQuad(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3, OGLVertex* vertex4)
{
    if (vertex1->x==0 && vertex1->y==0 && vertex2->x==0 && vertex2->y==0 && vertex3->x==0 && vertex3->y==0 && vertex4->x==0 && vertex4->y==0) return;
    
    OGLVertex v[4];
    v[0] = *vertex1;
    v[1] = *vertex2;
    v[2] = *vertex4;
    v[3] = *vertex3;
    
    if (CSTEXTURE==0) glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);
    if (CSCOLOR==0) glEnableClientState(GL_COLOR_ARRAY);
    
    glTexCoordPointer(2, GL_FLOAT, sizeof(v[0]), &v[0].sow);
    glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(v[0]), &v[0].c);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    CSTEXTURE=CSVERTEX=CSCOLOR=1;
}

/////////////////////////////////////////////////////////

void PRIMdrawTri(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3)
{
    if (vertex1->x==0 && vertex1->y==0 && vertex2->x==0 && vertex2->y==0 && vertex3->x==0 && vertex3->y==0) return;
    
    OGLVertex v[3];
    v[0] = *vertex1;
    v[1] = *vertex2;
    v[2] = *vertex3;
    
    if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);
    if (CSTEXTURE==1) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    if (CSCOLOR==1) glDisableClientState(GL_COLOR_ARRAY);
    
    glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0].x);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    CSVERTEX=1;
    CSTEXTURE=CSCOLOR=0;
}

/////////////////////////////////////////////////////////

void PRIMdrawTri2(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3, OGLVertex* vertex4)
{
    if (vertex1->x==0&&vertex1->y==0&&vertex2->x==0&&vertex2->y==0&&vertex3->x==0&&vertex3->y==0&&vertex4->x==0&&vertex4->y==0) return;
    
    OGLVertex v[4];
    v[0] = *vertex1;
    v[1] = *vertex3;
    v[2] = *vertex2;
    v[3] = *vertex4;
    
    if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);
    if (CSTEXTURE==1) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    if (CSCOLOR==1) glDisableClientState(GL_COLOR_ARRAY);
    
    glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0].x);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    CSVERTEX=1;
    CSTEXTURE=CSCOLOR=0;
}

/////////////////////////////////////////////////////////

void PRIMdrawGouraudTriColor(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3)
{
    if (vertex1->x==0&&vertex1->y==0&&vertex2->x==0&&vertex2->y==0&&vertex3->x==0&&vertex3->y==0) return;

    OGLVertex v[3];
    v[0] = *vertex1;
    v[1] = *vertex2;
    v[2] = *vertex3;
    
    if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);
    if (CSCOLOR==0) glEnableClientState(GL_COLOR_ARRAY);
    if (CSTEXTURE==1) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    
    glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(v[0]), &v[0].c);
    
    glDrawArrays(GL_TRIANGLES, 0, 3);
    CSVERTEX=CSCOLOR=1;
    CSTEXTURE=0;
}

/////////////////////////////////////////////////////////

void PRIMdrawGouraudTri2Color(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3, OGLVertex* vertex4)
{
    if (vertex1->x==0&&vertex1->y==0&&vertex2->x==0&&vertex2->y==0&&vertex3->x==0&&vertex3->y==0&&vertex4->x==0&&vertex4->y==0) return;

    OGLVertex v[4];
    v[0] = *vertex1;
    v[1] = *vertex2;
    v[2] = *vertex3;
    v[3] = *vertex4;

    if (CSTEXTURE==1) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);
    if (CSCOLOR==0) glEnableClientState(GL_COLOR_ARRAY);
    
    glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(v[0]), &v[0].c);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    CSTEXTURE=0;
    CSVERTEX=CSCOLOR=1;
}

/////////////////////////////////////////////////////////

void PRIMdrawFlatLine(OGLVertex* vertex1, OGLVertex* vertex2,OGLVertex* vertex3, OGLVertex* vertex4)
{
    if (vertex1->x==0&&vertex1->y==0&&vertex2->x==0&&vertex2->y==0&&vertex3->x==0&&vertex3->y==0&&vertex4->x==0&&vertex4->y==0) return;

    OGLVertex v[4];
    v[0] = *vertex1;
    v[1] = *vertex2;
    v[2] = *vertex4;
    v[3] = *vertex3;
    
    if (CSTEXTURE==1) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);
    if (CSCOLOR==0) glEnableClientState(GL_COLOR_ARRAY);
    
    glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(v[0]), &v[0].c);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    CSTEXTURE=0;
    CSVERTEX=CSCOLOR=1;
    
    
}

/////////////////////////////////////////////////////////

void PRIMdrawGouraudLine(OGLVertex* vertex1, OGLVertex* vertex2,OGLVertex* vertex3, OGLVertex* vertex4)
{
    if (vertex1->x==0&&vertex1->y==0&&vertex2->x==0&&vertex2->y==0&&vertex3->x==0&&vertex3->y==0&&vertex4->x==0&&vertex4->y==0) return;

    OGLVertex v[4];
    v[0] = *vertex1;
    v[1] = *vertex2;
    v[2] = *vertex3;
    v[3] = *vertex4;
    
    if (CSTEXTURE==1) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);
    if (CSCOLOR==0) glEnableClientState(GL_COLOR_ARRAY);
    
    glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(v[0]), &v[0].c);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    CSTEXTURE=0;
    CSVERTEX=CSCOLOR=1;
}

/////////////////////////////////////////////////////////

void PRIMdrawQuad(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3, OGLVertex* vertex4)
{
    if (vertex1->x==0&&vertex1->y==0&&vertex2->x==0&&vertex2->y==0&&vertex3->x==0&&vertex3->y==0&&vertex4->x==0&&vertex4->y==0) return;
    
    OGLVertex v[4];
    v[0] = *vertex1;
    v[1] = *vertex2;
    v[2] = *vertex4;
    v[3] = *vertex3;
    
    if (CSTEXTURE==1) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);
    if (CSCOLOR==1) glDisableClientState(GL_COLOR_ARRAY);
    
    glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0].x);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    CSTEXTURE=0;
    CSVERTEX=1;
    CSCOLOR=0;
}



extern GLubyte *texturepart;

void mali400() {
return;
    Vertex v[4];
    Vertex2 v2[4];
    //Vec3f v3[4];
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glAlphaFuncx(GL_NOTEQUAL,0);
    glDisable(GL_BLEND);
    glBlendFunc(770,770);
    glLoadIdentity();
    glOrthof(0,256,1, 0, -1, 1);
    glScissor(0,0,iResX,iResY);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0,0,0,1.0f);
    glClear(16384);
    glEnable(GL_SCISSOR_TEST);
    glLoadIdentity();
    glOrthof(0,256,251, 0, -1, 1);
    glLoadIdentity();
    glOrthof(0,368,502, 0, -1, 1);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0,0,0,128);
    glClear(16384);
    glEnable(GL_SCISSOR_TEST);
    glGenTextures(1, &gTexName);
    glBindTexture(GL_TEXTURE_2D, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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

