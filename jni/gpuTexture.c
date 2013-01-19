/***************************************************************************
                          texture.c  -  description
                             -------------------
    begin                : Sun Mar 08 2009
    copyright            : (C) 1999-2009 by Pete Bernert
    web                  : www.pbernert.com   
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

//*************************************************************************// 
// History of changes:
//
// 2009/03/08 - Pete  
// - generic cleanup for the Peops release
//
//*************************************************************************// 


////////////////////////////////////////////////////////////////////////////////////
// Texture related functions are here !
//
// The texture handling is heart and soul of this gpu. The plugin was developed
// 1999, by this time no shaders were available. Since the psx gpu is making
// heavy use of CLUT (="color lookup tables", aka palettized textures), it was 
// an interesting task to get those emulated at good speed on NV TNT cards 
// (which was my major goal when I created the first "gpuPeteTNT"). Later cards 
// (Geforce256) supported texture palettes by an OGL extension, but at some point
// this support was dropped again by gfx card vendors.
// Well, at least there is a certain advatage, if no texture palettes extension can
// be used: it is possible to modify the textures in any way, allowing "hi-res" 
// textures and other tweaks.
//
// My main texture caching is kinda complex: the plugin is allocating "n" 256x256 textures,
// and it places small psx texture parts inside them. The plugin keeps track what 
// part (with what palette) it had placed in which texture, so it can re-use this 
// part again. The more ogl textures it can use, the better (of course the managing/
// searching will be slower, but everything is faster than uploading textures again
// and again to a gfx card). My first card (TNT1) had 16 MB Vram, and it worked
// well with many games, but I recommend nowadays 64 MB Vram to get a good speed.
//
// Sadly, there is also a second kind of texture cache needed, for "psx texture windows".
// Those are "repeated" textures, so a psx "texture window" needs to be put in 
// a whole texture to use the GL_TEXTURE_WRAP_ features. This cache can get full very
// fast in games which are having an heavy "texture window" usage, like RRT4. As an 
// alternative, this plugin can use the OGL "palette" extension on texture windows, 
// if available. Nowadays also a fragment shader can easily be used to emulate
// texture wrapping in a texture atlas, so the main cache could hold the texture
// windows as well (that's what I am doing in the OGL2 plugin). But currently the
// OGL1 plugin is a "shader-free" zone, so heavy "texture window" games will cause
// much texture uploads.
//
// Some final advice: take care if you change things in here. I've removed my ASM
// handlers (they didn't cause much speed gain anyway) for readability/portability,
// but still the functions/data structures used here are easy to mess up. I guess it
// can be a pain in the ass to port the plugin to another byte order :)
//
////////////////////////////////////////////////////////////////////////////////////
 
#define _IN_TEXTURE

#include "gpuStdafx.h"

#include "gpuDraw.h"
//#include "plugins.h"
#include "gpuExternals.h"
#include "gpuTexture.h"
#include "gpuPlugin.h"
#include "gpuPrim.h"
#include <android/log.h>
#include <sys/time.h>

#include "gfxCommand.h"
#include "gfxTexture.h"

#define TAG "ellis"
#define  LOG_TAG    "libfpse"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
extern unsigned int start,maxtime;
#if 0
#define glError() { \
  if (start==0) start=GetTicks(); \
  if (Getticks()-start>maxtime){maxtime=Getticks()-start;LOGE("Max time %s:%u    %d\n",  __FILE__, __LINE__,maxtime);} \
  start=GetTicks(); \
  	GLenum err = glGetError(); \
	while (err != GL_NO_ERROR) { \
		LOGE("glError: %d caught at %s:%u\n", err, __FILE__, __LINE__); \
		err = glGetError(); \
	} \
}
#else
#define glError() 
#endif

#define CLUTCHK   0x00060000
#define CLUTSHIFT 17

////////////////////////////////////////////////////////////////////////
// texture conversion buffer .. 
////////////////////////////////////////////////////////////////////////

u8       ubPaletteBuffer[256][4];
u32        gTexMovieName=0;
u32        gTexBlurName=0;
u32        gTexFrameName=0;
int           iTexGarbageCollection=1;
unsigned long dwTexPageComp=0;
int           iVRamSize=0;
// replace iClampType by gClampType
//int           iClampType=GL_CLAMP_TO_EDGE;
s32     gClampType = 0;
// replace iFilter by gFilter
//int iFilter = GL_NEAREST;
s32     gFilter = 0;
void               (*LoadSubTexFn) (int,int,short,short);
unsigned long      (*PalTexturedColourFn)  (unsigned long);

////////////////////////////////////////////////////////////////////////
// defines
////////////////////////////////////////////////////////////////////////

#define PALCOL(x) PalTexturedColourFn (x)

#define CSUBSIZE  2048
#define CSUBSIZEA 8192
#define CSUBSIZES 4096

#define OFFA 0
#define OFFB 2048
#define OFFC 4096
#define OFFD 6144

#define XOFFA 0
#define XOFFB 512
#define XOFFC 1024
#define XOFFD 1536

#define SOFFA 0
#define SOFFB 1024
#define SOFFC 2048
#define SOFFD 3072

#define MAXWNDTEXCACHE 128

#define XCHECK(pos1,pos2) ((pos1.c[0]>=pos2.c[1])&&(pos1.c[1]<=pos2.c[0])&&(pos1.c[2]>=pos2.c[3])&&(pos1.c[3]<=pos2.c[2]))
#define INCHECK(pos2,pos1) ((pos1.c[0]<=pos2.c[0]) && (pos1.c[1]>=pos2.c[1]) && (pos1.c[2]<=pos2.c[2]) && (pos1.c[3]>=pos2.c[3]))

////////////////////////////////////////////////////////////////////////

unsigned char * CheckTextureInSubSCache(long TextureMode,unsigned long GivenClutId,unsigned short * pCache);
void            LoadSubTexturePageSort(int pageid, int mode, short cx, short cy);
void            LoadPackedSubTexturePageSort(int pageid, int mode, short cx, short cy);
void            DefineSubTextureSort(void);

////////////////////////////////////////////////////////////////////////
// some globals
////////////////////////////////////////////////////////////////////////

long  GlobalTexturePage;
s32 XTexS;
s32 YTexS;
s32 DXTexS;
s32 DYTexS;
int   iSortTexCnt=32;
BOOL  bUseFastMdec=FALSE;
BOOL  bUse15bitMdec=FALSE;
int   iFrameTexType=0;
int   iFrameReadType=0;

unsigned long  (*TCF[2]) (unsigned long);
unsigned short (*PTCF[2]) (unsigned short);

////////////////////////////////////////////////////////////////////////
// texture cache implementation
////////////////////////////////////////////////////////////////////////





// "texture window" cache entry

typedef struct textureWndCacheEntryTag
{
 unsigned long  ClutID;
 short          pageid;
 short          textureMode;
 short          Opaque;
 short          used;
 EXLong         pos;
 u32         texname;
} textureWndCacheEntry;

// "standard texture" cache entry (12 byte per entry, as small as possible... we need lots of them)

typedef struct textureSubCacheEntryTagS 
{
 unsigned long   ClutID;
 EXLong          pos;
 unsigned char   posTX;
 unsigned char   posTY;
 unsigned char   cTexID;
 unsigned char   Opaque;
} textureSubCacheEntryS;


//---------------------------------------------

#define MAXTPAGES_MAX 64
#define MAXSORTTEX_MAX 256

//---------------------------------------------

textureWndCacheEntry     wcWndtexStore[MAXWNDTEXCACHE];
textureSubCacheEntryS *  pscSubtexStore[3][MAXTPAGES_MAX];
EXLong *                 pxSsubtexLeft [MAXSORTTEX_MAX];
u32                   uiStexturePage[MAXSORTTEX_MAX];

unsigned short           usLRUTexPage=0;

int                      iMaxTexWnds=0;
int                      iTexWndTurn=0;
int                      iTexWndLimit=MAXWNDTEXCACHE/2;

u8 *                texturepart=NULL;
u8 *                texturebuffer=NULL;
unsigned long            g_x1,g_y1,g_x2,g_y2;
unsigned char            ubOpaqueDraw=0;

unsigned short MAXTPAGES     = 32;
unsigned short CLUTMASK      = 0x7fff;
unsigned short CLUTYMASK     = 0x1ff;
unsigned short MAXSORTTEX    = 256;

////////////////////////////////////////////////////////////////////////
// Texture color conversions... all my ASM funcs are removed for easier
// porting... and honestly: nowadays the speed gain would be pointless 
////////////////////////////////////////////////////////////////////////

