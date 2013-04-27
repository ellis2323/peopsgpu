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

void logProjection(const c8 *msg, f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    c8 vl[20]; floatV(vl, left);
    c8 vr[20]; floatV(vr, right);
    c8 vb[20]; floatV(vb, bottom);
    c8 vt[20]; floatV(vt, top);
    c8 vn[20]; floatV(vn, near);
    c8 vf[20]; floatV(vf, far);
    logInfo(TAG, "%s: %s %s %s %s %s %s", msg, vl, vr, vb, vt, vn ,vf);
}
