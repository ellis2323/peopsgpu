/***************************************************************************
                            gpu.h  -  description
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

#ifndef _GPU_PLUGIN_H
#define _GPU_PLUGIN_H

/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

#define PRED(x)   ((x << 3) & 0xF8)
#define PBLUE(x)  ((x >> 2) & 0xF8)
#define PGREEN(x) ((x >> 7) & 0xF8)

#define RED(x) (x & 0xff)
#define BLUE(x) ((x>>16) & 0xff)
#define GREEN(x) ((x>>8) & 0xff)
#define COLOR(x) (x & 0xffffff)


#include "gpuExternals.h"
#include "gpuPlugin.h"

/////////////////////////////////////////////////////////////////////////////

#define CALLBACK

typedef struct {
        unsigned int ulFreezeVersion;
        unsigned int ulStatus;
        unsigned int ulControl[256];
        unsigned char psxVRam[1024*1024*2];
} GPUFreeze_t;

long CALLBACK GPU_init();
long CALLBACK GPU_shutdown();
long CALLBACK GPU_open(int hwndGPU);                    
long CALLBACK GPU_close();
unsigned long CALLBACK GPU_readData(void);
void CALLBACK GPU_readDataMem(unsigned long * pMem, int iSize);
unsigned long CALLBACK GPU_readStatus(void);
void CALLBACK GPU_writeData(unsigned long gdata);
void CALLBACK GPU_writeDataMem(unsigned long * pMem, int iSize);
void CALLBACK GPU_writeStatus(unsigned long gdata);
long CALLBACK GPU_dmaChain(unsigned long * baseAddrL, unsigned long addr);
void CALLBACK GPU_updateLace(void);
void CALLBACK GPU_makeSnapshot(void);
long CALLBACK GPU_freeze(unsigned long ulGetFreezeData,GPUFreeze_t * pF);
long CALLBACK GPU_getScreenPic(unsigned char * pMem);
long CALLBACK GPU_showScreenPic(unsigned char * pMem);
//void CALLBACK GPU_keypressed(int keycode);
//void CALLBACK GPU_displayText(char * pText);
//void CALLBACK GPU_clearDynarec(void (CALLBACK *callback)(void));
long CALLBACK GPU_configure(void);
long CALLBACK GPU_test(void);
void CALLBACK GPU_about(void);


void           DoSnapShot(void);
void               GPU_vSinc(void);
void           updateDisplay(void);
void           updateFrontDisplay(void);
void           SetAspectRatio(void);
void           CheckVRamRead(int x, int y, int dx, int dy, bool bFront);
void           CheckVRamReadEx(int x, int y, int dx, int dy);
void           SetFixes(void);

void PaintPicDot(unsigned char * p,unsigned char c);
//void DrawNumBorPic(unsigned char *pMem, int lSelectedSlot);
void ResizeWindow();
void flipEGL();
const char *getShaderPath();

extern int UseFrameLimit;
extern int UseFrameSkip;
extern float fFrameRate;
extern int iFrameLimit;
extern float fFrameRateHz;
extern float fps_skip;
extern float fps_cur;
////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif


#endif // _GPU_INTERNALS_H

