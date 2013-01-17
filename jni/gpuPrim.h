/***************************************************************************
                          prim.h  -  description
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

#ifndef _PRIMDRAW_H_
#define _PRIMDRAW_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "gpuExternals.h"
#include "gpuStdafx.h"

void UploadScreen (long Position);
void PrepareFullScreenUpload (long Position);
BOOL CheckAgainstScreen(short imageX0,short imageY0,short imageX1,short imageY1);
BOOL CheckAgainstFrontScreen(short imageX0,short imageY0,short imageX1,short imageY1);
BOOL FastCheckAgainstScreen(short imageX0,short imageY0,short imageX1,short imageY1);
BOOL FastCheckAgainstFrontScreen(short imageX0,short imageY0,short imageX1,short imageY1);
BOOL bCheckFF9G4(unsigned char * baseAddr);
void SetScanTrans(void);
void SetScanTexTrans(void);
void DrawMultiBlur(void);
void CheckWriteUpdate();

void offsetPSXLine(void);
void offsetPSX2(void);
void offsetPSX3(void);
void offsetPSX4(void);

void FillSoftwareAreaTrans(short x0,short y0,short x1,short y1,unsigned short col);
void FillSoftwareArea(short x0,short y0,short x1,short y1,unsigned short col);
void drawPoly3G(long rgb1, long rgb2, long rgb3);
void drawPoly4G(long rgb1, long rgb2, long rgb3, long rgb4);
void drawPoly3F(long rgb);
void drawPoly4F(long rgb);
void drawPoly4FT(unsigned char * baseAddr);
void drawPoly4GT(unsigned char * baseAddr);
void drawPoly3FT(unsigned char * baseAddr);
void drawPoly3GT(unsigned char * baseAddr);
void DrawSoftwareSprite(unsigned char * baseAddr,short w,short h,long tx,long ty);
void DrawSoftwareSpriteTWin(unsigned char * baseAddr,long w,long h);
void DrawSoftwareSpriteMirror(unsigned char * baseAddr,long w,long h);

#ifdef __cplusplus
}
#endif

#endif // _PRIMDRAW_H_
