/*****************************************************************

   FPSE Plugin: OpenGL Plugin for FPSE
   This file is copyright (c) 2012-2013 Laurent Mallet.
   License: GPL v3.  See License.txt.

   Created by Mallet Laurent on 5/01/13.
   Copyright (c) 2013 Mallet Laurent. All rights reserved.
   
   Original Plugin written by Pete Bernert

*******************************************************************/

#include "gfxInput.h"
#include "gfxCommand.h"

#define TAG "ELLIS"

bool flag = false;

void keyPressed(int key, int val) {
#if DEBUG
    if (key==KEY_L3 && val!=0) changeDebuggedCommand();
    if (key==KEY_R3 && val!=0) { flag = !flag; debugCommand(flag); }
#endif
}