#pragma once
#ifndef GFX_GL_H
#define GFX_GL_H

/**********************************************************************

   FPSE Plugin: OpenGL Plugin for FPSE
   This file is copyright (c) 2012-2013 Laurent Mallet.
   License: GPL v3.  See License.txt.

   Created by Mallet Laurent on 17/01/13.
   Copyright (c) 2013 Mallet Laurent. All rights reserved.
   
   Original Plugin written by Pete Bernert

*******************************************************************/


#ifdef ANDROID
    
    #ifdef GL_OGLES1
    #define GL_GLEXT_PROTOTYPES
    #include <GLES/gl.h>
    #include <GLES/glext.h>
    #endif
    
    #ifdef GL_OGLES2
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>
    #endif

#else

#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

#endif

#endif // GFX_GL_H
