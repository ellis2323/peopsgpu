#include "gfxCommon.h"
#include <math.h>

#define TAG "ELLIS"

void initVertex(GLSLVertex *v) {
    v->x = 0; v->y = 0; v->z = 0;
    v->sow = 0; v->tow = 0;
    v->c.col[0] = 0; v->c.col[1] = 0; v->c.col[2] = 0; v->c.col[3] = 0;
}

void initVertices(GLSLVertex *v, s32 count) {
    s32 i = 0;
    for (i=0; i<count; ++i) {
        initVertex(&v[i]);
    }
}

static void floatV(c8 *res, f32 value) {
    s32 d = (s32)(value);
    f32 integral;
    f32 fract = modff(value, &integral);
    s32 f = (s32)(fract * 1000);
    sprintf(res, "%4d.%03d", d, f);
}

static void unitV(c8 *res, f32 value) {
    f32 integral;
    f32 fract = modff(value, &integral);
    s32 f = (s32)(fract * 1000);
    sprintf(res, ".%03d", f);
}

void logVertex(const GLSLVertex *v) {
    c8 vx[20]; floatV(vx, v->x);
    c8 vy[20]; floatV(vy, v->y);
    c8 vz[20]; floatV(vz, v->z);
    c8 vu[20]; unitV(vu, v->sow);
    c8 vv[20]; unitV(vv, v->tow);
    logInfo(TAG, "x:%s y:%s z:%s u:%s v:%s r:%d g:%d b:%d a:%d", vx, vy, vz, vu, vv, v->c.col[0], v->c.col[1], v->c.col[2], v->c.col[3]);
}
