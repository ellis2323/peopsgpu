//
//  gpuInput.c
//  fpse
//
//  Created by Mallet Laurent on 05/01/13.
//  Copyright (c) 2013 Mallet Laurent. All rights reserved.
//

#include "gpuInput.h"
#include "gfxCommand.h"

#define TAG "ELLIS"

bool flag = false;

void keyPressed(int key, int val) {
#if DEBUG
    if (key==KEY_L3 && val!=0) changeDebuggedCommand();
    if (key==KEY_R3 && val!=0) { flag = !flag; debugCommand(flag); }
#endif
}