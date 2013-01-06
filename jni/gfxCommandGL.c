#include "gfxCommand.h"

#define TAG "ELLIS"

#if defined(GL_OGLES2) || defined(GL_OGLES1)

u32 sCurrentBlendingMode = 7;
E_TRIBOOL_TYPE sBlendingState = TRIBOOL_UNKNOWN;
bool sDebugCmdFlag = false;
E_CMD_TYPE sDebugCmdType = CMD_TYPE_NOOP;


void debugCommand(bool flag) {
    if (flag) { logInfo(TAG, "debug: TRUE"); } else { logInfo(TAG, "debug: FALSE"); }
    sDebugCmdFlag = flag;
}

void changeDebuggedCommand() {
    sDebugCmdType++;
    sDebugCmdType %= (s32)CMD_TYPE_COUNT;
    switch (sDebugCmdType) {
        case CMD_TYPE_NOOP:
            logInfo(TAG, "No command selected");
            break;
        case CMD_TYPE_POINT_OPAQUE:
            logInfo(TAG, "POINT OPAQUE selected");
            break;
        case CMD_TYPE_POINT_TRANS:
            logInfo(TAG, "POINT TRANS selected");
            break;
        case CMD_TYPE_LINE_OPAQUE:
            logInfo(TAG, "LINE OPAQUE selected");
            break;
        case CMD_TYPE_LINE_TRANS:
            logInfo(TAG, "LINE TRANS selected");
            break;
        case CMD_TYPE_TRI_OPAQUE:
            logInfo(TAG, "OPAQUE TRIANGLE selected");
            break;
        case CMD_TYPE_TRI_TRANS:
            logInfo(TAG, "TRANS TRIANGLE selected");
            break;
        case CMD_TYPE_TEXTRI_OPAQUE:
            logInfo(TAG, "TEXTURED OPAQUE TRIANGLE selected");
            break;
        case CMD_TYPE_TEXTRI_TRANS:
            logInfo(TAG, "TEXTURED TRANS TRIANGLE selected");
            break;
        case CMD_TYPE_GOUTEXTRI_OPAQUE:
            logInfo(TAG, "GOURAUD TEXTURED OPAQUE TRIANGLE selected");
            break;
        case CMD_TYPE_GOUTEXTRI_TRANS:
            logInfo(TAG, "GOURAUD TEXTURED TRANS TRIANGLE selected");
            break;
        default:
            logInfo(TAG, "Unkown command selected %d", sDebugCmdType);
    }
}

bool isDebuggedCommand(E_CMD_TYPE type) {
    if (!sDebugCmdFlag) return false;
    if (type==sDebugCmdType) return true;
    return false;
}



#endif // GL_OGLES2 or GL_OGLES1