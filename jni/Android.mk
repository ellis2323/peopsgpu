LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_MODULE := openglplugin

LOCAL_C_INCLUDES :=

ifeq ($(APP_GL_ARCH),ogles1)
LOCAL_CFLAGS := -DGL_OGLES1
endif
ifeq ($(APP_GL_ARCH),ogles2)
LOCAL_CFLAGS := -DGL_OGLES2
endif
ifeq ($(APP_GL_ARCH),nogl)
LOCAL_CFLAGS := -DGL_NOGL
endif

 
LOCAL_SRC_FILES :=  gpuDraw.c gpuFPS.c \
                    gpuPrim.c gpuPlugin.c \
                    gpupsemu.c gpusoft.c gpuTexture.c  \
                    gpuCommon.c gpuShader.c gpuInput.c gpuContext.c \
                    gfxCommandGL.c gfxCommandGLES2.c gfxCommandGLES1.c gfxCommandLog.c \
					rmalloc.c

ifeq ($(APP_GL_ARCH),ogles1)
LOCAL_LDLIBS := -lGLESv1_CM -ldl -llog
endif
ifeq ($(APP_GL_ARCH),ogles2)
LOCAL_LDLIBS := -lGLESv2 -ldl -llog
endif
ifeq ($(APP_GL_ARCH),nogl)
LOCAL_LDLIBS := -ldl -llog
endif

include $(BUILD_SHARED_LIBRARY)  