unsigned long XP8RGBA(unsigned long BGR)
{
 if(!(BGR&0xffff)) return 0x50000000;
 if(DrawSemiTrans && !(BGR&0x8000)) 
  {ubOpaqueDraw=1;return ((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff);}
 return ((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff)|0xff000000;
}

unsigned long XP8RGBAEx(unsigned long BGR)
{
 if(!(BGR&0xffff)) return 0x03000000;
 if(DrawSemiTrans && !(BGR&0x8000)) 
  {ubOpaqueDraw=1;return ((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff);}
 return ((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff)|0xff000000;
}

unsigned long CP8RGBA(unsigned long BGR)
{
 unsigned long l;
 if(!(BGR&0xffff)) return 0x50000000;
 if(DrawSemiTrans && !(BGR&0x8000)) 
  {ubOpaqueDraw=1;return ((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff);}
 l=((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff)|0xff000000;
 if(l==0xffffff00) l=0xff000000;
 return l;
}

unsigned long CP8RGBAEx(unsigned long BGR)
{
 unsigned long l;
 if(!(BGR&0xffff)) return 0x03000000;
 if(DrawSemiTrans && !(BGR&0x8000)) 
  {ubOpaqueDraw=1;return ((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff);}
 l=((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff)|0xff000000;
 if(l==0xffffff00) l=0xff000000;
 return l;
}

unsigned long XP8RGBA_0(unsigned long BGR)
{
 if(!(BGR&0xffff)) return 0x50000000;
 return ((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff)|0xff000000;
}

unsigned long XP8RGBAEx_0(unsigned long BGR)
{
 if(!(BGR&0xffff)) return 0x03000000;
 return ((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff)|0xff000000;
}

unsigned long XP8BGRA_0(unsigned long BGR)
{
 if(!(BGR&0xffff)) return 0x50000000;
 return ((((BGR>>7)&0xf8)|((BGR<<6)&0xf800)|((BGR<<19)&0xf80000))&0xffffff)|0xff000000;
}

unsigned long XP8BGRAEx_0(unsigned long BGR)
{
 if(!(BGR&0xffff)) return 0x03000000;
 return ((((BGR>>7)&0xf8)|((BGR<<6)&0xf800)|((BGR<<19)&0xf80000))&0xffffff)|0xff000000;
}

unsigned long CP8RGBA_0(unsigned long BGR)
{
 unsigned long l;

 if(!(BGR&0xffff)) return 0x50000000;
 l=((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff)|0xff000000;
 if(l==0xfff8f800) l=0xff000000;
 return l;
}

unsigned long CP8RGBAEx_0(unsigned long BGR)
{
 unsigned long l;

 if(!(BGR&0xffff)) return 0x03000000;
 l=((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff)|0xff000000;
 if(l==0xfff8f800) l=0xff000000;
 return l;
}

unsigned long CP8BGRA_0(unsigned long BGR)
{
 unsigned long l;

 if(!(BGR&0xffff)) return 0x50000000;
 l=((((BGR>>7)&0xf8)|((BGR<<6)&0xf800)|((BGR<<19)&0xf80000))&0xffffff)|0xff000000;
 if(l==0xff00f8f8) l=0xff000000;
 return l;
}

unsigned long CP8BGRAEx_0(unsigned long BGR)
{
 unsigned long l;

 if(!(BGR&0xffff)) return 0x03000000;
 l=((((BGR>>7)&0xf8)|((BGR<<6)&0xf800)|((BGR<<19)&0xf80000))&0xffffff)|0xff000000;
 if(l==0xff00f8f8) l=0xff000000;
 return l;
}

unsigned long XP8RGBA_1(unsigned long BGR)
{
 if(!(BGR&0xffff)) return 0x50000000;
 if(!(BGR&0x8000)) {ubOpaqueDraw=1;return ((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff);}
 return ((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff)|0xff000000;
}

unsigned long XP8RGBAEx_1(unsigned long BGR)
{
 if(!(BGR&0xffff)) return 0x03000000;
 if(!(BGR&0x8000)) {ubOpaqueDraw=1;return ((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff);}
 return ((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff)|0xff000000;
}

unsigned long XP8BGRA_1(unsigned long BGR)
{
 if(!(BGR&0xffff)) return 0x50000000;
 if(!(BGR&0x8000)) {ubOpaqueDraw=1;return ((((BGR>>7)&0xf8)|((BGR<<6)&0xf800)|((BGR<<19)&0xf80000))&0xffffff);}
 return ((((BGR>>7)&0xf8)|((BGR<<6)&0xf800)|((BGR<<19)&0xf80000))&0xffffff)|0xff000000;
}

unsigned long XP8BGRAEx_1(unsigned long BGR)
{
 if(!(BGR&0xffff)) return 0x03000000;
 if(!(BGR&0x8000)) {ubOpaqueDraw=1;return ((((BGR>>7)&0xf8)|((BGR<<6)&0xf800)|((BGR<<19)&0xf80000))&0xffffff);}
 return ((((BGR>>7)&0xf8)|((BGR<<6)&0xf800)|((BGR<<19)&0xf80000))&0xffffff)|0xff000000;
}

unsigned long P8RGBA(unsigned long BGR)
{
 if(!(BGR&0xffff)) return 0;
 return ((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff)|0xff000000;
}

unsigned long P8BGRA(unsigned long BGR)
{
 if(!(BGR&0xffff)) return 0;
 return ((((BGR>>7)&0xf8)|((BGR<<6)&0xf800)|((BGR<<19)&0xf80000))&0xffffff)|0xff000000;
}

unsigned short XP5RGBA(unsigned short BGR)
{
 if(!BGR) return 0;
 if(DrawSemiTrans && !(BGR&0x8000)) 
  {ubOpaqueDraw=1;return ((((BGR<<11))|((BGR>>9)&0x3e)|((BGR<<1)&0x7c0)));}
 return ((((BGR<<11))|((BGR>>9)&0x3e)|((BGR<<1)&0x7c0)))|1;
}

unsigned short XP5RGBA_0 (unsigned short BGR)
{
 if(!BGR) return 0;

 return ((((BGR<<11))|((BGR>>9)&0x3e)|((BGR<<1)&0x7c0)))|1;
}

unsigned short CP5RGBA_0 (unsigned short BGR)
{
 unsigned short s;

 if(!BGR) return 0;

 s=((((BGR<<11))|((BGR>>9)&0x3e)|((BGR<<1)&0x7c0)))|1;
 if(s==0x07ff) s=1;
 return s;
}

unsigned short XP5RGBA_1(unsigned short BGR)
{
 if(!BGR) return 0;
 if(!(BGR&0x8000)) 
  {ubOpaqueDraw=1;return ((((BGR<<11))|((BGR>>9)&0x3e)|((BGR<<1)&0x7c0)));}
 return ((((BGR<<11))|((BGR>>9)&0x3e)|((BGR<<1)&0x7c0)))|1;
}

unsigned short P5RGBA(unsigned short BGR)
{
 if(!BGR) return 0;
 return ((((BGR<<11))|((BGR>>9)&0x3e)|((BGR<<1)&0x7c0)))|1;
}

unsigned short XP4RGBA(unsigned short BGR)
{
 if(!BGR) return 6;
 if(DrawSemiTrans && !(BGR&0x8000)) 
  {ubOpaqueDraw=1;return ((((BGR<<11))|((BGR>>9)&0x3e)|((BGR<<1)&0x7c0)));}
 return (((((BGR&0x1e)<<11))|((BGR&0x7800)>>7)|((BGR&0x3c0)<<2)))|0xf;
}

unsigned short XP4RGBA_0 (unsigned short BGR)
{
 if(!BGR) return 6;
 return (((((BGR&0x1e)<<11))|((BGR&0x7800)>>7)|((BGR&0x3c0)<<2)))|0xf;
}

unsigned short CP4RGBA_0 (unsigned short BGR)
{
 unsigned short s;
 if(!BGR) return 6;
 s=(((((BGR&0x1e)<<11))|((BGR&0x7800)>>7)|((BGR&0x3c0)<<2)))|0xf;
 if(s==0x0fff) s=0x000f;
 return s;
}

unsigned short XP4RGBA_1(unsigned short BGR)
{
 if(!BGR) return 6;
 if(!(BGR&0x8000)) 
  {ubOpaqueDraw=1;return ((((BGR<<11))|((BGR>>9)&0x3e)|((BGR<<1)&0x7c0)));}
 return (((((BGR&0x1e)<<11))|((BGR&0x7800)>>7)|((BGR&0x3c0)<<2)))|0xf;
}

unsigned short P4RGBA(unsigned short BGR)
{
 if(!BGR) return 0;
 return (((((BGR&0x1e)<<11))|((BGR&0x7800)>>7)|((BGR&0x3c0)<<2)))|0xf;
}

////////////////////////////////////////////////////////////////////////
// CHECK TEXTURE MEM (on plugin startup)
////////////////////////////////////////////////////////////////////////

int iFTexA=512;
int iFTexB=512;

void CheckTextureMemory(void)
{
 iSortTexCnt=192;
 __android_log_print(ANDROID_LOG_ERROR,"iSortTexCnt","iSortTexCnt:%d",iSortTexCnt);
} 

////////////////////////////////////////////////////////////////////////
// Main init of textures
////////////////////////////////////////////////////////////////////////

void InitializeTextureStore() 
{
    logInfo(TAG, "InitializeTextureStore");
    int i,j;

 if(iGPUHeight==1024)
  {
   MAXTPAGES     = 64;
   CLUTMASK      = 0xffff;
   CLUTYMASK     = 0x3ff;
   MAXSORTTEX    = 128;
   iTexGarbageCollection=0;
  }
 else
  {
   MAXTPAGES     = 32;
   CLUTMASK      = 0x7fff;
   CLUTYMASK     = 0x1ff;
   MAXSORTTEX    = 256;
  }

 memset(vertex,0,4*sizeof(OGLVertex));                 // init vertices

 gTexName=0;                                           // init main tex name

 iTexWndLimit=MAXWNDTEXCACHE;
 iTexWndLimit/=2;

 memset(wcWndtexStore,0,sizeof(textureWndCacheEntry)*
                        MAXWNDTEXCACHE);
 texturepart=(u8 *)malloc(256*256*4);
 memset(texturepart,0,256*256*4);
	 texturebuffer=NULL;

 for(i=0;i<3;i++)                                    // -> info for 32*3
  for(j=0;j<MAXTPAGES;j++)
   {                                               
    pscSubtexStore[i][j]=(textureSubCacheEntryS *)malloc(CSUBSIZES*sizeof(textureSubCacheEntryS));
    memset(pscSubtexStore[i][j],0,CSUBSIZES*sizeof(textureSubCacheEntryS));
   }
 for(i=0;i<MAXSORTTEX;i++)                           // -> info 0..511
  {
   pxSsubtexLeft[i]=(EXLong *)malloc(CSUBSIZE*sizeof(EXLong));
   memset(pxSsubtexLeft[i],0,CSUBSIZE*sizeof(EXLong));
   uiStexturePage[i]=0;
  }
}

////////////////////////////////////////////////////////////////////////
// Clean up on exit
////////////////////////////////////////////////////////////////////////

void CleanupTextureStore() 
{
 int i,j;textureWndCacheEntry * tsx;
 //----------------------------------------------------//
    bindTexture(0);
 //----------------------------------------------------//
 free(texturepart);                                    // free tex part
 texturepart=0;
 if(texturebuffer)
  {
   free(texturebuffer);
   texturebuffer=0;
  }
 //----------------------------------------------------//
    tsx=wcWndtexStore;                                    // loop tex window cache
    for(i=0;i<MAXWNDTEXCACHE;i++,tsx++) {
        if(tsx->texname) {                                // -> some tex?
            destroyTexture(tsx->texname);                 // --> delete it
        }
    }
    iMaxTexWnds=0;                                        // no more tex wnds
    //----------------------------------------------------//
    if(gTexMovieName!=0) {                                // some movie tex?
        destroyTexture(gTexMovieName);                 // -> delete it
    }
    gTexMovieName=0;                                      // no more movie tex
    //----------------------------------------------------//
    if(gTexFrameName!=0) {                                 // some 15bit framebuffer tex?
        destroyTexture(gTexFrameName);                 // -> delete it
    }
    gTexFrameName=0;                                      // no more movie tex
    //----------------------------------------------------//
    if(gTexBlurName!=0) {                                  // some 15bit framebuffer tex?
        destroyTexture(gTexBlurName);                  // -> delete it
    }
    gTexBlurName=0;                                       // no more movie tex
    //----------------------------------------------------//
    for(i=0;i<3;i++) {                                    // -> loop
        for(j=0;j<MAXTPAGES;j++) {                         // loop tex pages
            free(pscSubtexStore[i][j]);                      // -> clean mem
        }
    }
    for(i=0;i<MAXSORTTEX;i++)
    {
        if(uiStexturePage[i])                             // --> tex used ?
        {
            destroyTexture(uiStexturePage[i]);
            uiStexturePage[i]=0;                            // --> delete it
        }
        free(pxSsubtexLeft[i]);                           // -> clean mem
    }
 //----------------------------------------------------//
}

////////////////////////////////////////////////////////////////////////
// Reset textures in game...
////////////////////////////////////////////////////////////////////////

void ResetTextureArea(BOOL bDelTex)
{
 int i,j;textureSubCacheEntryS * tss;EXLong * lu;
 textureWndCacheEntry * tsx;
 //----------------------------------------------------//

 dwTexPageComp=0;

 //----------------------------------------------------//
    if(bDelTex) {
        bindTexture(0);
        gTexName=0;
    }
 //----------------------------------------------------//
 tsx=wcWndtexStore;
 for(i=0;i<MAXWNDTEXCACHE;i++,tsx++)
  {
   tsx->used=0;
   if(bDelTex && tsx->texname)
    {
     destroyTexture(tsx->texname);
     tsx->texname=0;
    }
  }
 iMaxTexWnds=0;
 //----------------------------------------------------//

 for(i=0;i<3;i++)
  for(j=0;j<MAXTPAGES;j++)
   {
    tss=pscSubtexStore[i][j];
    (tss+SOFFA)->pos.l=0;
    (tss+SOFFB)->pos.l=0;
    (tss+SOFFC)->pos.l=0;
    (tss+SOFFD)->pos.l=0;
   }

 for(i=0;i<iSortTexCnt;i++)
  {
   lu=pxSsubtexLeft[i];
   lu->l=0;
    if(bDelTex && uiStexturePage[i]) {
        destroyTexture(uiStexturePage[i]);
        uiStexturePage[i]=0;
        }
  }
}


////////////////////////////////////////////////////////////////////////
// Invalidate tex windows
////////////////////////////////////////////////////////////////////////

void InvalidateWndTextureArea(long X,long Y,long W, long H)
{
 int i,px1,px2,py1,py2,iYM=1;
 textureWndCacheEntry * tsw=wcWndtexStore;

 W+=X-1;      
 H+=Y-1;
 if(X<0) X=0;if(X>1023) X=1023;
 if(W<0) W=0;if(W>1023) W=1023;
 if(Y<0) Y=0;if(Y>iGPUHeightMask)  Y=iGPUHeightMask;
 if(H<0) H=0;if(H>iGPUHeightMask)  H=iGPUHeightMask;
 W++;H++;

 if(iGPUHeight==1024) iYM=3;

 py1=min(iYM,Y>>8);
 py2=min(iYM,H>>8);                                    // y: 0 or 1

 px1=max(0,(X>>6));
 px2=min(15,(W>>6));

 if(py1==py2)
  {
   py1=py1<<4;px1+=py1;px2+=py1;                       // change to 0-31
   for(i=0;i<iMaxTexWnds;i++,tsw++)
    {
     if(tsw->used)
      {
       if(tsw->pageid>=px1 && tsw->pageid<=px2)
        {
         tsw->used=0;
        }
      }
    }
  }
 else
  {
   py1=px1+16;py2=px2+16;
   for(i=0;i<iMaxTexWnds;i++,tsw++)
    {
     if(tsw->used)
      {
       if((tsw->pageid>=px1 && tsw->pageid<=px2) ||
          (tsw->pageid>=py1 && tsw->pageid<=py2))
        {
         tsw->used=0;
        }
      }
    }
  }

 // adjust tex window count
 tsw=wcWndtexStore+iMaxTexWnds-1;
 while(iMaxTexWnds && !tsw->used) {iMaxTexWnds--;tsw--;}
}



////////////////////////////////////////////////////////////////////////
// same for sort textures
////////////////////////////////////////////////////////////////////////

void MarkFree(textureSubCacheEntryS * tsx)
{
 EXLong * ul, * uls;
 int j,iMax;unsigned char x1,y1,dx,dy;

 uls=pxSsubtexLeft[tsx->cTexID];
 iMax=uls->l;ul=uls+1;

 if(!iMax) return;

 for(j=0;j<iMax;j++,ul++)
  if(ul->l==0xffffffff) break;

 if(j<CSUBSIZE-2)
  {
   if(j==iMax) uls->l=uls->l+1;

   x1=tsx->posTX;dx=tsx->pos.c[2]-tsx->pos.c[3];
   if(tsx->posTX) {x1--;dx+=3;}
   y1=tsx->posTY;dy=tsx->pos.c[0]-tsx->pos.c[1];
   if(tsx->posTY) {y1--;dy+=3;}

   ul->c[3]=x1;
   ul->c[2]=dx;
   ul->c[1]=y1;
   ul->c[0]=dy;
  }
}

void InvalidateSubSTextureArea(long X,long Y,long W, long H)
{
 int i,j,k,iMax,px,py,px1,px2,py1,py2,iYM=1;
 EXLong npos;textureSubCacheEntryS * tsb;
 long x1,x2,y1,y2,xa,sw;

 W+=X-1;      
 H+=Y-1;
 if(X<0) X=0;if(X>1023) X=1023;
 if(W<0) W=0;if(W>1023) W=1023;
 if(Y<0) Y=0;if(Y>iGPUHeightMask)  Y=iGPUHeightMask;
 if(H<0) H=0;if(H>iGPUHeightMask)  H=iGPUHeightMask;
 W++;H++;

 if(iGPUHeight==1024) iYM=3;

 py1=min(iYM,Y>>8);
 py2=min(iYM,H>>8);                                    // y: 0 or 1
 px1=max(0,(X>>6)-3);                                   
 px2=min(15,(W>>6)+3);                                 // x: 0-15

 for(py=py1;py<=py2;py++)
  {
   j=(py<<4)+px1;                                      // get page

   y1=py*256;y2=y1+255;

   if(H<y1)  continue;
   if(Y>y2)  continue;

   if(Y>y1)  y1=Y;
   if(H<y2)  y2=H;
   if(y2<y1) {sw=y1;y1=y2;y2=sw;}
   y1=((y1%256)<<8);
   y2=(y2%256);

   for(px=px1;px<=px2;px++,j++)
    {
     for(k=0;k<3;k++)
      {
       xa=x1=px<<6;
       if(W<x1) continue;
       x2=x1+(64<<k)-1;
       if(X>x2) continue;

       if(X>x1)  x1=X;
       if(W<x2)  x2=W;
       if(x2<x1) {sw=x1;x1=x2;x2=sw;}

       if (dwGPUVersion == 2)
        npos.l=0x00ff00ff;
       else
        npos.l=((x1-xa)<<(26-k))|((x2-xa)<<(18-k))|y1|y2;

        {
         tsb=pscSubtexStore[k][j]+SOFFA;iMax=tsb->pos.l;tsb++;
         for(i=0;i<iMax;i++,tsb++)
          if(tsb->ClutID && XCHECK(tsb->pos,npos)) {tsb->ClutID=0;MarkFree(tsb);}

//         if(npos.l & 0x00800000)
          {
           tsb=pscSubtexStore[k][j]+SOFFB;iMax=tsb->pos.l;tsb++;
           for(i=0;i<iMax;i++,tsb++)
            if(tsb->ClutID && XCHECK(tsb->pos,npos)) {tsb->ClutID=0;MarkFree(tsb);}
          }

//         if(npos.l & 0x00000080)
          {
           tsb=pscSubtexStore[k][j]+SOFFC;iMax=tsb->pos.l;tsb++;
           for(i=0;i<iMax;i++,tsb++)
            if(tsb->ClutID && XCHECK(tsb->pos,npos)) {tsb->ClutID=0;MarkFree(tsb);}
          }

//         if(npos.l & 0x00800080)
          {
           tsb=pscSubtexStore[k][j]+SOFFD;iMax=tsb->pos.l;tsb++;
           for(i=0;i<iMax;i++,tsb++)
            if(tsb->ClutID && XCHECK(tsb->pos,npos)) {tsb->ClutID=0;MarkFree(tsb);}
          }
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////
// Invalidate some parts of cache: main routine
////////////////////////////////////////////////////////////////////////

void InvalidateTextureAreaEx(void)
{
 short W=sxmax-sxmin;
 short H=symax-symin;

 if(W==0 && H==0) return;

 if(iMaxTexWnds) 
  InvalidateWndTextureArea(sxmin,symin,W,H);

 InvalidateSubSTextureArea(sxmin,symin,W,H);
}

////////////////////////////////////////////////////////////////////////

void InvalidateTextureArea(long X,long Y,long W, long H)
{
 if(W==0 && H==0) return;

 if(iMaxTexWnds) InvalidateWndTextureArea(X,Y,W,H); 

 InvalidateSubSTextureArea(X,Y,W,H);
}


////////////////////////////////////////////////////////////////////////
// tex window: define
////////////////////////////////////////////////////////////////////////

void DefineTextureWnd(void)
{
    if(gTexName==0) {
        s8 clampType = gFilter;
        gTexName = createTexture(1, clampType);
        
    }

    //--bindTexture(gTexName);
    setTexture(gTexName, TWin.Position.x1, TWin.Position.y1, 3, texturepart);

//LOGE("DefineTextureWnd x:%d y:%d",TWin.Position.x1,TWin.Position.y1);

}

////////////////////////////////////////////////////////////////////////
// tex window: load packed stretch
////////////////////////////////////////////////////////////////////////

void LoadStretchPackedWndTexturePage(int pageid, int mode, short cx, short cy)
{
 unsigned long start,row,column,j,sxh,sxm,ldx,ldy,ldxo;
 unsigned int   palstart;
 unsigned short *px,*pa,*ta;
 unsigned char  *cSRCPtr,*cOSRCPtr;
 unsigned short *wSRCPtr,*wOSRCPtr;
 unsigned long  LineOffset;unsigned short s;
 int pmult=pageid/16;
 unsigned short (*LPTCOL)(unsigned short);

 LPTCOL=PTCF[DrawSemiTrans];

 ldxo=TWin.Position.x1-TWin.OPosition.x1;
 ldy =TWin.Position.y1-TWin.OPosition.y1;

 pa=px=(unsigned short *)ubPaletteBuffer;
 ta=(unsigned short *)texturepart;
 palstart=cx+(cy*1024);

 ubOpaqueDraw=0;
//LOGE("LoadStretchPackedWndTexturePage bits:%d x:%d y:%d",mode,cx,cy);
 switch(mode)
  {
   //--------------------------------------------------// 
   // 4bit texture load ..
   case 0:
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;
      for(row=0;row<16;row++)
       *px++=LPTCOL(*wSRCPtr++);

      column=g_y2-ldy;
      for(TXV=g_y1;TXV<=column;TXV++)
       {
        ldx=ldxo;
        for(TXU=g_x1;TXU<=g_x2-ldxo;TXU++)
         {
  		  n_xi = ( ( TXU >> 2 ) & ~0x3c ) + ( ( TXV << 2 ) & 0x3c );
		  n_yi = ( TXV & ~0xf ) + ( ( TXU >> 4 ) & 0xf );

          s=*(pa+((*( psxVuw + ((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi ) >> ( ( TXU & 0x03 ) << 2 ) ) & 0x0f ));
          *ta++=s;

          if(ldx) {*ta++=s;ldx--;}
         }

        if(ldy) 
         {ldy--;
          for(TXU=g_x1;TXU<=g_x2;TXU++) {
           *ta=*(ta-(g_x2-g_x1));
           ta++;
           }
         }
       }

      DefineTextureWnd();

      break;
     }


    start=((pageid-16*pmult)*128)+256*2048*pmult;

    // convert CLUT to 32bits .. and then use THAT as a lookup table

    wSRCPtr=psxVuw+palstart;
    for(row=0;row<16;row++)
     *px++=LPTCOL(*wSRCPtr++);

    sxm=g_x1&1;sxh=g_x1>>1;
    if(sxm) j=g_x1+1; else j=g_x1;
    cSRCPtr = psxVub + start + (2048*g_y1) + sxh;
    for(column=g_y1;column<=g_y2;column++)
     {
      cOSRCPtr=cSRCPtr;ldx=ldxo;
      if(sxm) *ta++=*(pa+((*cSRCPtr++ >> 4) & 0xF));
      
      for(row=j;row<=g_x2-ldxo;row++)
       {
        s=*(pa+(*cSRCPtr & 0xF));
        *ta++=s;
        if(ldx) {*ta++=s;ldx--;}
        row++;
        if(row<=g_x2-ldxo) 
         {
          s=*(pa+((*cSRCPtr >> 4) & 0xF));
          *ta++=s; 
          if(ldx) {*ta++=s;ldx--;}
         }
        cSRCPtr++;
       }

      if(ldy && column&1) 
           {ldy--;cSRCPtr = cOSRCPtr;}
      else cSRCPtr = psxVub + start + (2048*(column+1)) + sxh;
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------// 
   // 8bit texture load ..
   case 1:
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;
      for(row=0;row<256;row++)
       *px++=LPTCOL(*wSRCPtr++);

      column=g_y2-ldy;
      for(TXV=g_y1;TXV<=column;TXV++)
       {
        ldx=ldxo;
        for(TXU=g_x1;TXU<=g_x2-ldxo;TXU++)
         {
  		  n_xi = ( ( TXU >> 1 ) & ~0x78 ) + ( ( TXU << 2 ) & 0x40 ) + ( ( TXV << 3 ) & 0x38 );
		  n_yi = ( TXV & ~0x7 ) + ( ( TXU >> 5 ) & 0x7 );

          s=*(pa+((*( psxVuw + ((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi ) >> ( ( TXU & 0x01 ) << 3 ) ) & 0xff));

          *ta++=s;
          if(ldx) {*ta++=s;ldx--;}
         }

        if(ldy) 
         {ldy--;
          for(TXU=g_x1;TXU<=g_x2;TXU++) {
           *ta=*(ta-(g_x2-g_x1));
           ta++;
           }
         }

       }

      DefineTextureWnd();

      break;
     }

    start=((pageid-16*pmult)*128)+256*2048*pmult;

    // not using a lookup table here... speeds up smaller texture areas
    cSRCPtr = psxVub + start + (2048*g_y1) + g_x1;
    LineOffset = 2048 - (g_x2-g_x1+1) +ldxo; 

    for(column=g_y1;column<=g_y2;column++)
     {
      cOSRCPtr=cSRCPtr;ldx=ldxo;
      for(row=g_x1;row<=g_x2-ldxo;row++)
       {
        s=LPTCOL(psxVuw[palstart+ *cSRCPtr++]);
        *ta++=s;
        if(ldx) {*ta++=s;ldx--;}
       }
      if(ldy && column&1) {ldy--;cSRCPtr=cOSRCPtr;}
      else                cSRCPtr+=LineOffset;
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------// 
   // 16bit texture load ..
   case 2:
    start=((pageid-16*pmult)*64)+256*1024*pmult;
    wSRCPtr = psxVuw + start + (1024*g_y1) + g_x1;
    LineOffset = 1024 - (g_x2-g_x1+1) +ldxo; 
                                
    for(column=g_y1;column<=g_y2;column++)
     {
      wOSRCPtr=wSRCPtr;ldx=ldxo;
      for(row=g_x1;row<=g_x2-ldxo;row++)
       {
        s=LPTCOL(*wSRCPtr++);
        *ta++=s;
        if(ldx) {*ta++=s;ldx--;}
       }
      if(ldy && column&1) {ldy--;wSRCPtr=wOSRCPtr;}
      else                 wSRCPtr+=LineOffset;
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------// 
   // others are not possible !
  }
}

////////////////////////////////////////////////////////////////////////
// tex window: load stretched
////////////////////////////////////////////////////////////////////////

void LoadStretchWndTexturePage(int pageid, int mode, short cx, short cy)
{
 unsigned long start,row,column,j,sxh,sxm,ldx,ldy,ldxo,s;
 unsigned int   palstart;
 unsigned long  *px,*pa,*ta;
 unsigned char  *cSRCPtr,*cOSRCPtr;
 unsigned short *wSRCPtr,*wOSRCPtr;
 unsigned long  LineOffset;
 int pmult=pageid/16;
 unsigned long (*LTCOL)(unsigned long);
 
 LTCOL=TCF[DrawSemiTrans];

 ldxo=TWin.Position.x1-TWin.OPosition.x1;
 ldy =TWin.Position.y1-TWin.OPosition.y1;

 pa=px=(unsigned long *)ubPaletteBuffer;
 ta=(unsigned long *)texturepart;
 palstart=cx+(cy*1024);

 ubOpaqueDraw=0;
//LOGE("LoadStretchWndTexturePage bits:%d x:%d y:%d",mode,cx,cy);

 switch(mode)
  {
   //--------------------------------------------------// 
   // 4bit texture load ..
   case 0:
    //------------------- ZN STUFF

    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;

      row=4;do
       {
        *px    =LTCOL(*wSRCPtr);
        *(px+1)=LTCOL(*(wSRCPtr+1));
        *(px+2)=LTCOL(*(wSRCPtr+2));
        *(px+3)=LTCOL(*(wSRCPtr+3));
        row--;px+=4;wSRCPtr+=4;
       }
      while (row);

      column=g_y2-ldy;
      for(TXV=g_y1;TXV<=column;TXV++)
       {
        ldx=ldxo;
        for(TXU=g_x1;TXU<=g_x2-ldxo;TXU++)
         {
  		  n_xi = ( ( TXU >> 2 ) & ~0x3c ) + ( ( TXV << 2 ) & 0x3c );
		  n_yi = ( TXV & ~0xf ) + ( ( TXU >> 4 ) & 0xf );

          s=*(pa+((*( psxVuw + ((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi ) >> ( ( TXU & 0x03 ) << 2 ) ) & 0x0f ));
          *ta++=s;

          if(ldx) {*ta++=s;ldx--;}
         }

        if(ldy) 
         {ldy--;
          for(TXU=g_x1;TXU<=g_x2;TXU++) {
           *ta=*(ta-(g_x2-g_x1));
           ta++;
           }
         }
       }

      DefineTextureWnd();

      break;
     }

    //-------------------

    start=((pageid-16*pmult)*128)+256*2048*pmult;
    // convert CLUT to 32bits .. and then use THAT as a lookup table

    wSRCPtr=psxVuw+palstart;
    for(row=0;row<16;row++)
     *px++=LTCOL(*wSRCPtr++);

    sxm=g_x1&1;sxh=g_x1>>1;
    if(sxm) j=g_x1+1; else j=g_x1;
    cSRCPtr = psxVub + start + (2048*g_y1) + sxh;
    for(column=g_y1;column<=g_y2;column++)
     {
      cOSRCPtr=cSRCPtr;ldx=ldxo;
      if(sxm) *ta++=*(pa+((*cSRCPtr++ >> 4) & 0xF));
      
      for(row=j;row<=g_x2-ldxo;row++)
       {
        s=*(pa+(*cSRCPtr & 0xF));
        *ta++=s;
        if(ldx) {*ta++=s;ldx--;}
        row++;
        if(row<=g_x2-ldxo) 
         {
          s=*(pa+((*cSRCPtr >> 4) & 0xF));
          *ta++=s; 
          if(ldx) {*ta++=s;ldx--;}
         }
        cSRCPtr++;
       }
      if(ldy && column&1) 
           {ldy--;cSRCPtr = cOSRCPtr;}
      else cSRCPtr = psxVub + start + (2048*(column+1)) + sxh;
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------//
   // 8bit texture load ..
   case 1:
    //------------ ZN STUFF
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;

      row=64;do
       {
        *px    =LTCOL(*wSRCPtr);
        *(px+1)=LTCOL(*(wSRCPtr+1));
        *(px+2)=LTCOL(*(wSRCPtr+2));
        *(px+3)=LTCOL(*(wSRCPtr+3));
        row--;px+=4;wSRCPtr+=4;
       }
      while (row);

      column=g_y2-ldy;
      for(TXV=g_y1;TXV<=column;TXV++)
       {
        ldx=ldxo;
        for(TXU=g_x1;TXU<=g_x2-ldxo;TXU++)
         {
  		  n_xi = ( ( TXU >> 1 ) & ~0x78 ) + ( ( TXU << 2 ) & 0x40 ) + ( ( TXV << 3 ) & 0x38 );
		  n_yi = ( TXV & ~0x7 ) + ( ( TXU >> 5 ) & 0x7 );

          s=*(pa+((*( psxVuw + ((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi ) >> ( ( TXU & 0x01 ) << 3 ) ) & 0xff));
          *ta++=s;
          if(ldx) {*ta++=s;ldx--;}
         }

        if(ldy) 
         {ldy--;
          for(TXU=g_x1;TXU<=g_x2;TXU++) {
           *ta=*(ta-(g_x2-g_x1));
           ta++;
           }
         }

       }

      DefineTextureWnd();

      break;
     }
    //------------

    start=((pageid-16*pmult)*128)+256*2048*pmult;    

    // not using a lookup table here... speeds up smaller texture areas
    cSRCPtr = psxVub + start + (2048*g_y1) + g_x1;
    LineOffset = 2048 - (g_x2-g_x1+1) +ldxo; 

    for(column=g_y1;column<=g_y2;column++)
     {
      cOSRCPtr=cSRCPtr;ldx=ldxo;
      for(row=g_x1;row<=g_x2-ldxo;row++)
       {
        s=LTCOL(psxVuw[palstart+ *cSRCPtr++]);
        *ta++=s;
        if(ldx) {*ta++=s;ldx--;}
       }
      if(ldy && column&1) {ldy--;cSRCPtr=cOSRCPtr;}
      else                cSRCPtr+=LineOffset;
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------// 
   // 16bit texture load ..
   case 2:
    start=((pageid-16*pmult)*64)+256*1024*pmult;

    wSRCPtr = psxVuw + start + (1024*g_y1) + g_x1;
    LineOffset = 1024 - (g_x2-g_x1+1) +ldxo; 

    for(column=g_y1;column<=g_y2;column++)
     {
      wOSRCPtr=wSRCPtr;ldx=ldxo;
      for(row=g_x1;row<=g_x2-ldxo;row++)
       {
        s=LTCOL(*wSRCPtr++);
        *ta++=s;
        if(ldx) {*ta++=s;ldx--;}
       }
      if(ldy && column&1) {ldy--;wSRCPtr=wOSRCPtr;}
      else                 wSRCPtr+=LineOffset;
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------// 
   // others are not possible !
  }
}

////////////////////////////////////////////////////////////////////////
// tex window: load packed simple
////////////////////////////////////////////////////////////////////////

void LoadPackedWndTexturePage(int pageid, int mode, short cx, short cy)
{
 unsigned long start,row,column,j,sxh,sxm;
 unsigned int   palstart;
 unsigned short *px,*pa,*ta;
 unsigned char  *cSRCPtr;
 unsigned short *wSRCPtr;
 unsigned long  LineOffset;
 int pmult=pageid/16;
 unsigned short (*LPTCOL)(unsigned short);

 LPTCOL=PTCF[DrawSemiTrans];

 pa=px=(unsigned short *)ubPaletteBuffer;
 ta=(unsigned short *)texturepart;
 palstart=cx+(cy*1024);

 ubOpaqueDraw=0;
//LOGE("LoadPackedWndTexturePage bits:%d x:%d y:%d",mode,cx,cy);
 switch(mode)
  {
   //--------------------------------------------------// 
   // 4bit texture load ..
   case 0:
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;
      for(row=0;row<16;row++)
       *px++=LPTCOL(*wSRCPtr++);

      for(TXV=g_y1;TXV<=g_y2;TXV++)
       {
        for(TXU=g_x1;TXU<=g_x2;TXU++)
         {
  		  n_xi = ( ( TXU >> 2 ) & ~0x3c ) + ( ( TXV << 2 ) & 0x3c );
		  n_yi = ( TXV & ~0xf ) + ( ( TXU >> 4 ) & 0xf );

          *ta++=*(pa+((*( psxVuw + ((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi ) >> ( ( TXU & 0x03 ) << 2 ) ) & 0x0f ));
         }
       }

      DefineTextureWnd();

      break;
     }

    start=((pageid-16*pmult)*128)+256*2048*pmult;

    // convert CLUT to 32bits .. and then use THAT as a lookup table

    wSRCPtr=psxVuw+palstart;
    for(row=0;row<16;row++)
     *px++=LPTCOL(*wSRCPtr++);

    sxm=g_x1&1;sxh=g_x1>>1;
    if(sxm) j=g_x1+1; else j=g_x1;
    cSRCPtr = psxVub + start + (2048*g_y1) + sxh;
    for(column=g_y1;column<=g_y2;column++)
     {
      cSRCPtr = psxVub + start + (2048*column) + sxh;
    
      if(sxm) *ta++=*(pa+((*cSRCPtr++ >> 4) & 0xF));
      
      for(row=j;row<=g_x2;row++)
       {
        *ta++=*(pa+(*cSRCPtr & 0xF)); row++;
        if(row<=g_x2) *ta++=*(pa+((*cSRCPtr >> 4) & 0xF)); 
        cSRCPtr++;
       }
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------// 
   // 8bit texture load ..
   case 1:
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;
      for(row=0;row<256;row++)
       *px++=LPTCOL(*wSRCPtr++);

      for(TXV=g_y1;TXV<=g_y2;TXV++)
       {
        for(TXU=g_x1;TXU<=g_x2;TXU++)
         {
  		  n_xi = ( ( TXU >> 1 ) & ~0x78 ) + ( ( TXU << 2 ) & 0x40 ) + ( ( TXV << 3 ) & 0x38 );
		  n_yi = ( TXV & ~0x7 ) + ( ( TXU >> 5 ) & 0x7 );

          *ta++=*(pa+((*( psxVuw + ((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi ) >> ( ( TXU & 0x01 ) << 3 ) ) & 0xff));
         }
       }

      DefineTextureWnd();

      break;
     }

    start=((pageid-16*pmult)*128)+256*2048*pmult;

    // not using a lookup table here... speeds up smaller texture areas
    cSRCPtr = psxVub + start + (2048*g_y1) + g_x1;
    LineOffset = 2048 - (g_x2-g_x1+1); 

    for(column=g_y1;column<=g_y2;column++)
     {
      for(row=g_x1;row<=g_x2;row++)
       *ta++=LPTCOL(psxVuw[palstart+ *cSRCPtr++]);
      cSRCPtr+=LineOffset;
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------// 
   // 16bit texture load ..
   case 2:
    start=((pageid-16*pmult)*64)+256*1024*pmult;
    wSRCPtr = psxVuw + start + (1024*g_y1) + g_x1;
    LineOffset = 1024 - (g_x2-g_x1+1); 

    for(column=g_y1;column<=g_y2;column++)
     {
      for(row=g_x1;row<=g_x2;row++)
       *ta++=LPTCOL(*wSRCPtr++);
      wSRCPtr+=LineOffset;
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------// 
   // others are not possible !
  }
}

////////////////////////////////////////////////////////////////////////
// tex window: load simple
////////////////////////////////////////////////////////////////////////

void LoadWndTexturePage(int pageid, int mode, short cx, short cy)
{
 unsigned long start,row,column,j,sxh,sxm;
 unsigned int   palstart;
 unsigned long  *px,*pa,*ta;
 unsigned char  *cSRCPtr;
 unsigned short *wSRCPtr;
 unsigned long  LineOffset;
 int pmult=pageid/16;
 unsigned long (*LTCOL)(unsigned long);
 
 LTCOL=TCF[DrawSemiTrans];

 pa=px=(unsigned long *)ubPaletteBuffer;
 ta=(unsigned long *)texturepart;
 palstart=cx+(cy*1024);

 ubOpaqueDraw=0;
//LOGE("LoadWndTexturePage bits:%d x:%d y:%d w:%d h:%d",mode,cx,cy,g_x2-g_x1,g_y2-g_y1);
 switch(mode)
  {
   //--------------------------------------------------// 
   // 4bit texture load ..
   case 0:
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;

      row=4;do
       {
        *px    =LTCOL(*wSRCPtr);
        *(px+1)=LTCOL(*(wSRCPtr+1));
        *(px+2)=LTCOL(*(wSRCPtr+2));
        *(px+3)=LTCOL(*(wSRCPtr+3));
        row--;px+=4;wSRCPtr+=4;
       }
      while (row);

      for(TXV=g_y1;TXV<=g_y2;TXV++)
       {
        for(TXU=g_x1;TXU<=g_x2;TXU++)
         {
  		  n_xi = ( ( TXU >> 2 ) & ~0x3c ) + ( ( TXV << 2 ) & 0x3c );
		  n_yi = ( TXV & ~0xf ) + ( ( TXU >> 4 ) & 0xf );

          *ta++=*(pa+((*( psxVuw + ((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi ) >> ( ( TXU & 0x03 ) << 2 ) ) & 0x0f ));
         }
       }

      DefineTextureWnd();

      break;
     }

    start=((pageid-16*pmult)*128)+256*2048*pmult;

    // convert CLUT to 32bits .. and then use THAT as a lookup table

    wSRCPtr=psxVuw+palstart;
    for(row=0;row<16;row++)
     *px++=LTCOL(*wSRCPtr++);

    sxm=g_x1&1;sxh=g_x1>>1;
    if(sxm) j=g_x1+1; else j=g_x1;
    cSRCPtr = psxVub + start + (2048*g_y1) + sxh;
    for(column=g_y1;column<=g_y2;column++)
     {
      cSRCPtr = psxVub + start + (2048*column) + sxh;
    
      if(sxm) *ta++=*(pa+((*cSRCPtr++ >> 4) & 0xF));
      
      for(row=j;row<=g_x2;row++)
       {
        *ta++=*(pa+(*cSRCPtr & 0xF)); row++;
        if(row<=g_x2) *ta++=*(pa+((*cSRCPtr >> 4) & 0xF)); 
        cSRCPtr++;
       }
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------//
   // 8bit texture load ..
   case 1:
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;

      row=64;do
       {
        *px    =LTCOL(*wSRCPtr);
        *(px+1)=LTCOL(*(wSRCPtr+1));
        *(px+2)=LTCOL(*(wSRCPtr+2));
        *(px+3)=LTCOL(*(wSRCPtr+3));
        row--;px+=4;wSRCPtr+=4;
       }
      while (row);

      for(TXV=g_y1;TXV<=g_y2;TXV++)
       {
        for(TXU=g_x1;TXU<=g_x2;TXU++)
         {
  		  n_xi = ( ( TXU >> 1 ) & ~0x78 ) + ( ( TXU << 2 ) & 0x40 ) + ( ( TXV << 3 ) & 0x38 );
		  n_yi = ( TXV & ~0x7 ) + ( ( TXU >> 5 ) & 0x7 );

          *ta++=*(pa+((*( psxVuw + ((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi ) >> ( ( TXU & 0x01 ) << 3 ) ) & 0xff));
         }
       }

      DefineTextureWnd();

      break;
     }

    start=((pageid-16*pmult)*128)+256*2048*pmult;

    // not using a lookup table here... speeds up smaller texture areas
    cSRCPtr = psxVub + start + (2048*g_y1) + g_x1;
    LineOffset = 2048 - (g_x2-g_x1+1); 

    for(column=g_y1;column<=g_y2;column++)
     {
      for(row=g_x1;row<=g_x2;row++)
       *ta++=LTCOL(psxVuw[palstart+ *cSRCPtr++]);
      cSRCPtr+=LineOffset;
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------// 
   // 16bit texture load ..
   case 2:
    start=((pageid-16*pmult)*64)+256*1024*pmult;

    wSRCPtr = psxVuw + start + (1024*g_y1) + g_x1;
    LineOffset = 1024 - (g_x2-g_x1+1); 

    for(column=g_y1;column<=g_y2;column++)
     {
      for(row=g_x1;row<=g_x2;row++)
       *ta++=LTCOL(*wSRCPtr++);
      wSRCPtr+=LineOffset;
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------// 
   // others are not possible !
  }
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void UploadTexWndPal(int mode,short cx,short cy)
{
 unsigned int i;//,iSize;
 unsigned short * wSrcPtr;
 unsigned long * ta=(unsigned long *)texturepart;

 wSrcPtr=psxVuw+cx+(cy*1024);
 if(mode==0) i=4; else i=64;
 //iSize=i<<2;
 ubOpaqueDraw=0;

 do
  {
   *ta    =PALCOL(*wSrcPtr);
   *(ta+1)=PALCOL(*(wSrcPtr+1));
   *(ta+2)=PALCOL(*(wSrcPtr+2));
   *(ta+3)=PALCOL(*(wSrcPtr+3));
   ta+=4;wSrcPtr+=4;i--;
  }
 while(i);

/* (*glColorTableEXTEx)(GL_TEXTURE_2D,GL_RGBA8,iSize,
                    GL_RGBA,GL_UNSIGNED_BYTE,texturepart);
*/}

////////////////////////////////////////////////////////////////////////

void DefinePalTextureWnd(void)
{
    if(gTexName==0) {
        s8 clampType = gFilter;
        gTexName = createTexture(1, clampType);
        
    }
    
    //--bindTexture(gTexName);
    setTexture(gTexName, TWin.Position.x1, TWin.Position.y1, 3, texturepart);
    /*glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA,
              TWin.Position.x1,
              TWin.Position.y1,
              0, GL_RGBA, GL_UNSIGNED_BYTE,texturepart);*/
    glError();
  //LOGE("DefinePalTextureWnd x:%d y:%d",TWin.Position.x1,TWin.Position.y1);
}

///////////////////////////////////////////////////////

void LoadPalWndTexturePage(int pageid, int mode, short cx, short cy)
{
 unsigned long start,row,column,j,sxh,sxm;
 unsigned char  *ta;
 unsigned char  *cSRCPtr;
 unsigned long  LineOffset;
 int pmult=pageid/16;

 ta=(unsigned char *)texturepart;

 switch(mode)
  {
   //--------------------------------------------------// 
   // 4bit texture load ..
   case 0:
    start=((pageid-16*pmult)*128)+256*2048*pmult;

    sxm=g_x1&1;sxh=g_x1>>1;
    if(sxm) j=g_x1+1; else j=g_x1;
    cSRCPtr = psxVub + start + (2048*g_y1) + sxh;
    for(column=g_y1;column<=g_y2;column++)
     {
      cSRCPtr = psxVub + start + (2048*column) + sxh;
    
      if(sxm) *ta++=((*cSRCPtr++ >> 4) & 0xF);
      
      for(row=j;row<=g_x2;row++)
       {
        *ta++=(*cSRCPtr & 0xF); row++;
        if(row<=g_x2) *ta++=((*cSRCPtr >> 4) & 0xF); 
        cSRCPtr++;
       }
     }

    DefinePalTextureWnd();
    break;
   //--------------------------------------------------// 
   // 8bit texture load ..
   case 1:
    start=((pageid-16*pmult)*128)+256*2048*pmult;

    // not using a lookup table here... speeds up smaller texture areas
    cSRCPtr = psxVub + start + (2048*g_y1) + g_x1;
    LineOffset = 2048 - (g_x2-g_x1+1); 

    for(column=g_y1;column<=g_y2;column++)
     {
      for(row=g_x1;row<=g_x2;row++)
       *ta++=*cSRCPtr++;
      cSRCPtr+=LineOffset;
     }

    DefinePalTextureWnd();
    break;
  }
 UploadTexWndPal(mode,cx,cy);
}

////////////////////////////////////////////////////////////////////////

void LoadStretchPalWndTexturePage(int pageid, int mode, short cx, short cy)
{
 unsigned long start,row,column,j,sxh,sxm,ldx,ldy,ldxo;
 unsigned char  *ta,s;
 unsigned char  *cSRCPtr,*cOSRCPtr;
 unsigned long  LineOffset;
 int pmult=pageid/16;

 ldxo=TWin.Position.x1-TWin.OPosition.x1;
 ldy =TWin.Position.y1-TWin.OPosition.y1;

 ta=(unsigned char *)texturepart;

 switch(mode)
  {
   //--------------------------------------------------// 
   // 4bit texture load ..
   case 0:
    start=((pageid-16*pmult)*128)+256*2048*pmult;

    sxm=g_x1&1;sxh=g_x1>>1;
    if(sxm) j=g_x1+1; else j=g_x1;
    cSRCPtr = psxVub + start + (2048*g_y1) + sxh;
    for(column=g_y1;column<=g_y2;column++)
     {
      cOSRCPtr=cSRCPtr;ldx=ldxo;
      if(sxm) *ta++=((*cSRCPtr++ >> 4) & 0xF);
      
      for(row=j;row<=g_x2-ldxo;row++)
       {
        s=(*cSRCPtr & 0xF);
        *ta++=s;
        if(ldx) {*ta++=s;ldx--;}
        row++;
        if(row<=g_x2-ldxo) 
         {
          s=((*cSRCPtr >> 4) & 0xF);
          *ta++=s; 
          if(ldx) {*ta++=s;ldx--;}
         }
        cSRCPtr++;
       }
      if(ldy && column&1) 
           {ldy--;cSRCPtr = cOSRCPtr;}
      else cSRCPtr = psxVub + start + (2048*(column+1)) + sxh;
     }

    DefinePalTextureWnd();
    break;
   //--------------------------------------------------// 
   // 8bit texture load ..
   case 1:
    start=((pageid-16*pmult)*128)+256*2048*pmult;

    cSRCPtr = psxVub + start + (2048*g_y1) + g_x1;
    LineOffset = 2048 - (g_x2-g_x1+1) +ldxo; 

    for(column=g_y1;column<=g_y2;column++)
     {
      cOSRCPtr=cSRCPtr;ldx=ldxo;
      for(row=g_x1;row<=g_x2-ldxo;row++)
       {
        s=*cSRCPtr++;
        *ta++=s;
        if(ldx) {*ta++=s;ldx--;}
       }
      if(ldy && column&1) {ldy--;cSRCPtr=cOSRCPtr;}
      else                cSRCPtr+=LineOffset;
     }

    DefinePalTextureWnd();
    break;
  }
 UploadTexWndPal(mode,cx,cy);
}

////////////////////////////////////////////////////////////////////////
// tex window: main selecting, cache handler included
////////////////////////////////////////////////////////////////////////

u32 LoadTextureWnd(long pageid,long TextureMode,unsigned long GivenClutId)
{
 textureWndCacheEntry * ts, * tsx=NULL;
 int i;short cx,cy;
 EXLong npos;

 npos.c[3]=TWin.Position.x0;
 npos.c[2]=TWin.OPosition.x1;
 npos.c[1]=TWin.Position.y0;
 npos.c[0]=TWin.OPosition.y1;

 g_x1=TWin.Position.x0;g_x2=g_x1+TWin.Position.x1-1;
 g_y1=TWin.Position.y0;g_y2=g_y1+TWin.Position.y1-1;

 if(TextureMode==2) {GivenClutId=0;cx=cy=0;}
 else  
  {
   cx=((GivenClutId << 4) & 0x3F0);cy=((GivenClutId >> 6) & CLUTYMASK);
   GivenClutId=(GivenClutId&CLUTMASK)|(DrawSemiTrans<<30);

   // palette check sum
    {
     unsigned long l=0,row;
     unsigned long * lSRCPtr=(unsigned long *)(psxVuw+cx+(cy*1024));
     if(TextureMode==1) for(row=1;row<129;row++) l+=((*lSRCPtr++)-1)*row;
     else               for(row=1;row<9;row++)   l+=((*lSRCPtr++)-1)<<row;
     l=(l+HIWORD(l))&0x3fffL;
     GivenClutId|=(l<<16);
    }

  }

 ts=wcWndtexStore;

 for(i=0;i<iMaxTexWnds;i++,ts++)
  {
   if(ts->used)
    {
     if(ts->pos.l==npos.l &&
        ts->pageid==pageid &&
        ts->textureMode==TextureMode)
      {
       if(ts->ClutID==GivenClutId)
        {
         ubOpaqueDraw=ts->Opaque;
         return ts->texname;
        }
      }
    }
   else tsx=ts;
  }

 if(!tsx) 
  {
   if(iMaxTexWnds==iTexWndLimit)
    {
     tsx=wcWndtexStore+iTexWndTurn;
     iTexWndTurn++; 
     if(iTexWndTurn==iTexWndLimit) iTexWndTurn=0;
    }
   else
    {
     tsx=wcWndtexStore+iMaxTexWnds;
     iMaxTexWnds++;
    }
  }

 gTexName=tsx->texname;

 if(TWin.OPosition.y1==TWin.Position.y1 &&
    TWin.OPosition.x1==TWin.Position.x1)
  {
    LoadWndTexturePage(pageid,TextureMode,cx,cy);
  }       
 else
  {
    LoadStretchWndTexturePage(pageid,TextureMode,cx,cy);
  }

 tsx->Opaque=ubOpaqueDraw;
 tsx->pos.l=npos.l;
 tsx->ClutID=GivenClutId;
 tsx->pageid=pageid;
 tsx->textureMode=TextureMode;
 tsx->texname=gTexName;
 tsx->used=1;
       
 return gTexName;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// movie texture: define
////////////////////////////////////////////////////////////////////////

void DefinePackedTextureMovie(void)
{
    if(gTexMovieName==0) {
        s8 clampType = gClampType;
        s8 filter;
        if(!bUseFastMdec) {
            filter = 1;
        } else {
            filter = gFilter;
        }
        gTexMovieName = createTexture(filter, clampType);
        gTexName=gTexMovieName;
   
        //--bindTexture(gTexName);
        setTexture(gTexName, 256, 256, 3, texturepart);
        /*glTexImage2D(GL_TEXTURE_2D, 0, //giWantedRGBA,
                GL_RGBA,
                256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, texturepart);glError();*/
  }
 else 
  {
   gTexName=gTexMovieName;
   bindTexture(gTexName);
  }

    setSubTexture(gTexMovieName, 0, 0, (xrMovieArea.x1-xrMovieArea.x0), (xrMovieArea.y1-xrMovieArea.y0), 3, texturepart);
 /*glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                 (xrMovieArea.x1-xrMovieArea.x0),
                 (xrMovieArea.y1-xrMovieArea.y0),
                 GL_RGBA,
                 GL_UNSIGNED_SHORT,
                 texturepart);glError();*/
  //LOGE("DefinePackedTextureMovie x:%d y:%d",(xrMovieArea.x1-xrMovieArea.x0),(xrMovieArea.y1-xrMovieArea.y0));

}

////////////////////////////////////////////////////////////////////////

void DefineTextureMovie(void)
{
    if(gTexMovieName==0) {
        s8 clampType = gClampType;
        s8 filter;
        if(!bUseFastMdec) {
            filter = 1;
        } else {
            filter = gFilter;
        }
        gTexMovieName = createTexture(filter, clampType);
        gTexName=gTexMovieName;
        
        //--bindTexture(gTexName);
        setTexture(gTexName, 256, 256, 3, texturepart);
        //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, texturepart);glError();
  }
 else 
  {
   gTexName=gTexMovieName;bindTexture(gTexName);
  }

    setSubTexture(gTexMovieName, 0, 0, (xrMovieArea.x1-xrMovieArea.x0), (xrMovieArea.y1-xrMovieArea.y0), 3, texturepart);

 /*glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                 (xrMovieArea.x1-xrMovieArea.x0), 
                 (xrMovieArea.y1-xrMovieArea.y0), 
                 GL_RGBA, GL_UNSIGNED_BYTE, texturepart);glError();*/
 //LOGE("DefineTextureMovie x:%d y:%d",(xrMovieArea.x1-xrMovieArea.x0),(xrMovieArea.y1-xrMovieArea.y0));
}

////////////////////////////////////////////////////////////////////////
// movie texture: load
////////////////////////////////////////////////////////////////////////

#define MRED(x)   ((x>>3) & 0x1f)
#define MGREEN(x) ((x>>6) & 0x3e0)
#define MBLUE(x)  ((x>>9) & 0x7c00)

#define XMGREEN(x) ((x>>5)  & 0x07c0)
#define XMRED(x)   ((x<<8)  & 0xf800)
#define XMBLUE(x)  ((x>>18) & 0x003e)

////////////////////////////////////////////////////////////////////////
// movie texture: load
////////////////////////////////////////////////////////////////////////

unsigned char * LoadDirectMovieFast(void)
{
 long row,column;
 unsigned int startxy;

 unsigned long * ta=(unsigned long *)texturepart;

 if(PSXDisplay.RGB24)
  {
   unsigned char * pD;

   startxy=((1024)*xrMovieArea.y0)+xrMovieArea.x0;

   for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++,startxy+=1024)
    {
     pD=(unsigned char *)&psxVuw[startxy];
     for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
      {
       *ta++=*((unsigned long *)pD)|0xff000000;
       pD+=3;
      }
    }
  }
 else
  {
   unsigned long (*LTCOL)(unsigned long);

   LTCOL=XP8RGBA_0;//TCF[0];

   ubOpaqueDraw=0;

   for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++)
    {
     startxy=((1024)*column)+xrMovieArea.x0;
     for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
      *ta++=LTCOL(psxVuw[startxy++]|0x8000);
    }
  }
 
 return texturepart;
}

////////////////////////////////////////////////////////////////////////

u32 LoadTextureMovieFast(void)
{
 long row,column;
 unsigned int startxy;

{
   if(PSXDisplay.RGB24)
    {
     unsigned char * pD;
     unsigned long * ta=(unsigned long *)texturepart;

     startxy=((1024)*xrMovieArea.y0)+xrMovieArea.x0;

     for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++,startxy+=1024)
      {
       //startxy=((1024)*column)+xrMovieArea.x0;
       pD=(unsigned char *)&psxVuw[startxy];
       for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
        {
         *ta++=*((unsigned long *)pD)|0xff000000;
         pD+=3;
        }
      }
    }
   else
    {
     unsigned long (*LTCOL)(unsigned long);
     unsigned long *ta;

     LTCOL=XP8RGBA_0;//TCF[0];

     ubOpaqueDraw=0;
     ta=(unsigned long *)texturepart;

     for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++)
      {
       startxy=((1024)*column)+xrMovieArea.x0;
       for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
        *ta++=LTCOL(psxVuw[startxy++]|0x8000);
      }
    }
   DefineTextureMovie();
  }
 return gTexName;   
}

////////////////////////////////////////////////////////////////////////

u32 LoadTextureMovie(void)
{
 short row,column,dx;
 unsigned int startxy;
 BOOL b_X,b_Y;

 if(bUseFastMdec) return LoadTextureMovieFast();

 b_X=FALSE;b_Y=FALSE;

 if((xrMovieArea.x1-xrMovieArea.x0)<255)  b_X=TRUE;
 if((xrMovieArea.y1-xrMovieArea.y0)<255)  b_Y=TRUE;

{
   if(PSXDisplay.RGB24)
    {
     unsigned char * pD;
     unsigned long * ta=(unsigned long *)texturepart;

     if(b_X)
      {
       for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++)
        {
         startxy=((1024)*column)+xrMovieArea.x0;
         pD=(unsigned char *)&psxVuw[startxy];
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
          {
           *ta++=*((unsigned long *)pD)|0xff000000;
           pD+=3;
          }
         *ta=*(ta-1);
         ta++;
        }
       if(b_Y)
        {
         dx=xrMovieArea.x1-xrMovieArea.x0+1;
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++) {
          *ta=*(ta-dx);
          ta++;
          }
         *ta=*(ta-1);
         ta++;
        }
      }
     else
      {
       for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++)
        {
         startxy=((1024)*column)+xrMovieArea.x0;
         pD=(unsigned char *)&psxVuw[startxy];
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
          {
           *ta++=*((unsigned long *)pD)|0xff000000;
           pD+=3;
          }
        }
       if(b_Y)
        {
         dx=xrMovieArea.x1-xrMovieArea.x0;
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++) {
          *ta=*(ta-dx);
          ta++;
          }
        }
      }
    }
   else
    {
     unsigned long (*LTCOL)(unsigned long);
     unsigned long *ta;

     LTCOL=XP8RGBA_0;//TCF[0];

     ubOpaqueDraw=0;
     ta=(unsigned long *)texturepart;

     if(b_X)
      {
       for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++)
        {
         startxy=((1024)*column)+xrMovieArea.x0;
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++) {
          *ta=LTCOL(psxVuw[startxy++]|0x8000);
          ta++;
          }
         *ta=*(ta-1);ta++;
        }

       if(b_Y)
        {
         dx=xrMovieArea.x1-xrMovieArea.x0+1;
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++) {
          *ta=*(ta-dx);
          ta++;
          }
         *ta=*(ta-1);
         ta++;
        }
      }
     else
      {
       for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++)
        {
         startxy=((1024)*column)+xrMovieArea.x0;
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
          *ta++=LTCOL(psxVuw[startxy++]|0x8000);
        }

       if(b_Y)
        {
         dx=xrMovieArea.x1-xrMovieArea.x0;
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++) {
          *ta=*(ta-dx);
          ta++;
          }
        }
      }
    }

   xrMovieArea.x1+=b_X;xrMovieArea.y1+=b_Y;
   DefineTextureMovie();
   xrMovieArea.x1-=b_X;xrMovieArea.y1-=b_Y;
  }
 return gTexName;   
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

u32 BlackFake15BitTexture(void)
{
 long pmult;short x1,x2,y1,y2;

 if(PSXDisplay.InterlacedTest) return 0;
 
 pmult=GlobalTexturePage/16;
 x1=gl_ux[7];
 x2=gl_ux[6]-gl_ux[7];
 y1=gl_ux[5];
 y2=gl_ux[4]-gl_ux[5];

 if(iSpriteTex)
  {
   if(x2<255) x2++;
   if(y2<255) y2++;
  }

 y1+=pmult*256;
 x1+=((GlobalTexturePage-16*pmult)<<6);

 if(   FastCheckAgainstFrontScreen(x1,y1,x2,y2)
    || FastCheckAgainstScreen(x1,y1,x2,y2))
  {
        if(!gTexFrameName) {
            s8 clampType = gClampType;
            s8 filter = gFilter;
            gTexFrameName = createTexture(filter, clampType);
            gTexName=gTexFrameName;
     
            //bindTexture(gTexName);

     
     {
       unsigned long * ta=(unsigned long *)texturepart;
       for(y1=0;y1<=4;y1++)
        for(x1=0;x1<=4;x1++)
         *ta++=0xff000000;
      }
      setTexture(gTexName, 4, 4, 3, texturepart);
     //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, texturepart);glError();

    }
   else
    {
     gTexName=gTexFrameName;
     bindTexture(gTexName);
    }
      //LOGE("BlackFake15BitTexture x:%d y:%d",4,4);
   ubOpaqueDraw=0;

   return (u32)gTexName;
  }
 return 0;
}

/////////////////////////////////////////////////////////////////////////////

BOOL bFakeFrontBuffer=FALSE;
BOOL bIgnoreNextTile =FALSE;

int iFTex=512;

u32 Fake15BitTexture(void)
{
 long pmult;short x1,x2,y1,y2;int iYAdjust;
 float ScaleX,ScaleY;RECT rSrc;

 if(iFrameTexType==1) return BlackFake15BitTexture();
 if(PSXDisplay.InterlacedTest) return 0;
 
 pmult=GlobalTexturePage/16;
 x1=gl_ux[7];
 x2=gl_ux[6]-gl_ux[7];
 y1=gl_ux[5];
 y2=gl_ux[4]-gl_ux[5];

 y1+=pmult*256;
 x1+=((GlobalTexturePage-16*pmult)<<6);

 if(iFrameTexType==3)
  {
   if(iFrameReadType==4) return 0;

   if(!FastCheckAgainstFrontScreen(x1,y1,x2,y2) &&
      !FastCheckAgainstScreen(x1,y1,x2,y2))
    return 0;

   if(bFakeFrontBuffer) bIgnoreNextTile=TRUE;
   CheckVRamReadEx(x1,y1,x1+x2,y1+y2);
   return 0;
  }

 /////////////////////////

 if(FastCheckAgainstFrontScreen(x1,y1,x2,y2))
  {
   x1-=PSXDisplay.DisplayPosition.x;
   y1-=PSXDisplay.DisplayPosition.y;
  }
 else
 if(FastCheckAgainstScreen(x1,y1,x2,y2))
  {
   x1-=PreviousPSXDisplay.DisplayPosition.x;
   y1-=PreviousPSXDisplay.DisplayPosition.y;
  }
 else return 0;

 bDrawMultiPass = FALSE;

 if(!gTexFrameName)
  {
   u8* p;

   if(iResX>1280 || iResY>1024) iFTex=2048;
   else
   if(iResX>640  || iResY>480)  iFTex=1024;
   else                         iFTex=512; 

    
    s8 clampType = gClampType;
    s8 filter = gFilter;
    gTexFrameName = createTexture(filter, clampType);
    gTexName=gTexFrameName;


   //bindTexture(gTexName);


   p=(u8 *)malloc(iFTex*iFTex*4);
   memset(p,0,iFTex*iFTex*4);
   setTexture(gTexName, iFTex, iFTex, 3, p);
   //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iFTex, iFTex, 0, GL_RGBA, GL_UNSIGNED_BYTE, p);glError();
   //glGetError();
   free(p);

  }
 else 
  {
   gTexName=gTexFrameName;
   bindTexture(gTexName);
  }
      //LOGE("Fake15BitTexture x:%d y:%d",iFTex,iFTex);
 x1+=PreviousPSXDisplay.Range.x0;
 y1+=PreviousPSXDisplay.Range.y0;

 if(PSXDisplay.DisplayMode.x)
      ScaleX=(float)rRatioRect.right/(float)PSXDisplay.DisplayMode.x;
 else ScaleX=1.0f;
 if(PSXDisplay.DisplayMode.y)
      ScaleY=(float)rRatioRect.bottom/(float)PSXDisplay.DisplayMode.y;
 else ScaleY=1.0f;

 rSrc.left  =max(x1*ScaleX,0);
 rSrc.right =min((x1+x2)*ScaleX+0.99f,iResX-1);
 rSrc.top   =max(y1*ScaleY,0);
 rSrc.bottom=min((y1+y2)*ScaleY+0.99f,iResY-1);

 iYAdjust=(y1+y2)-PSXDisplay.DisplayMode.y;
 if(iYAdjust>0)
      iYAdjust=(int)((float)iYAdjust*ScaleY)+1;
 else iYAdjust=0;
          
 gl_vy[0]=255-gl_vy[0];
 gl_vy[1]=255-gl_vy[1];
 gl_vy[2]=255-gl_vy[2];
 gl_vy[3]=255-gl_vy[3];

 y1=min(gl_vy[0],min(gl_vy[1],min(gl_vy[2],gl_vy[3])));

 gl_vy[0]-=y1;
 gl_vy[1]-=y1;
 gl_vy[2]-=y1;
 gl_vy[3]-=y1;
 gl_ux[0]-=gl_ux[7];
 gl_ux[1]-=gl_ux[7];
 gl_ux[2]-=gl_ux[7];
 gl_ux[3]-=gl_ux[7];

 ScaleX*=256.0f/((float)(iFTex));
 ScaleY*=256.0f/((float)(iFTex));

 y1=((float)gl_vy[0]*ScaleY); if(y1>255) y1=255;
 gl_vy[0]=y1;
 y1=((float)gl_vy[1]*ScaleY); if(y1>255) y1=255;
 gl_vy[1]=y1;
 y1=((float)gl_vy[2]*ScaleY); if(y1>255) y1=255;
 gl_vy[2]=y1;
 y1=((float)gl_vy[3]*ScaleY); if(y1>255) y1=255;
 gl_vy[3]=y1;

 x1=((float)gl_ux[0]*ScaleX); if(x1>255) x1=255;
 gl_ux[0]=x1;
 x1=((float)gl_ux[1]*ScaleX); if(x1>255) x1=255;
 gl_ux[1]=x1;
 x1=((float)gl_ux[2]*ScaleX); if(x1>255) x1=255;
 gl_ux[2]=x1;
 x1=((float)gl_ux[3]*ScaleX); if(x1>255) x1=255;
 gl_ux[3]=x1;

 x1=rSrc.right-rSrc.left;
 if(x1<=0)             x1=1;
 if(x1>iFTex)          x1=iFTex;

 y1=rSrc.bottom-rSrc.top;
 if(y1<=0)             y1=1;
 if(y1+iYAdjust>iFTex) y1=iFTex-iYAdjust;


 copySubTexture(gTexName, 0, iYAdjust, rSrc.left+rRatioRect.left, iResY-rSrc.bottom-rRatioRect.top, x1,y1);
 //glError();*/

 if(hasError())
  {
   u8 *p=(u8*)malloc(iFTex*iFTex*4);
   memset(p,0,iFTex*iFTex*4);
   setSubTexture(gTexName, 0, 0, iFTex, iFTex, 3, p);
   //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, iFTex, iFTex,
   //                GL_RGBA, GL_UNSIGNED_BYTE, p);glError();
   free(p);
  }

 
 ubOpaqueDraw=0;

 if(iSpriteTex)
  {
   sprtW=gl_ux[1]-gl_ux[0];    
   sprtH=-(gl_vy[0]-gl_vy[2]);
  }

 return (u32)gTexName;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// load texture part (unpacked)
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void LoadSubTexturePageSort(int pageid, int mode, short cx, short cy)
{
 unsigned long  start,row,column,j,sxh,sxm;
 unsigned int   palstart;
 unsigned long  *px,*pa,*ta;
 unsigned char  *cSRCPtr;
 unsigned short *wSRCPtr;
 unsigned long  LineOffset;
 unsigned long  x2a,xalign=0;
 unsigned long  x1=gl_ux[7];
 unsigned long  x2=gl_ux[6];
 unsigned long  y1=gl_ux[5];
 unsigned long  y2=gl_ux[4];
 unsigned long  dx=x2-x1+1;
 unsigned long  dy=y2-y1+1;
 int pmult=pageid/16;
 unsigned long (*LTCOL)(unsigned long);
 unsigned int a,r,g,b,cnt,h;
 unsigned long scol[8];
 
 LTCOL=TCF[DrawSemiTrans];

 pa=px=(unsigned long *)ubPaletteBuffer;
 ta=(unsigned long *)texturepart;
 palstart=cx+(cy<<10);

 ubOpaqueDraw=0;

 if(YTexS) {ta+=dx;if(XTexS) ta+=2;}
 if(XTexS) {ta+=1;xalign=2;}
  //LOGE("LoadSubTexturePageSort bits:%d x:%d y:%d w:%d h:%d",mode,cx,cy,x2-x1,y2-y1);
 switch(mode)
  {
   //--------------------------------------------------// 
   // 4bit texture load ..
   case 0:
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;

      row=4;do
       {
        *px    =LTCOL(*wSRCPtr);
        *(px+1)=LTCOL(*(wSRCPtr+1));
        *(px+2)=LTCOL(*(wSRCPtr+2));
        *(px+3)=LTCOL(*(wSRCPtr+3));
        row--;px+=4;wSRCPtr+=4;
       }
      while (row);

      for(TXV=y1;TXV<=y2;TXV++)
       {
        for(TXU=x1;TXU<=x2;TXU++)
         {
  		  n_xi = ( ( TXU >> 2 ) & ~0x3c ) + ( ( TXV << 2 ) & 0x3c );
		  n_yi = ( TXV & ~0xf ) + ( ( TXU >> 4 ) & 0xf );

          *ta++=*(pa+((*( psxVuw + ((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi ) >> ( ( TXU & 0x03 ) << 2 ) ) & 0x0f ));
         }
        ta+=xalign;
       }
      break;
     }

    start=((pageid-16*pmult)<<7)+524288*pmult;
    // convert CLUT to 32bits .. and then use THAT as a lookup table

    wSRCPtr=psxVuw+palstart;

    row=4;do
     {
      *px    =LTCOL(*wSRCPtr);
      *(px+1)=LTCOL(*(wSRCPtr+1));
      *(px+2)=LTCOL(*(wSRCPtr+2));
      *(px+3)=LTCOL(*(wSRCPtr+3));
      row--;px+=4;wSRCPtr+=4;
     }
    while (row);

    x2a=x2?(x2-1):0;//if(x2) x2a=x2-1; else x2a=0;
    sxm=x1&1;sxh=x1>>1;
    j=sxm?(x1+1):x1;//if(sxm) j=x1+1; else j=x1;
    for(column=y1;column<=y2;column++)
     {
      cSRCPtr = psxVub + start + (column<<11) + sxh;
    
      if(sxm) *ta++=*(pa+((*cSRCPtr++ >> 4) & 0xF));

      for(row=j;row<x2a;row+=2)
       {
        *ta    =*(pa+(*cSRCPtr & 0xF)); 
        *(ta+1)=*(pa+((*cSRCPtr >> 4) & 0xF)); 
        cSRCPtr++;ta+=2;
       }

      if(row<=x2) 
       {
        *ta++=*(pa+(*cSRCPtr & 0xF)); row++;
        if(row<=x2) *ta++=*(pa+((*cSRCPtr >> 4) & 0xF));
       }

      ta+=xalign;
     }

    break;
   //--------------------------------------------------// 
   // 8bit texture load ..
   case 1:
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;

      row=64;do
       {
        *px    =LTCOL(*wSRCPtr);
        *(px+1)=LTCOL(*(wSRCPtr+1));
        *(px+2)=LTCOL(*(wSRCPtr+2));
        *(px+3)=LTCOL(*(wSRCPtr+3));
        row--;px+=4;wSRCPtr+=4;
       }
      while (row);

      for(TXV=y1;TXV<=y2;TXV++)
       {
        for(TXU=x1;TXU<=x2;TXU++)
         {
  		  n_xi = ( ( TXU >> 1 ) & ~0x78 ) + ( ( TXU << 2 ) & 0x40 ) + ( ( TXV << 3 ) & 0x38 );
		  n_yi = ( TXV & ~0x7 ) + ( ( TXU >> 5 ) & 0x7 );

          *ta++=*(pa+((*( psxVuw + ((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi ) >> ( ( TXU & 0x01 ) << 3 ) ) & 0xff));
         }
        ta+=xalign;
       }

      break;
     }

    start=((pageid-16*pmult)<<7)+524288*pmult;

    cSRCPtr = psxVub + start + (y1<<11) + x1;
    LineOffset = 2048 - dx; 

    if(dy*dx>384)
     {
      wSRCPtr=psxVuw+palstart;

      row=64;do
       {
        *px    =LTCOL(*wSRCPtr);
        *(px+1)=LTCOL(*(wSRCPtr+1));
        *(px+2)=LTCOL(*(wSRCPtr+2));
        *(px+3)=LTCOL(*(wSRCPtr+3));
        row--;px+=4;wSRCPtr+=4;
       }
      while (row);

      column=dy;do 
       {
        row=dx;
        do {*ta++=*(pa+(*cSRCPtr++));row--;} while(row);
        ta+=xalign;
        cSRCPtr+=LineOffset;column--;
       }
      while(column);
     }
    else
     {
      wSRCPtr=psxVuw+palstart;

      column=dy;do 
       {
        row=dx;
        do {*ta++=LTCOL(*(wSRCPtr+*cSRCPtr++));row--;} while(row);
        ta+=xalign;
        cSRCPtr+=LineOffset;column--;
       }
      while(column);
     }

    break;
   //--------------------------------------------------// 
   // 16bit texture load ..
   case 2:
    start=((pageid-16*pmult)<<6)+262144*pmult;

    wSRCPtr = psxVuw + start + (y1<<10) + x1;
    LineOffset = 1024 - dx; 

    column=dy;do 
     {
      row=dx;
      do {*ta++=LTCOL(*wSRCPtr++);row--;} while(row);
      ta+=xalign;
      wSRCPtr+=LineOffset;column--;
     }
    while(column);

    break;
   //--------------------------------------------------//
   // others are not possible !
  }

 x2a=dx+xalign;

 if(YTexS)
  {
   ta=(unsigned long *)texturepart;
   pa=(unsigned long *)texturepart+x2a;
   row=x2a;do {*ta++=*pa++;row--;} while(row);        
   pa=(unsigned long *)texturepart+dy*x2a;
   ta=pa+x2a;
   row=x2a;do {*ta++=*pa++;row--;} while(row);
   YTexS--;
   dy+=2;
  }

 if(XTexS)
  {
   ta=(unsigned long *)texturepart;
   pa=ta+1;
   row=dy;do {*ta=*pa;ta+=x2a;pa+=x2a;row--;} while(row);
   pa=(unsigned long *)texturepart+dx;
   ta=pa+1;
   row=dy;do {*ta=*pa;ta+=x2a;pa+=x2a;row--;} while(row);
   XTexS--;
   dx+=2;
  }

 DXTexS=dx;DYTexS=dy;

 if(!iFilterType) {DefineSubTextureSort();return;}
 if(iFilterType!=2 && iFilterType!=4 && iFilterType!=6) {DefineSubTextureSort();return;}
 if((iFilterType==4 || iFilterType==6) && ly0==ly1 && ly2==ly3 && lx0==lx3 && lx1==lx2)
  {DefineSubTextureSort();return;}

 ta=(unsigned long *)texturepart;
 x1=dx-1;
 y1=dy-1;

 if(bOpaquePass)
  {
{
     for(column=0;column<dy;column++)
      {
       for(row=0;row<dx;row++)
        {
         if(*ta==0x50000000)
          {
           cnt=0;

           if(           column     && *(ta-dx)  !=0x50000000 && *(ta-dx)>>24!=1) scol[cnt++]=*(ta-dx);
           if(row                   && *(ta-1)   !=0x50000000 && *(ta-1)>>24!=1) scol[cnt++]=*(ta-1);
           if(row!=x1               && *(ta+1)   !=0x50000000 && *(ta+1)>>24!=1) scol[cnt++]=*(ta+1);
           if(           column!=y1 && *(ta+dx)  !=0x50000000 && *(ta+dx)>>24!=1) scol[cnt++]=*(ta+dx);

           if(row     && column     && *(ta-dx-1)!=0x50000000 && *(ta-dx-1)>>24!=1) scol[cnt++]=*(ta-dx-1);
           if(row!=x1 && column     && *(ta-dx+1)!=0x50000000 && *(ta-dx+1)>>24!=1) scol[cnt++]=*(ta-dx+1);
           if(row     && column!=y1 && *(ta+dx-1)!=0x50000000 && *(ta+dx-1)>>24!=1) scol[cnt++]=*(ta+dx-1);
           if(row!=x1 && column!=y1 && *(ta+dx+1)!=0x50000000 && *(ta+dx+1)>>24!=1) scol[cnt++]=*(ta+dx+1);

           if(cnt)
            {
             r=g=b=a=0;
             for(h=0;h<cnt;h++)
              {
               a+=(scol[h]>>24);
               r+=(scol[h]>>16)&0xff;
               g+=(scol[h]>>8)&0xff;
               b+=scol[h]&0xff;
              }
             r/=cnt;b/=cnt;g/=cnt;

             *ta=(r<<16)|(g<<8)|b;
             if(a) *ta|=0x50000000;
             else  *ta|=0x01000000;
            }
          }
         ta++;
        }
      }
    }
  }
 else
 for(column=0;column<dy;column++)
  {
   for(row=0;row<dx;row++)
    {
     if(*ta==0x00000000)
      {
       cnt=0;

       if(row!=x1               && *(ta+1)   !=0x00000000) scol[cnt++]=*(ta+1);
       if(           column!=y1 && *(ta+dx)  !=0x00000000) scol[cnt++]=*(ta+dx);

       if(cnt)
        {
         r=g=b=0;
         for(h=0;h<cnt;h++)
          {
           r+=(scol[h]>>16)&0xff;
           g+=(scol[h]>>8)&0xff;
           b+=scol[h]&0xff;
          }
         r/=cnt;b/=cnt;g/=cnt;
         *ta=(r<<16)|(g<<8)|b;
        }
      }
     ta++;
    }
  }

 DefineSubTextureSort();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// load texture part (packed)
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void LoadPackedSubTexturePageSort(int pageid, int mode, short cx, short cy)
{
 unsigned long  start,row,column,j,sxh,sxm;
 unsigned int   palstart;
 unsigned short *px,*pa,*ta;
 unsigned char  *cSRCPtr;
 unsigned short *wSRCPtr;
 unsigned long  LineOffset;
 unsigned long  x2a,xalign=0;
 unsigned long  x1=gl_ux[7];
 unsigned long  x2=gl_ux[6];
 unsigned long  y1=gl_ux[5];
 unsigned long  y2=gl_ux[4];
 unsigned long  dx=x2-x1+1;
 unsigned long  dy=y2-y1+1;
 int pmult=pageid/16;
 unsigned short (*LPTCOL)(unsigned short);
 unsigned int r,g,b,cnt,h;
 unsigned short scol[8];

 LPTCOL=PTCF[DrawSemiTrans];

 pa=px=(unsigned short *)ubPaletteBuffer;
 ta=(unsigned short *)texturepart;
 palstart=cx+(cy<<10);

 ubOpaqueDraw=0;

 if(YTexS) {ta+=dx;if(XTexS) ta+=2;}
 if(XTexS) {ta+=1;xalign=2;}
      //LOGE("LoadPackedSubTexturePageSort bits:%d x:%d y:%d",mode,cx,cy);

 switch(mode)
  {
   //--------------------------------------------------// 
   // 4bit texture load ..
   case 0:
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;
      row=4;do
       {
        *px    =LPTCOL(*wSRCPtr);
        *(px+1)=LPTCOL(*(wSRCPtr+1));
        *(px+2)=LPTCOL(*(wSRCPtr+2));
        *(px+3)=LPTCOL(*(wSRCPtr+3));
        row--;px+=4;wSRCPtr+=4;
       }
      while (row);

      for(TXV=y1;TXV<=y2;TXV++)
       {
        for(TXU=x1;TXU<=x2;TXU++)
         {
  		  n_xi = ( ( TXU >> 2 ) & ~0x3c ) + ( ( TXV << 2 ) & 0x3c );
		  n_yi = ( TXV & ~0xf ) + ( ( TXU >> 4 ) & 0xf );

          *ta++=*(pa+((*( psxVuw + ((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi ) >> ( ( TXU & 0x03 ) << 2 ) ) & 0x0f ));
         }
        ta+=xalign;
       }
      break;
     }

    start=((pageid-16*pmult)<<7)+524288*pmult;

    wSRCPtr=psxVuw+palstart;
    row=4;do
     {
      *px    =LPTCOL(*wSRCPtr);
      *(px+1)=LPTCOL(*(wSRCPtr+1));
      *(px+2)=LPTCOL(*(wSRCPtr+2));
      *(px+3)=LPTCOL(*(wSRCPtr+3));
      row--;px+=4;wSRCPtr+=4;
     }
    while (row);

    x2a=x2?(x2-1):0;//if(x2) x2a=x2-1; else x2a=0;
    sxm=x1&1;sxh=x1>>1;
    j=sxm?(x1+1):x1;//if(sxm) j=x1+1; else j=x1;

    for(column=y1;column<=y2;column++)
     {
      cSRCPtr = psxVub + start + (column<<11) + sxh;
    
      if(sxm) *ta++=*(pa+((*cSRCPtr++ >> 4) & 0xF));
 
      for(row=j;row<x2a;row+=2)
       {
        *ta    =*(pa+(*cSRCPtr & 0xF));
        *(ta+1)=*(pa+((*cSRCPtr >> 4) & 0xF)); 
        cSRCPtr++;ta+=2;
       }

      if(row<=x2)
       {
        *ta++=*(pa+(*cSRCPtr & 0xF));row++;
        if(row<=x2) *ta++=*(pa+((*cSRCPtr >> 4) & 0xF));
       }

      ta+=xalign;
     }
    break;
   //--------------------------------------------------// 
   // 8bit texture load ..
   case 1:
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;

      row=64;do
       {
        *px    =LPTCOL(*wSRCPtr);
        *(px+1)=LPTCOL(*(wSRCPtr+1));
        *(px+2)=LPTCOL(*(wSRCPtr+2));
        *(px+3)=LPTCOL(*(wSRCPtr+3));
        row--;px+=4;wSRCPtr+=4;
       }
      while (row);

      for(TXV=y1;TXV<=y2;TXV++)
       {
        for(TXU=x1;TXU<=x2;TXU++)
         {
  		  n_xi = ( ( TXU >> 1 ) & ~0x78 ) + ( ( TXU << 2 ) & 0x40 ) + ( ( TXV << 3 ) & 0x38 );
		  n_yi = ( TXV & ~0x7 ) + ( ( TXU >> 5 ) & 0x7 );

          *ta++=*(pa+((*( psxVuw + ((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi ) >> ( ( TXU & 0x01 ) << 3 ) ) & 0xff));
         }
        ta+=xalign;
       }

      break;
     }

    start=((pageid-16*pmult)<<7)+524288*pmult;

    cSRCPtr = psxVub + start + (y1<<11) + x1;
    LineOffset = 2048 - dx;

    if(dy*dx>384)                                      // more pix? use lut
     {
      wSRCPtr=psxVuw+palstart;

      row=64;do
       {
        *px    =LPTCOL(*wSRCPtr);
        *(px+1)=LPTCOL(*(wSRCPtr+1));
        *(px+2)=LPTCOL(*(wSRCPtr+2));
        *(px+3)=LPTCOL(*(wSRCPtr+3));
        row--;px+=4;wSRCPtr+=4;
       }
      while (row);

      column=dy;do 
       {
        row=dx;
        do {*ta++=*(pa+(*cSRCPtr++));row--;} while(row);

        ta+=xalign;

        cSRCPtr+=LineOffset;column--;
       }
      while(column);
     }
    else                                               // small area? no lut
     {                                            
      wSRCPtr=psxVuw+palstart;

      column=dy;do 
       {
        row=dx;
        do {*ta++=LPTCOL(*(wSRCPtr+*cSRCPtr++));row--;} while(row);

        ta+=xalign;

        cSRCPtr+=LineOffset;column--;
       }
      while(column);
     }
    break;
   //--------------------------------------------------// 
   // 16bit texture load ..
   case 2:
    start=((pageid-16*pmult)<<6)+262144*pmult;

    wSRCPtr = psxVuw + start + (y1<<10) + x1;
    LineOffset = 1024 - dx; 

    column=dy;do 
     {
      row=dx;
      do {*ta++=LPTCOL(*wSRCPtr++);row--;} while(row);

      ta+=xalign;

      wSRCPtr+=LineOffset;column--;
     }
    while(column);
    break;
   //--------------------------------------------------// 
   // others are not possible !
  }

 ////////////////////////////////////////////////////////

 x2a=dx+xalign;

 if(YTexS)
  {
   ta=(unsigned short *)texturepart;
   pa=(unsigned short *)texturepart+x2a;
   row=x2a;do {*ta++=*pa++;row--;} while(row);

   pa=(unsigned short *)texturepart+dy*x2a;
   ta=pa+x2a;
   row=x2a;do {*ta++=*pa++;row--;} while(row);

   YTexS--;
   dy+=2;
  }

 if(XTexS)
  {
   ta=(unsigned short *)texturepart;
   pa=ta+1;
   row=dy;do {*ta=*pa;ta+=x2a;pa+=x2a;row--;} while(row);

   pa=(unsigned short *)texturepart+dx;
   ta=pa+1;
   row=dy;do {*ta=*pa;ta+=x2a;pa+=x2a;row--;} while(row);

   XTexS--;
   dx+=2;
  }

 DXTexS=dx;DYTexS=dy;

 if(!iFilterType) {DefineSubTextureSort();return;}
 if(iFilterType!=2 && iFilterType!=4 && iFilterType!=6) {DefineSubTextureSort();return;}
 if((iFilterType==4 || iFilterType==6) && ly0==ly1 && ly2==ly3 && lx0==lx3 && lx1==lx2)
  {DefineSubTextureSort();return;}

 ta=(unsigned short *)texturepart;
 x1=dx-1;
 y1=dy-1;      

{
   for(column=0;column<dy;column++)
    {
     for(row=0;row<dx;row++)
      {
       if(*ta==0)
        {
         cnt=0;

         if(           column     && *(ta-dx)  &1) scol[cnt++]=*(ta-dx);
         if(row                   && *(ta-1)   &1) scol[cnt++]=*(ta-1);
         if(row!=x1               && *(ta+1)   &1) scol[cnt++]=*(ta+1);
         if(           column!=y1 && *(ta+dx)  &1) scol[cnt++]=*(ta+dx);

         if(row     && column     && *(ta-dx-1)&1) scol[cnt++]=*(ta-dx-1);
         if(row!=x1 && column     && *(ta-dx+1)&1) scol[cnt++]=*(ta-dx+1);
         if(row     && column!=y1 && *(ta+dx-1)&1) scol[cnt++]=*(ta+dx-1);
         if(row!=x1 && column!=y1 && *(ta+dx+1)&1) scol[cnt++]=*(ta+dx+1);

         if(cnt)
          {
           r=g=b=0;
           for(h=0;h<cnt;h++)
            {
             r+=scol[h]>>11;
             g+=(scol[h]>>6)&0x1f;
             b+=(scol[h]>>1)&0x1f;
            }
           r/=cnt;b/=cnt;g/=cnt;
           *ta=(r<<11)|(g<<6)|(b<<1);
          }
        }
       ta++;
      }
    }
  }

 DefineSubTextureSort();
}

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// hires texture funcs
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


#define GET_RESULT(A, B, C, D) ((A != C || A != D) - (B != C || B != D))

////////////////////////////////////////////////////////////////////////

#define colorMask8     0x00FEFEFE
#define lowPixelMask8  0x00010101
#define qcolorMask8    0x00FCFCFC
#define qlowpixelMask8 0x00030303


#define INTERPOLATE8_02(A, B) (((((A & colorMask8) >> 1) + ((B & colorMask8) >> 1) + (A & B & lowPixelMask8))|((((A&0xFF000000)==0x03000000)?0x03000000:(((B&0xFF000000)==0x03000000)?0x03000000:(((A&0xFF000000)==0x00000000)?0x00000000:(((B&0xFF000000)==0x00000000)?0x00000000:0xFF000000)))))))

#define Q_INTERPOLATE8_02(A, B, C, D) (((((A & qcolorMask8) >> 2) + ((B & qcolorMask8) >> 2) + ((C & qcolorMask8) >> 2) + ((D & qcolorMask8) >> 2) + ((((A & qlowpixelMask8) + (B & qlowpixelMask8) + (C & qlowpixelMask8) + (D & qlowpixelMask8)) >> 2) & qlowpixelMask8))|((((A&0xFF000000)==0x03000000)?0x03000000:(((B&0xFF000000)==0x03000000)?0x03000000:(((C&0xFF000000)==0x03000000)?0x03000000:(((D&0xFF000000)==0x03000000)?0x03000000:(((A&0xFF000000)==0x00000000)?0x00000000:(((B&0xFF000000)==0x00000000)?0x00000000:(((C&0xFF000000)==0x00000000)?0x00000000:(((D&0xFF000000)==0x00000000)?0x00000000:0xFF000000)))))))))))

#define INTERPOLATE8(A, B) (((((A & colorMask8) >> 1) + ((B & colorMask8) >> 1) + (A & B & lowPixelMask8))|((((A&0xFF000000)==0x50000000)?0x50000000:(((B&0xFF000000)==0x50000000)?0x50000000:(((A&0xFF000000)==0x00000000)?0x00000000:(((B&0xFF000000)==0x00000000)?0x00000000:0xFF000000)))))))

#define Q_INTERPOLATE8(A, B, C, D) (((((A & qcolorMask8) >> 2) + ((B & qcolorMask8) >> 2) + ((C & qcolorMask8) >> 2) + ((D & qcolorMask8) >> 2) + ((((A & qlowpixelMask8) + (B & qlowpixelMask8) + (C & qlowpixelMask8) + (D & qlowpixelMask8)) >> 2) & qlowpixelMask8))|((((A&0xFF000000)==0x50000000)?0x50000000:(((B&0xFF000000)==0x50000000)?0x50000000:(((C&0xFF000000)==0x50000000)?0x50000000:(((D&0xFF000000)==0x50000000)?0x50000000:(((A&0xFF000000)==0x00000000)?0x00000000:(((B&0xFF000000)==0x00000000)?0x00000000:(((C&0xFF000000)==0x00000000)?0x00000000:(((D&0xFF000000)==0x00000000)?0x00000000:0xFF000000)))))))))))

void Super2xSaI_ex8_Ex(unsigned char *srcPtr, DWORD srcPitch,
	            unsigned char  *dstBitmap, int width, int height)
{
 DWORD dstPitch = srcPitch * 2;
 DWORD line;
 DWORD *dP;
 DWORD *bP;
 int   width2 = width*2;
 int iXA,iXB,iXC,iYA,iYB,iYC,finish;
 DWORD color4, color5, color6;
 DWORD color1, color2, color3;
 DWORD colorA0, colorA1, colorA2, colorA3,
       colorB0, colorB1, colorB2, colorB3,
       colorS1, colorS2;
 DWORD product1a, product1b,
       product2a, product2b;

 line = 0;

  {
   for (; height; height-=1)
	{
     bP = (DWORD *)srcPtr;
	 dP = (DWORD *)(dstBitmap + line*dstPitch);
     for (finish = width; finish; finish -= 1 )
      {
//---------------------------------------    B1 B2
//                                         4  5  6 S2
//                                         1  2  3 S1
//                                           A1 A2
       if(finish==width) iXA=0;
       else              iXA=1;
       if(finish>4) {iXB=1;iXC=2;}
       else
       if(finish>3) {iXB=1;iXC=1;}
       else         {iXB=0;iXC=0;}
       if(line==0) iYA=0;
       else        iYA=width;
       if(height>4) {iYB=width;iYC=width2;}
       else
       if(height>3) {iYB=width;iYC=width;}
       else         {iYB=0;iYC=0;}


       colorB0 = *(bP- iYA - iXA);
       colorB1 = *(bP- iYA);
       colorB2 = *(bP- iYA + iXB);
       colorB3 = *(bP- iYA + iXC);

       color4 = *(bP  - iXA);
       color5 = *(bP);
       color6 = *(bP  + iXB);
       colorS2 = *(bP + iXC);

       color1 = *(bP  + iYB  - iXA);
       color2 = *(bP  + iYB);
       color3 = *(bP  + iYB  + iXB);
       colorS1= *(bP  + iYB  + iXC);

       colorA0 = *(bP + iYC - iXA);
       colorA1 = *(bP + iYC);
       colorA2 = *(bP + iYC + iXB);
       colorA3 = *(bP + iYC + iXC);

//--------------------------------------
       if (color2 == color6 && color5 != color3)
        {
         product2b = product1b = color2;
        }
       else
       if (color5 == color3 && color2 != color6)
        {
         product2b = product1b = color5;
        }
       else
       if (color5 == color3 && color2 == color6)
        {
         register int r = 0;

         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (color1&0x00ffffff),  (colorA1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (color4&0x00ffffff),  (colorB1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (colorA2&0x00ffffff), (colorS1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (colorB2&0x00ffffff), (colorS2&0x00ffffff));

         if (r > 0)
          product2b = product1b = color6;
         else
         if (r < 0)
          product2b = product1b = color5;
         else
          {
           product2b = product1b = INTERPOLATE8_02(color5, color6);
          }
        }
       else
        {
         if (color6 == color3 && color3 == colorA1 && color2 != colorA2 && color3 != colorA0)
             product2b = Q_INTERPOLATE8_02 (color3, color3, color3, color2);
         else
         if (color5 == color2 && color2 == colorA2 && colorA1 != color3 && color2 != colorA3)
             product2b = Q_INTERPOLATE8_02 (color2, color2, color2, color3);
         else
             product2b = INTERPOLATE8_02 (color2, color3);

         if (color6 == color3 && color6 == colorB1 && color5 != colorB2 && color6 != colorB0)
             product1b = Q_INTERPOLATE8_02 (color6, color6, color6, color5);
         else
         if (color5 == color2 && color5 == colorB2 && colorB1 != color6 && color5 != colorB3)
             product1b = Q_INTERPOLATE8_02 (color6, color5, color5, color5);
         else
             product1b = INTERPOLATE8_02 (color5, color6);
        }

       if (color5 == color3 && color2 != color6 && color4 == color5 && color5 != colorA2)
        product2a = INTERPOLATE8_02(color2, color5);
       else
       if (color5 == color1 && color6 == color5 && color4 != color2 && color5 != colorA0)
        product2a = INTERPOLATE8_02(color2, color5);
       else
        product2a = color2;

       if (color2 == color6 && color5 != color3 && color1 == color2 && color2 != colorB2)
        product1a = INTERPOLATE8_02(color2, color5);
       else
       if (color4 == color2 && color3 == color2 && color1 != color5 && color2 != colorB0)
        product1a = INTERPOLATE8_02(color2, color5);
       else
        product1a = color5;

       *dP=product1a;
       *(dP+1)=product1b;
       *(dP+(width2))=product2a;
       *(dP+1+(width2))=product2b;

       bP += 1;
       dP += 2;
      }//end of for ( finish= width etc..)

     line += 2;
     srcPtr += srcPitch;
	}; //endof: for (; height; height--)
  }
}


void Super2xSaI_ex8(unsigned char *srcPtr, DWORD srcPitch,
	            unsigned char  *dstBitmap, int width, int height)
{
 DWORD dstPitch = srcPitch * 2;
 DWORD line;
 DWORD *dP;
 DWORD *bP;
 int   width2 = width*2;
 int iXA,iXB,iXC,iYA,iYB,iYC,finish;
 DWORD color4, color5, color6;
 DWORD color1, color2, color3;
 DWORD colorA0, colorA1, colorA2, colorA3,
       colorB0, colorB1, colorB2, colorB3,
       colorS1, colorS2;
 DWORD product1a, product1b,
       product2a, product2b;

 line = 0;

  {
   for (; height; height-=1)
	{
     bP = (DWORD *)srcPtr;
	 dP = (DWORD *)(dstBitmap + line*dstPitch);
     for (finish = width; finish; finish -= 1 )
      {
//---------------------------------------    B1 B2
//                                         4  5  6 S2
//                                         1  2  3 S1
//                                           A1 A2
       if(finish==width) iXA=0;
       else              iXA=1;
       if(finish>4) {iXB=1;iXC=2;}
       else
       if(finish>3) {iXB=1;iXC=1;}
       else         {iXB=0;iXC=0;}
       if(line==0) iYA=0;
       else        iYA=width;
       if(height>4) {iYB=width;iYC=width2;}
       else
       if(height>3) {iYB=width;iYC=width;}
       else         {iYB=0;iYC=0;}


       colorB0 = *(bP- iYA - iXA);
       colorB1 = *(bP- iYA);
       colorB2 = *(bP- iYA + iXB);
       colorB3 = *(bP- iYA + iXC);

       color4 = *(bP  - iXA);
       color5 = *(bP);
       color6 = *(bP  + iXB);
       colorS2 = *(bP + iXC);

       color1 = *(bP  + iYB  - iXA);
       color2 = *(bP  + iYB);
       color3 = *(bP  + iYB  + iXB);
       colorS1= *(bP  + iYB  + iXC);

       colorA0 = *(bP + iYC - iXA);
       colorA1 = *(bP + iYC);
       colorA2 = *(bP + iYC + iXB);
       colorA3 = *(bP + iYC + iXC);

//--------------------------------------
       if (color2 == color6 && color5 != color3)
        {
         product2b = product1b = color2;
        }
       else
       if (color5 == color3 && color2 != color6)
        {
         product2b = product1b = color5;
        }
       else
       if (color5 == color3 && color2 == color6)
        {
         register int r = 0;

         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (color1&0x00ffffff),  (colorA1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (color4&0x00ffffff),  (colorB1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (colorA2&0x00ffffff), (colorS1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (colorB2&0x00ffffff), (colorS2&0x00ffffff));

         if (r > 0)
          product2b = product1b = color6;
         else
         if (r < 0)
          product2b = product1b = color5;
         else
          {
           product2b = product1b = INTERPOLATE8(color5, color6);
          }
        }
       else
        {
         if (color6 == color3 && color3 == colorA1 && color2 != colorA2 && color3 != colorA0)
             product2b = Q_INTERPOLATE8 (color3, color3, color3, color2);
         else
         if (color5 == color2 && color2 == colorA2 && colorA1 != color3 && color2 != colorA3)
             product2b = Q_INTERPOLATE8 (color2, color2, color2, color3);
         else
             product2b = INTERPOLATE8 (color2, color3);

         if (color6 == color3 && color6 == colorB1 && color5 != colorB2 && color6 != colorB0)
             product1b = Q_INTERPOLATE8 (color6, color6, color6, color5);
         else
         if (color5 == color2 && color5 == colorB2 && colorB1 != color6 && color5 != colorB3)
             product1b = Q_INTERPOLATE8 (color6, color5, color5, color5);
         else
             product1b = INTERPOLATE8 (color5, color6);
        }

       if (color5 == color3 && color2 != color6 && color4 == color5 && color5 != colorA2)
        product2a = INTERPOLATE8(color2, color5);
       else
       if (color5 == color1 && color6 == color5 && color4 != color2 && color5 != colorA0)
        product2a = INTERPOLATE8(color2, color5);
       else
        product2a = color2;

       if (color2 == color6 && color5 != color3 && color1 == color2 && color2 != colorB2)
        product1a = INTERPOLATE8(color2, color5);
       else
       if (color4 == color2 && color3 == color2 && color1 != color5 && color2 != colorB0)
        product1a = INTERPOLATE8(color2, color5);
       else
        product1a = color5;

       *dP=product1a;
       *(dP+1)=product1b;
       *(dP+(width2))=product2a;
       *(dP+1+(width2))=product2b;

       bP += 1;
       dP += 2;
      }//end of for ( finish= width etc..)

     line += 2;
     srcPtr += srcPitch;
	}; //endof: for (; height; height--)
  }
}
/////////////////////////////////////////////////////////////////////////////

#define colorMask4     0x0000EEE0
#define lowPixelMask4  0x00001110
#define qcolorMask4    0x0000CCC0
#define qlowpixelMask4 0x00003330

#define INTERPOLATE4(A, B) ((((A & colorMask4) >> 1) + ((B & colorMask4) >> 1) + (A & B & lowPixelMask4))|((((A&0x0000000F)==0x00000006)?0x00000006:(((B&0x0000000F)==0x00000006)?0x00000006:(((A&0x0000000F)==0x00000000)?0x00000000:(((B&0x0000000F)==0x00000000)?0x00000000:0x0000000F))))))

#define Q_INTERPOLATE4(A, B, C, D) ((((A & qcolorMask4) >> 2) + ((B & qcolorMask4) >> 2) + ((C & qcolorMask4) >> 2) + ((D & qcolorMask4) >> 2) + ((((A & qlowpixelMask4) + (B & qlowpixelMask4) + (C & qlowpixelMask4) + (D & qlowpixelMask4)) >> 2) & qlowpixelMask4))| ((((A&0x0000000F)==0x00000006)?0x00000006:(((B&0x0000000F)==0x00000006)?0x00000006:(((C&0x0000000F)==0x00000006)?0x00000006:(((D&0x0000000F)==0x00000006)?0x00000006:(((A&0x0000000F)==0x00000000)?0x00000000:(((B&0x0000000F)==0x00000000)?0x00000000:(((C&0x0000000F)==0x00000000)?0x00000000:(((D&0x0000000F)==0x00000000)?0x00000000:0x0000000F))))))))))


#define colorMask5     0x0000F7BC
#define lowPixelMask5  0x00000842
#define qcolorMask5    0x0000E738
#define qlowpixelMask5 0x000018C6

#define INTERPOLATE5(A, B) ((((A & colorMask5) >> 1) + ((B & colorMask5) >> 1) + (A & B & lowPixelMask5))|((((A&0x00000001)==0x00000000)?0x00000000:(((B&0x00000001)==0x00000000)?0x00000000:0x00000001))))

#define Q_INTERPOLATE5(A, B, C, D) ((((A & qcolorMask5) >> 2) + ((B & qcolorMask5) >> 2) + ((C & qcolorMask5) >> 2) + ((D & qcolorMask5) >> 2) + ((((A & qlowpixelMask5) + (B & qlowpixelMask5) + (C & qlowpixelMask5) + (D & qlowpixelMask5)) >> 2) & qlowpixelMask5))| ((((A&0x00000001)==0x00000000)?0x00000000:(((B&0x00000001)==0x00000000)?0x00000000:(((C&0x00000001)==0x00000000)?0x00000000:(((D&0x00000001)==0x00000000)?0x00000000:0x00000001))))))
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// ogl texture defines
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void DefineSubTextureSortHiRes(void)
{
    gTexName=gTexMovieName;
    
    if(!gTexName) {
        s8 clampType = gClampType;
        s8 filter;
        if(iFilterType) {
            filter = 1;
        } else {
            filter = gFilter;
        }
        gTexName = createTexture(filter, clampType);
        
        
        //bindTexture(gTexName);
        setTexture(gTexName, 512, 512, 3, texturebuffer);
        //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, texturebuffer);glError();
    }
    //else  { bindTexture(gTexName); }
    
    setSubTexture(gTexName, XTexS<<1, YTexS<<1, DXTexS<<1, DYTexS<<1, 3, texturebuffer); //glError();
    //LOGE("DefineSubTextureSortHiRes x:%d y:%d",XTexS<<1,YTexS<<1);
}

/////////////////////////////////////////////////////////////////////////////

void DefineSubTextureSort(void)
{
    logInfo(TAG, "DefineSubTextureSort %d", gTexName);
    if(!gTexName) {
        s8 clampType = gClampType;
        s8 filter;
        if(iFilterType) {
            filter = 1;
        } else {
            filter = gFilter;
        }
        logInfo(TAG, "createTexture");
        gTexName = createTexture(filter, clampType);
        //bindTexture(gTexName);
        setTexture(gTexName, 256, 256, 3, texturepart);
        //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0,GL_RGBA, GL_UNSIGNED_BYTE, texturepart);glError();
    } /*else {
        bindTexture(gTexName);
    }*/
                                        //LOGE("DefineSubTextureSort x:%d y:%d w:%d h:%d",XTexS,YTexS,DXTexS,DYTexS);

    setSubTexture(gTexName, XTexS, YTexS, DXTexS, DYTexS, 3, texturepart);//glError();
}

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// texture cache garbage collection
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void DoTexGarbageCollection(void)
{
 static unsigned short LRUCleaned=0;
 unsigned short iC,iC1,iC2;
 int i,j,iMax;textureSubCacheEntryS * tsb;

 iC=4;//=iSortTexCnt/2,
 LRUCleaned+=iC;                                       // we clean different textures each time
 if((LRUCleaned+iC)>=iSortTexCnt) LRUCleaned=0;        // wrap? wrap!
 iC1=LRUCleaned;                                       // range of textures to clean
 iC2=LRUCleaned+iC;

 for(iC=iC1;iC<iC2;iC++)                               // make some textures available
  {
   pxSsubtexLeft[iC]->l=0;
  }

 for(i=0;i<3;i++)                                      // remove all references to that textures
  for(j=0;j<MAXTPAGES;j++)
   for(iC=0;iC<4;iC++)                                 // loop all texture rect info areas
    {
     tsb=pscSubtexStore[i][j]+(iC*SOFFB);
     iMax=tsb->pos.l;
     if(iMax)
      do
       {
        tsb++;
        if(tsb->cTexID>=iC1 && tsb->cTexID<iC2)        // info uses the cleaned textures? remove info
         tsb->ClutID=0;
       } 
      while(--iMax);
     }

 usLRUTexPage=LRUCleaned;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// search cache for existing (already used) parts
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

unsigned char * CheckTextureInSubSCache(long TextureMode,unsigned long GivenClutId,unsigned short * pCache)
{
 textureSubCacheEntryS * tsx, * tsb, *tsg;//, *tse=NULL;
 int i,iMax;EXLong npos;
 unsigned char cx,cy;
 int iC,j,k;unsigned long rx,ry,mx,my;
 EXLong * ul=0, * uls;
 EXLong rfree;
 unsigned char cXAdj,cYAdj;

 npos.l=*((unsigned long *)&gl_ux[4]);

 //--------------------------------------------------------------//
 // find matching texturepart first... speed up...
 //--------------------------------------------------------------//

 tsg=pscSubtexStore[TextureMode][GlobalTexturePage];
 tsg+=((GivenClutId&CLUTCHK)>>CLUTSHIFT)*SOFFB;

 iMax=tsg->pos.l;
 if(iMax)
  {
   i=iMax;
   tsb=tsg+1;                 
   do
    {
     if(GivenClutId==tsb->ClutID &&
        (INCHECK(tsb->pos,npos)))
      {
        {
         cx=tsb->pos.c[3]-tsb->posTX;
         cy=tsb->pos.c[1]-tsb->posTY;

         gl_ux[0]-=cx;
         gl_ux[1]-=cx;
         gl_ux[2]-=cx;
         gl_ux[3]-=cx;
         gl_vy[0]-=cy;
         gl_vy[1]-=cy;
         gl_vy[2]-=cy;
         gl_vy[3]-=cy;

         ubOpaqueDraw=tsb->Opaque;
         *pCache=tsb->cTexID;
         return NULL;
        }
      } 
     tsb++;
    }
   while(--i);
  }

 //----------------------------------------------------//

 cXAdj=1;cYAdj=1;

 rx=(int)gl_ux[6]-(int)gl_ux[7];
 ry=(int)gl_ux[4]-(int)gl_ux[5];

 tsx=NULL;tsb=tsg+1;
 for(i=0;i<iMax;i++,tsb++)
  {
   if(!tsb->ClutID) {tsx=tsb;break;}
  }

 if(!tsx) 
  {
   iMax++;
   if(iMax>=SOFFB-2) 
    {
     if(iTexGarbageCollection)                         // gc mode?
      {
       if(*pCache==0) 
        {
         dwTexPageComp|=(1<<GlobalTexturePage);
         *pCache=0xffff;
         return 0;
        }

       iMax--;
       tsb=tsg+1;

       for(i=0;i<iMax;i++,tsb++)                       // 1. search other slots with same cluts, and unite the area
        if(GivenClutId==tsb->ClutID)
         {
          if(!tsx) {tsx=tsb;rfree.l=npos.l;}           // 
          else      tsb->ClutID=0;
          rfree.c[3]=min(rfree.c[3],tsb->pos.c[3]);
          rfree.c[2]=max(rfree.c[2],tsb->pos.c[2]);
          rfree.c[1]=min(rfree.c[1],tsb->pos.c[1]);
          rfree.c[0]=max(rfree.c[0],tsb->pos.c[0]);
          MarkFree(tsb);
         }

       if(tsx)                                         // 3. if one or more found, create a new rect with bigger size
        {
         *((unsigned long *)&gl_ux[4])=npos.l=rfree.l;
         rx=(int)rfree.c[2]-(int)rfree.c[3];
         ry=(int)rfree.c[0]-(int)rfree.c[1];
         DoTexGarbageCollection();
       
         goto ENDLOOP3;
        }
      }

     iMax=1;
    }
   tsx=tsg+iMax;
   tsg->pos.l=iMax;
  }

 //----------------------------------------------------//
 // now get a free texture space
 //----------------------------------------------------//

 if(iTexGarbageCollection) usLRUTexPage=0;

ENDLOOP3:

 rx+=3;if(rx>255) {cXAdj=0;rx=255;}
 ry+=3;if(ry>255) {cYAdj=0;ry=255;}

 iC=usLRUTexPage;

 for(k=0;k<iSortTexCnt;k++)
  {
   uls=pxSsubtexLeft[iC];
   iMax=uls->l;ul=uls+1;

   //--------------------------------------------------//
   // first time

   if(!iMax) 
    {
     rfree.l=0;

     if(rx>252 && ry>252)
      {uls->l=1;ul->l=0xffffffff;ul=0;goto ENDLOOP;}

     if(rx<253)
      {
       uls->l=uls->l+1;
       ul->c[3]=rx;
       ul->c[2]=255-rx;
       ul->c[1]=0;
       ul->c[0]=ry;
       ul++;
      }

     if(ry<253)
      {
       uls->l=uls->l+1; 
       ul->c[3]=0;
       ul->c[2]=255;
       ul->c[1]=ry;
       ul->c[0]=255-ry;
      }
     ul=0;
     goto ENDLOOP;
    }
                                                       
   //--------------------------------------------------//
   for(i=0;i<iMax;i++,ul++)
    {
     if(ul->l!=0xffffffff && 
        ry<=ul->c[0]      && 
        rx<=ul->c[2])
      {
       rfree=*ul;
       mx=ul->c[2]-2;
       my=ul->c[0]-2;
       if(rx<mx && ry<my)
        {
         ul->c[3]+=rx;
         ul->c[2]-=rx;
         ul->c[0]=ry;

         for(ul=uls+1,j=0;j<iMax;j++,ul++)
          if(ul->l==0xffffffff) break;
 
         if(j<CSUBSIZE-2)
          {
           if(j==iMax) uls->l=uls->l+1;

           ul->c[3]=rfree.c[3];
           ul->c[2]=rfree.c[2];
           ul->c[1]=rfree.c[1]+ry;
           ul->c[0]=rfree.c[0]-ry;
          }
        }
       else if(rx<mx)
        {
         ul->c[3]+=rx;
         ul->c[2]-=rx;
        }
       else if(ry<my)
        {
         ul->c[1]+=ry;
         ul->c[0]-=ry;
        }
       else
        {
         ul->l=0xffffffff;
        }
       ul=0;
       goto ENDLOOP;
      }
    }

   //--------------------------------------------------//

   iC++; if(iC>=iSortTexCnt) iC=0;
  }

 //----------------------------------------------------//
 // check, if free space got
 //----------------------------------------------------//

ENDLOOP:
 if(ul)
  {
   //////////////////////////////////////////////////////

    {
     dwTexPageComp=0;

     for(i=0;i<3;i++)                                    // cleaning up
      for(j=0;j<MAXTPAGES;j++)
       {
        tsb=pscSubtexStore[i][j];
        (tsb+SOFFA)->pos.l=0;
        (tsb+SOFFB)->pos.l=0;
        (tsb+SOFFC)->pos.l=0;
        (tsb+SOFFD)->pos.l=0;
       }
     for(i=0;i<iSortTexCnt;i++)
      {ul=pxSsubtexLeft[i];ul->l=0;}
     usLRUTexPage=0;
    }

   //////////////////////////////////////////////////////
   iC=usLRUTexPage;
   uls=pxSsubtexLeft[usLRUTexPage];
   uls->l=0;ul=uls+1;
   rfree.l=0;

   if(rx>252 && ry>252)
    {uls->l=1;ul->l=0xffffffff;}
   else
    {
     if(rx<253)
      {
       uls->l=uls->l+1;
       ul->c[3]=rx;
       ul->c[2]=255-rx;
       ul->c[1]=0;
       ul->c[0]=ry;
       ul++;
      }
     if(ry<253)
      {
       uls->l=uls->l+1; 
       ul->c[3]=0;
       ul->c[2]=255;
       ul->c[1]=ry;
       ul->c[0]=255-ry;
      }
    }
   tsg->pos.l=1;tsx=tsg+1;
  }

 rfree.c[3]+=cXAdj;
 rfree.c[1]+=cYAdj;

 tsx->cTexID   =*pCache=iC;
 tsx->pos      = npos;
 tsx->ClutID   = GivenClutId;
 tsx->posTX    = rfree.c[3];
 tsx->posTY    = rfree.c[1];

 cx=gl_ux[7]-rfree.c[3];
 cy=gl_ux[5]-rfree.c[1];

 gl_ux[0]-=cx;
 gl_ux[1]-=cx;
 gl_ux[2]-=cx;
 gl_ux[3]-=cx;
 gl_vy[0]-=cy;
 gl_vy[1]-=cy;
 gl_vy[2]-=cy;
 gl_vy[3]-=cy;

 XTexS=rfree.c[3];
 YTexS=rfree.c[1];

 return &tsx->Opaque;
}
                   
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// search cache for free place (on compress)
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BOOL GetCompressTexturePlace(textureSubCacheEntryS * tsx)
{
 int i,j,k,iMax,iC;unsigned long rx,ry,mx,my;
 EXLong * ul=0, * uls, rfree;
 unsigned char cXAdj=1,cYAdj=1;

 rx=(int)tsx->pos.c[2]-(int)tsx->pos.c[3];
 ry=(int)tsx->pos.c[0]-(int)tsx->pos.c[1];

 rx+=3;if(rx>255) {cXAdj=0;rx=255;}
 ry+=3;if(ry>255) {cYAdj=0;ry=255;}

 iC=usLRUTexPage;

 for(k=0;k<iSortTexCnt;k++)
  {
   uls=pxSsubtexLeft[iC];
   iMax=uls->l;ul=uls+1;

   //--------------------------------------------------//
   // first time

   if(!iMax)
    {
     rfree.l=0;

     if(rx>252 && ry>252)
      {uls->l=1;ul->l=0xffffffff;ul=0;goto TENDLOOP;}

     if(rx<253)
      {
       uls->l=uls->l+1;
       ul->c[3]=rx;
       ul->c[2]=255-rx;
       ul->c[1]=0;
       ul->c[0]=ry;
       ul++;
      }

     if(ry<253)
      {
       uls->l=uls->l+1;
       ul->c[3]=0;
       ul->c[2]=255;
       ul->c[1]=ry;
       ul->c[0]=255-ry;
      }
     ul=0;
     goto TENDLOOP;
    }

   //--------------------------------------------------//
   for(i=0;i<iMax;i++,ul++)
    {
     if(ul->l!=0xffffffff &&
        ry<=ul->c[0]      &&
        rx<=ul->c[2])
      {
       rfree=*ul;
       mx=ul->c[2]-2;
       my=ul->c[0]-2;

       if(rx<mx && ry<my)
        {
         ul->c[3]+=rx;
         ul->c[2]-=rx;
         ul->c[0]=ry;

         for(ul=uls+1,j=0;j<iMax;j++,ul++)
          if(ul->l==0xffffffff) break;

         if(j<CSUBSIZE-2)
          {
           if(j==iMax) uls->l=uls->l+1;

           ul->c[3]=rfree.c[3];
           ul->c[2]=rfree.c[2];
           ul->c[1]=rfree.c[1]+ry;
           ul->c[0]=rfree.c[0]-ry;
          }
        }
       else if(rx<mx)
        {
         ul->c[3]+=rx;
         ul->c[2]-=rx;
        }
       else if(ry<my)
        {
         ul->c[1]+=ry;
         ul->c[0]-=ry;
        }
       else
        {
         ul->l=0xffffffff;
        }
       ul=0;
       goto TENDLOOP;
      }
    }

   //--------------------------------------------------//

   iC++; if(iC>=iSortTexCnt) iC=0;
  }

 //----------------------------------------------------//
 // check, if free space got
 //----------------------------------------------------//

TENDLOOP:
 if(ul) return FALSE;

 rfree.c[3]+=cXAdj;
 rfree.c[1]+=cYAdj;

 tsx->cTexID   = iC;
 tsx->posTX    = rfree.c[3];
 tsx->posTY    = rfree.c[1];

 XTexS=rfree.c[3];
 YTexS=rfree.c[1];

 return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// compress texture cache (to make place for new texture part, if needed)
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void CompressTextureSpace(void)
{
 textureSubCacheEntryS * tsx, * tsg, * tsb;
 int i,j,k,m,n,iMax;EXLong * ul, r,opos;
 short sOldDST=DrawSemiTrans,cx,cy;
 long  lOGTP=GlobalTexturePage;
 unsigned long l,row;
 unsigned long * lSRCPtr;

 opos.l=*((unsigned long *)&gl_ux[4]);

 // 1. mark all textures as free
 for(i=0;i<iSortTexCnt;i++)
  {ul=pxSsubtexLeft[i];ul->l=0;}
 usLRUTexPage=0;

 // 2. compress
 for(j=0;j<3;j++)
  {
   for(k=0;k<MAXTPAGES;k++)
    {
     tsg=pscSubtexStore[j][k];

     if((!(dwTexPageComp&(1<<k))))
      {
       (tsg+SOFFA)->pos.l=0;
       (tsg+SOFFB)->pos.l=0;
       (tsg+SOFFC)->pos.l=0;
       (tsg+SOFFD)->pos.l=0;
       continue;
      }

     for(m=0;m<4;m++,tsg+=SOFFB)
      {
       iMax=tsg->pos.l;

       tsx=tsg+1;
       for(i=0;i<iMax;i++,tsx++)
        {
         if(tsx->ClutID)
          {
           r.l=tsx->pos.l;
           for(n=i+1,tsb=tsx+1;n<iMax;n++,tsb++)
            {
             if(tsx->ClutID==tsb->ClutID)
              {
               r.c[3]=min(r.c[3],tsb->pos.c[3]);
               r.c[2]=max(r.c[2],tsb->pos.c[2]);
               r.c[1]=min(r.c[1],tsb->pos.c[1]);
               r.c[0]=max(r.c[0],tsb->pos.c[0]);
               tsb->ClutID=0;
              }
            }

//           if(r.l!=tsx->pos.l)
            {
             cx=((tsx->ClutID << 4) & 0x3F0);          
             cy=((tsx->ClutID >> 6) & CLUTYMASK);

             if(j!=2)
              {
               // palette check sum
               l=0;lSRCPtr=(unsigned long *)(psxVuw+cx+(cy*1024));
               if(j==1) for(row=1;row<129;row++) l+=((*lSRCPtr++)-1)*row;
               else     for(row=1;row<9;row++)   l+=((*lSRCPtr++)-1)<<row;
               l=((l+HIWORD(l))&0x3fffL)<<16;
               if(l!=(tsx->ClutID&(0x00003fff<<16)))
                {
                 tsx->ClutID=0;continue;
                }
              }

             tsx->pos.l=r.l;
             if(!GetCompressTexturePlace(tsx))         // no place?
              {
               for(i=0;i<3;i++)                        // -> clean up everything
                for(j=0;j<MAXTPAGES;j++)
                 {
                  tsb=pscSubtexStore[i][j];
                  (tsb+SOFFA)->pos.l=0;
                  (tsb+SOFFB)->pos.l=0;
                  (tsb+SOFFC)->pos.l=0;
                  (tsb+SOFFD)->pos.l=0;
                 }
               for(i=0;i<iSortTexCnt;i++)
                {ul=pxSsubtexLeft[i];ul->l=0;}
               usLRUTexPage=0;
               DrawSemiTrans=sOldDST;
               GlobalTexturePage=lOGTP;
               *((unsigned long *)&gl_ux[4])=opos.l;
               dwTexPageComp=0;

               return;
              }

             if(tsx->ClutID&(1<<30)) DrawSemiTrans=1;
             else                    DrawSemiTrans=0;
             *((unsigned long *)&gl_ux[4])=r.l;
   
             gTexName=uiStexturePage[tsx->cTexID];
             LoadSubTexFn(k,j,cx,cy);
             uiStexturePage[tsx->cTexID]=gTexName;
             tsx->Opaque=ubOpaqueDraw;
            }
          }
        }

       if(iMax)  
        {
         tsx=tsg+iMax;
         while(!tsx->ClutID && iMax) {tsx--;iMax--;}
         tsg->pos.l=iMax;
        }

      }                      
    }
  }

 if(dwTexPageComp==0xffffffff) dwTexPageComp=0;

 *((unsigned long *)&gl_ux[4])=opos.l;
 GlobalTexturePage=lOGTP;
 DrawSemiTrans=sOldDST;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// main entry for searching/creating textures, called from prim.c
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

u32 SelectSubTextureS(long TextureMode, unsigned long GivenClutId)
{
 unsigned char * OPtr;unsigned short iCache;short cx,cy;

 // sort sow/tow infos for fast access

 unsigned char ma1,ma2,mi1,mi2;
 if(gl_ux[0]>gl_ux[1]) {mi1=gl_ux[1];ma1=gl_ux[0];}
 else                  {mi1=gl_ux[0];ma1=gl_ux[1];}
 if(gl_ux[2]>gl_ux[3]) {mi2=gl_ux[3];ma2=gl_ux[2];}
 else                  {mi2=gl_ux[2];ma2=gl_ux[3];}
 if(mi1>mi2) gl_ux[7]=mi2; 
 else        gl_ux[7]=mi1;
 if(ma1>ma2) gl_ux[6]=ma1; 
 else        gl_ux[6]=ma2;

 if(gl_vy[0]>gl_vy[1]) {mi1=gl_vy[1];ma1=gl_vy[0];}
 else                  {mi1=gl_vy[0];ma1=gl_vy[1];}
 if(gl_vy[2]>gl_vy[3]) {mi2=gl_vy[3];ma2=gl_vy[2];}
 else                  {mi2=gl_vy[2];ma2=gl_vy[3];}
 if(mi1>mi2) gl_ux[5]=mi2; 
 else        gl_ux[5]=mi1;
 if(ma1>ma2) gl_ux[4]=ma1; 
 else        gl_ux[4]=ma2;
//LOGE("Select subtexture TextureMode:%d",TextureMode);
 // get clut infos in one 32 bit val

 if(TextureMode==2)                                    // no clut here
  {
   GivenClutId=CLUTUSED|(DrawSemiTrans<<30);cx=cy=0;
 
   if(iFrameTexType && Fake15BitTexture()) 
    return (u32)gTexName;
  }           
 else 
  {
   cx=((GivenClutId << 4) & 0x3F0);                    // but here
   cy=((GivenClutId >> 6) & CLUTYMASK);
   GivenClutId=(GivenClutId&CLUTMASK)|(DrawSemiTrans<<30)|CLUTUSED;

   // palette check sum.. removed MMX asm, this easy func works as well
    {
     unsigned long l=0,row;

     unsigned long * lSRCPtr=(unsigned long *)(psxVuw+cx+(cy*1024));
     if(TextureMode==1) for(row=1;row<129;row++) l+=((*lSRCPtr++)-1)*row;
     else               for(row=1;row<9;row++)   l+=((*lSRCPtr++)-1)<<row;
     l=(l+HIWORD(l))&0x3fffL;
     GivenClutId|=(l<<16);
    }

  }

 // search cache
 iCache=0;
 OPtr=CheckTextureInSubSCache(TextureMode,GivenClutId,&iCache);

 // cache full? compress and try again
 if(iCache==0xffff)
  {
   CompressTextureSpace();
   OPtr=CheckTextureInSubSCache(TextureMode,GivenClutId,&iCache);
  }

 // found? fine
 usLRUTexPage=iCache;
 if(!OPtr) return uiStexturePage[iCache];
 // not found? upload texture and store infos in cache
 gTexName=uiStexturePage[iCache];
 //LOGE("Load New subtexture","TextureMode:%d cx:%d cy:%d ",TextureMode,cx,cy);

 LoadSubTexFn(GlobalTexturePage,TextureMode,cx,cy);
 uiStexturePage[iCache]=gTexName;
 *OPtr=ubOpaqueDraw;
 return (u32) gTexName;
}


