#include "gfxCommand.h"
#include "gfxTexture.h"
#include "gfxContext.h"
#include "gpuExternals.h"
#include "gpuPlugin.h"
#include "gfxGL.h"

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

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    
    glPixelStorei(GL_PACK_ALIGNMENT,1);
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
void mali4000 () {
    glClearDepthf(1.0f);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glHint(GL_GENERATE_MIPMAP_HINT, GL_FASTEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
    glHint(GL_POINT_SMOOTH_HINT, GL_FASTEST);
    glDepthFunc(GL_LEQUAL);
    glFrontFace( GL_CW );

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

return;
/*    Vertex v[4];
    Vertex2 v2[4];
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glAlphaFuncx(GL_NOTEQUAL,0);
    glDisable(GL_BLEND);
    glBlendFunc(770,770);
    glLoadIdentity();
    glOrtho(0,256,256, 0, -1, 1);
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
    flipEGL();*/
}


#endif // GL_OGLES1

