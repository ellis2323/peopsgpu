#pragma once
#ifndef GPU_FPS_H
#define GPU_FPS_H
void PCFrameCap (void);
void PCcalcfps(void);
void CheckFrameRate(void);
void InitFPS(void);
void FrameSkip(void);
unsigned long timeGetTime(void);
void SetAutoFrameCap(void);

#endif // GPU_FPS_H