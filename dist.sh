#!/bin/bash -x


# OGLES 1
ogles1() {
    mkdir -p dist/ogles1
    cp templates/ogles1/Application.mk jni/
    ndk-build clean
    ndk-build 
    cp libs/armeabi/libopenglplugin.so dist/ogles1/
    return
}

# OGLES 2
ogles2() {
    mkdir -p dist/ogles2
    cp templates/ogles2/Application.mk jni/
    ndk-build clean
    ndk-build 
    cp libs/armeabi/libopenglplugin.so dist/ogles2/
    cp assets/*.fsh dist/ogles2/
    cp assets/*.vsh dist/ogles2/
}

DATE=`date +%Y-%m-%d`
tar zcvf plugins_${DATE}.tgz dist/


