#include <stdio.h>
#include <string.h>

#define DECLARE_GPU_VARS
//#include "fpse.h"
#include "type.h"
#include "sdk.h"
#include "sdldef.h"
#include "gpuplugin.h"
static UINT8   * rams;
GPU_State gpustats;
static void (*Flush)();
//


//HWND      hWnd;
unsigned short * Vbuffer;
unsigned int Vbuffer_w;
unsigned int Vbuffer_h;
unsigned int Vbuffer_incx;
unsigned int Vbuffer_incy;
//extern ushort FrameBuffer[];

void GLESGPU_Update()
{   
    GPU_updateLace();
}

UINT32 GLESGP1_Read()
{  
    return GPU_readStatus();
}

UINT32 GLESGP0_Read()
{  
    return GPU_readData();
}

void GLESGP0_Write(UINT32 data)
{   
    GPU_writeData(data);
}

void GLESGP1_Write(UINT32 code)
{    
    GPU_writeStatus(code);
}

int GLESGPU_Open(UINT32 *par)
{
     FPSE_SDL *g = (FPSE_SDL *)par;
	rams=g->SystemRam;
    g->Flags = (g->Flags & ~GPU_USE_DIB_UPDATE ) | GPU_CORRECT_MDEC_COLORS;

    if (GPU_init()) {
        return FPSE_ERR;
    }

    if (GPU_open(0)) {
        return FPSE_ERR;
    }
    return FPSE_OK;
}

void GLESGPU_Close()
{
    GPU_close();
    GPU_shutdown();
}

void GLESGPU_ScreenShot(char *path)
{
    GPU_makeSnapshot();
}        

int GLESGPU_DmaChain(UINT32 adr)
{


    GPU_dmaChain((unsigned long *)rams,adr&0x1fffff);


    return 0;
}

void GLESGPU_DmaIn(UINT32 adr,int size)
{
    UINT32 *ram32 = (UINT32 *)(rams+(adr & 0x1FFFFF));
    void *ptFct = (void *)GPU_readDataMem;


    if (ptFct != NULL)
        GPU_readDataMem(ram32,size);
    else {
        int x;

        for (x=0;x<size;x++)
            ram32[x] = GPU_readData();
    }

}

void GLESGPU_DmaOut(UINT32 adr,int size)
{
    UINT32 *ram32 = (UINT32 *)(rams+(adr & 0x1FFFFF));
    void *ptFct = (void *)GPU_writeDataMem;


    if (ptFct != NULL)
        GPU_writeDataMem(ram32,size);
    else {
        int x;

        for (x=0;x<size;x++)
            GLESGP0_Write(ram32[x]);
    }

}

void GLESGPU_DmaExec(UINT32 adr,UINT32 bcr,UINT32 chcr)
{
    int x,size;
    UINT32 *ram32 = (UINT32 *)(rams+(adr & 0x1FFFFF));
    void *ptFctR = (void *)GPU_readDataMem;
    void *ptFctW = (void *)GPU_writeDataMem;

    size = (bcr>>16)*(bcr&0xffff);

    switch(chcr){
    case 0x01000200:
	 Flush(adr,adr+size*4);
        if (ptFctR != NULL)
            GPU_readDataMem(ram32,size);
        else {
            for (x=0;x<size;x++)
                ram32[x] = GPU_readData();
        }
        break;
    case 0x01000201:
        if (ptFctW != NULL)
            GPU_writeDataMem(ram32,size);
        else {
            for (x=0;x<size;x++)
                GLESGP0_Write(ram32[x]);
        }
        break;
    case 0x01000401:
        GPU_dmaChain((UINT32*)rams,adr&0x1fffff);
        break;
    case 0x00000200:
    case 0x00000201:
    case 0x00000401: // disable dma
        break;
    default:
	  break; //syslog("gpudma unknown %08x\n",(int)chcr);
    }

}

void GLESGPU_LoadState(GPU_State *gpustat)
{
    GPUFreeze_t *pF;

    if (gpustat == NULL)
        return;

    pF = (GPUFreeze_t *)malloc(sizeof(GPUFreeze_t));
    if (pF == NULL) return;

    pF->ulFreezeVersion = 1;
    pF->ulStatus = gpustat->status;
    memcpy(pF->ulControl,gpustat->control,256*sizeof(unsigned long));
    memcpy(pF->psxVRam,gpustat->vramp,1024*1024);

    GPU_freeze(0, pF);
    free(pF);
	}

void  *GLESGPU_SaveState(fpse_save_helper_t *helper)
{
    GPUFreeze_t *pF;
    GPU_State *gpustat;
    fpse_save_malloc_t host_malloc;
    void *ptFct = (void *)GPU_freeze;

    if (ptFct == NULL || helper == NULL)
        return NULL;

    pF = (GPUFreeze_t *)malloc(sizeof(GPUFreeze_t));
    if (pF == NULL) return NULL;

    pF->ulFreezeVersion = 1;
    if (GPU_freeze(1, pF) == 0) {
        free(pF);
        return NULL;
    }
    host_malloc = helper->save_malloc;
    gpustat = (GPU_State *)host_malloc(sizeof(GPU_State)+1024*1024);
    if (gpustat == NULL) {
        free(pF);
        return NULL;
    }
    gpustat->vramp = ((UINT8 *)gpustat) + sizeof(GPU_State);
    gpustat->version = 0;
    gpustat->status = pF->ulStatus;
    memcpy(gpustat->control,pF->ulControl,256*sizeof(unsigned long));
    memcpy(gpustat->vramp,pF->psxVRam,1024*1024);
    free(pF);

    /*if (gpu.GPUgetScreenPic != NULL) {
        gpustat->screenpic = (UINT8 *)host_malloc(128*96*3);
        if (gpustat->screenpic != NULL) {
            gpu.GPUgetScreenPic(gpustat->screenpic);
        }
    }*/

    return gpustat;
}


void GLESGPU_InfoDisplay(GPU_InfoDisplayType *nfo)
{
}

void GLESGPU_DrawBmp(GPU_BmpType *bm, int x, int y)
{
}

const GPUEngine_Type GLESGPU_Engine = {
    GLESGPU_Open,
    GLESGPU_Close,
    GLESGP0_Read,
    GLESGP1_Read,
    GLESGP0_Write,
    GLESGP1_Write,
    GLESGPU_Update,
    GLESGPU_DmaExec,
    GLESGPU_DmaChain,
    GLESGPU_DmaOut,
    GLESGPU_DmaIn,
    NULL, /* no integral screenshot function! */
    NULL,
    NULL,
    GLESGPU_LoadState,
    GLESGPU_SaveState,
    GLESGPU_InfoDisplay,
    GLESGPU_DrawBmp,
    NULL,
};
