#ifndef _FPSE_SDK_H_
#define _FPSE_SDK_H_

typedef void *(*fpse_save_malloc_t)(unsigned int);
typedef void  (*fpse_save_free_t)(void *);

typedef struct {
    fpse_save_malloc_t save_malloc;
    fpse_save_free_t   save_free;
} fpse_save_helper_t;

typedef struct {
    UINT32 version;
    UINT8 *data;
    UINT32 size;
} FPSE_PART_TYPE;

typedef void (*FPSE_CallBack_Type)(void);

//------------------------------------------------------------
// CDROM section
//------------------------------------------------------------

// Macros for Type:
#define CD_TYPE_UNKNOWN 0x00
#define CD_TYPE_DATA    0x01
#define CD_TYPE_AUDIO   0x02
#define CD_TYPE_NONE    0xff

// Macros for Status:
#define CD_STATUS_UNKNOWN   0x00
#define CD_STATUS_ERROR     0x01
#define CD_STATUS_SEEKERR   0x04
#define CD_STATUS_TRAYOPEN  0x10
#define CD_STATUS_READING   0x20
#define CD_STATUS_SEEKING   0x40
#define CD_STATUS_PLAYING   0x80

typedef struct {
    UINT8   CtrlADR;
    UINT8   Track;
    UINT8   Index;
    UINT8   Minute;
    UINT8   Second;
    UINT8   Frame;
    UINT8   Filler;
    UINT8   AMinute;
    UINT8   ASecond;
    UINT8   AFrame;
} FPSE_SUBQ_Type;

typedef struct {
    UINT32  Type;    // cdrom type
    UINT32  Status;  // drive status
    UINT8   Time[4]; // current playing time (M,S,F)
} FPSE_CDSTATUS_Type;

typedef UINT8 *(*pCD_Read)(UINT8 *param);
typedef int    (*pCD_Play)(UINT8 *param);
typedef int    (*pCD_Stop)(void);
typedef int    (*pCD_GetTD)(UINT8 *result,int track);
typedef int    (*pCD_GetTN)(UINT8 *result);
typedef int    (*pCD_Open)(UINT32 *par);
typedef void   (*pCD_Close)(void);
typedef int    (*pCD_Wait)(void);
typedef void   (*pCD_Eject)(void);

typedef FPSE_SUBQ_Type     *(*pCD_GetSeek)(void);
typedef FPSE_CDSTATUS_Type *(*pCD_GetStatus)(void);

typedef int    (*pCD_Configure)(UINT32 *par);
typedef void   (*pCD_About)(UINT32 *par);

typedef struct {
    pCD_Read        Read;
    pCD_Play        Play;
    pCD_Stop        Stop;
    pCD_GetTD       GetTD;
    pCD_GetTN       GetTN;
    pCD_Open        Open;
    pCD_Close       Close;
    pCD_Wait        Wait;
    pCD_GetSeek     GetSeek;
    pCD_GetStatus   GetStatus;
    pCD_Eject       Eject;
    pCD_Configure   Configure;
    pCD_About       About;
} CDEngine_Type;

//------------------------------------------------------------
// GPU section
//------------------------------------------------------------

#define GPU_INFODISPLAY_VERSION    0

#define GPU_INFODISPLAY_SHOWFPS    0x00010000
#define GPU_INFODISPLAY_PRINTF     0x00020000

typedef struct {
    UINT16 w;
    UINT16 h;
} GPU_BmpType;

typedef struct {
    UINT32 version;
    UINT32 status;
    UINT32 control[256];
    UINT8 *vramp;
    UINT8 *screenpic;
} GPU_State;

typedef struct {
    UINT32 version;
    UINT32 fps;
    char  *text;
} GPU_InfoDisplayType;

typedef int    (*pGPU_Open)(UINT32 *gpu);
typedef void   (*pGPU_Close)(void);
typedef UINT32 (*pGP0_Read)(void);
typedef UINT32 (*pGP1_Read)(void);
typedef void   (*pGP0_Write)(UINT32 data);
typedef void   (*pGP1_Write)(UINT32 code);
typedef void   (*pGPU_Update)(void);
typedef void   (*pGPU_DmaExec)(UINT32 adr,UINT32 bcr,UINT32 chcr);
typedef int    (*pGPU_DmaChain)(UINT32 addr);
typedef void   (*pGPU_DmaOut)(UINT32 addr,int size);
typedef void   (*pGPU_DmaIn)(UINT32 addr,int size);
typedef void   (*pGPU_ScreenShot)(char *path);
typedef int    (*pGPU_Configure)(UINT32 *par);
typedef void   (*pGPU_About)(UINT32 *par);
typedef void   (*pGPU_LoadState)(GPU_State *state);
typedef void  *(*pGPU_SaveState)(fpse_save_helper_t*);
typedef void   (*pGPU_InfoDisplay)(GPU_InfoDisplayType *nfo);
typedef void   (*pGPU_DrawBmp)(GPU_BmpType *bm, int x, int y);
typedef int    (*pGPU_DmaWait)(void);

typedef struct {
    pGPU_Open        Open;
    pGPU_Close       Close;
    pGP0_Read        Read0;
    pGP1_Read        Read1;
    pGP0_Write       Write0;
    pGP1_Write       Write1;
    pGPU_Update      Update;
    pGPU_DmaExec     DmaExec;
    pGPU_DmaChain    DmaChain;
    pGPU_DmaOut      DmaOut;
    pGPU_DmaIn       DmaIn;
    pGPU_ScreenShot  ScreenShot;
    pGPU_Configure   Configure;
    pGPU_About       About;
    pGPU_LoadState   LoadState;
    pGPU_SaveState   SaveState;
    pGPU_InfoDisplay InfoDisplay;
    pGPU_DrawBmp     DrawBmp;
    pGPU_DmaWait     DmaWait;
} GPUEngine_Type;

// SPU section
//------------------------------------------------------------

typedef struct {
    UINT32 version;
    UINT16 portio[0x100];
    UINT8 *sram;
    UINT32 size;
    UINT8 *control;
} SPU_State;

typedef int   (*pSPU_Open)(UINT32 *spu);
typedef void  (*pSPU_Close)(void);
typedef int   (*pSPU_Read)(UINT32 adr);
typedef void  (*pSPU_Write)(UINT32 adr, unsigned int data);
typedef void  (*pSPU_DmaExec)(UINT32 adr,UINT32 bcr,UINT32 chcr);
typedef void  (*pSPU_DmaOut)(UINT32 addr,int size);
typedef void  (*pSPU_DmaIn)(UINT32 addr,int size);
typedef int   (*pSPU_Configure)(UINT32 *par);
typedef void  (*pSPU_About)(UINT32 *par);
typedef void  (*pSPU_PlayStream)(INT16 *Buf, int freq, int chns);
typedef void  (*pSPU_LoadState)(SPU_State *state);
typedef void *(*pSPU_SaveState)(fpse_save_helper_t *);
typedef FPSE_CallBack_Type (*pSPU_GetCallBack)(void); // Deprecated!
typedef void  (*pSPU_SetVolume)(int volume);
typedef int   (*pSPU_Sync)(int nClocks);

typedef struct {
    pSPU_Open        Open;
    pSPU_Close       Close;
    pSPU_Read        Read;
    pSPU_Write       Write;
    pSPU_DmaExec     DmaExec;
    pSPU_DmaIn       DmaIn;
    pSPU_DmaOut      DmaOut;
    pSPU_Configure   Configure;
    pSPU_About       About;
    pSPU_PlayStream  PlayStream;
    pSPU_LoadState   LoadState;
    pSPU_SaveState   SaveState;
    pSPU_GetCallBack GetCallBack;
    pSPU_SetVolume   SetVolume;
    pSPU_Sync        Sync;
} SPUEngine_Type;

// CONTROLLERS section
//------------------------------------------------------------

typedef struct {
    int dummy;
} JOY_State;

#define ACK_OK	1
#define ACK_ERR 0

typedef int  (*pJOY_Open)(UINT32 *joy);
typedef void (*pJOY_Close)(void);
typedef void (*pJOY_SetOutputBuffer)(UINT8 *buf);
typedef int  (*pJOY_StartPoll)(void);
typedef int  (*pJOY_Poll)(int outbyte);
typedef void (*pJOY_LoadState)(JOY_State *state);
typedef void (*pJOY_SaveState)(JOY_State *state);
typedef int  (*pJOY_Configure)(UINT32 *par);
typedef void (*pJOY_About)(UINT32 *par);
typedef int  (*pJOY_SetAnalog)(int mode);

typedef struct {
    pJOY_Open            Open;
    pJOY_Close           Close;
    pJOY_SetOutputBuffer SetOutputBuffer;
    pJOY_StartPoll       StartPoll;
    pJOY_Poll            Poll;
    pJOY_SetAnalog       SetAnalog;
    pJOY_LoadState       LoadState;
    pJOY_SaveState       SaveState;
    pJOY_Configure       Configure;
    pJOY_About           About;
} JOYEngine_Type;

// PARALLEL port section
//------------------------------------------------------------

typedef struct {
    int dummy;
} PAR_State;

typedef struct {
    UINT8 *FlashAdr;
    UINT32 FlashSize;
} PAR_FlashDescriptor;

typedef int  (*pPAR_Open)(UINT32 *par);
typedef void (*pPAR_Close)(void);
typedef int  (*pPAR_Read)(UINT32 adr);
typedef void (*pPAR_Write)(UINT32 adr, unsigned int data);
typedef void (*pPAR_DmaExec)(UINT32 adr,UINT32 bcr,UINT32 chcr);
typedef int  (*pPAR_Configure)(UINT32 *par);
typedef void (*pPAR_About)(UINT32 *par);
typedef void (*pPAR_LoadState)(PAR_State *state);
typedef void (*pPAR_SaveState)(PAR_State *state);
typedef FPSE_CallBack_Type (*pPAR_GetCallBack)(void);
typedef void (*pPAR_GetFlash)(PAR_FlashDescriptor *des);

typedef struct {
    pPAR_Open        Open;
    pPAR_Close       Close;
    pPAR_Read        Read;
    pPAR_Write       Write;
    pPAR_DmaExec     DmaExec;
    pPAR_Configure   Configure;
    pPAR_About       About;
    pPAR_LoadState   LoadState;
    pPAR_SaveState   SaveState;
    pPAR_GetCallBack GetCallBack;
    pPAR_GetFlash    GetFlash;
} PAREngine_Type;

#if defined(FPSE_COMP_CORE) && defined(FPSE_MULTI_CORE)

// CDROM
#define CD_Read             CDEngine.Read
#define CD_Play             CDEngine.Play
#define CD_Stop             CDEngine.Stop
#define CD_GetTD            CDEngine.GetTD
#define CD_GetTN            CDEngine.GetTN
#define CD_Open             CDEngine.Open
#define CD_Close            CDEngine.Close
#define CD_Wait             CDEngine.Wait
#define CD_GetSeek          CDEngine.GetSeek
#define CD_GetStatus        CDEngine.GetStatus
#define CD_Eject            CDEngine.Eject
#define CD_Configure        CDEngine.Configure
#define CD_About            CDEngine.About

// GPU
#define GPU_Open            GPUEngine.Open
#define GPU_Close           GPUEngine.Close
#define GP0_Read            GPUEngine.Read0
#define GP1_Read            GPUEngine.Read1
#define GP0_Write           GPUEngine.Write0
#define GP1_Write           GPUEngine.Write1
#define GPU_Update          GPUEngine.Update
#define GPU_DmaExec         GPUEngine.DmaExec
#define GPU_DmaChain        GPUEngine.DmaChain
#define GPU_DmaOut          GPUEngine.DmaOut
#define GPU_DmaIn           GPUEngine.DmaIn
#define GPU_ScreenShot      GPUEngine.ScreenShot
#define GPU_Configure       GPUEngine.Configure
#define GPU_About           GPUEngine.About
#define GPU_LoadState       GPUEngine.LoadState
#define GPU_SaveState       GPUEngine.SaveState
#define GPU_InfoDisplay     GPUEngine.InfoDisplay
#define GPU_DrawBmp         GPUEngine.DrawBmp
#define GPU_DmaWait         GPUEngine.DmaWait

// SPU
#define SPU_Open            SPUEngine.Open
#define SPU_Close           SPUEngine.Close
#define SPU_Read            SPUEngine.Read
#define SPU_Write           SPUEngine.Write
#define SPU_DmaExec         SPUEngine.DmaExec
#define SPU_Configure       SPUEngine.Configure
#define SPU_About           SPUEngine.About
#define SPU_LoadState       SPUEngine.LoadState
#define SPU_SaveState       SPUEngine.SaveState
#define SPU_PlayStream      SPUEngine.PlayStream
#define SPU_GetCallBack     SPUEngine.GetCallBack
#define SPU_DmaOut          SPUEngine.DmaOut
#define SPU_DmaIn           SPUEngine.DmaIn
#define SPU_SetVolume       SPUEngine.SetVolume
#define SPU_Sync            SPUEngine.Sync

// JOY
#define JOY0_Open               JOY0Engine.Open
#define JOY0_Close              JOY0Engine.Close
#define JOY0_Configure          JOY0Engine.Configure
#define JOY0_About              JOY0Engine.About
#define JOY0_SetAnalog          JOY0Engine.SetAnalog
#define JOY0_LoadState          JOY0Engine.LoadState
#define JOY0_SaveState          JOY0Engine.SaveState
#define JOY0_SetOutputBuffer    JOY0Engine.SetOutputBuffer
#define JOY0_StartPoll          JOY0Engine.StartPoll
#define JOY0_Poll               JOY0Engine.Poll

#define JOY1_Open               JOY1Engine.Open
#define JOY1_Close              JOY1Engine.Close
#define JOY1_Configure          JOY1Engine.Configure
#define JOY1_About              JOY1Engine.About
#define JOY1_SetAnalog          JOY1Engine.SetAnalog
#define JOY1_LoadState          JOY1Engine.LoadState
#define JOY1_SaveState          JOY1Engine.SaveState
#define JOY1_SetOutputBuffer    JOY1Engine.SetOutputBuffer
#define JOY1_StartPoll          JOY1Engine.StartPoll
#define JOY1_Poll               JOY1Engine.Poll

// PAR
#define PAR_Open          PAREngine.Open
#define PAR_Close         PAREngine.Close
#define PAR_Read          PAREngine.Read
#define PAR_Write         PAREngine.Write
#define PAR_DmaExec       PAREngine.DmaExec
#define PAR_Configure     PAREngine.Configure
#define PAR_About         PAREngine.About
#define PAR_LoadState     PAREngine.LoadState
#define PAR_SaveState     PAREngine.SaveState
#define PAR_GetCallBack   PAREngine.GetCallBack
#define PAR_GetFlash      PAREngine.GetFlash

#else

// CDROM
UINT8 *CD_Read(UINT8 *param);
int    CD_Play(UINT8 *param);
int    CD_Stop(void);
int    CD_GetTD(UINT8 *result,int track);
int    CD_GetTN(UINT8 *result);
int    CD_Open(UINT32 *par);
void   CD_Close(void);
int    CD_Wait(void);
int    CD_Configure(UINT32 *par);
void   CD_About(UINT32 *par);
FPSE_SUBQ_Type     *CD_GetSeek(void);
FPSE_CDSTATUS_Type *CD_GetStatus(void);
void                CD_Eject(void);

// GPU
int    GPU_Open(UINT32 *gpu);
void   GPU_Close(void);
UINT32 GP0_Read(void);
UINT32 GP1_Read(void);
void   GP0_Write(UINT32 data);
void   GP1_Write(UINT32 code);
void   GPU_Update(void);
void   GPU_DmaExec(UINT32 adr,UINT32 bcr,UINT32 chcr);
int    GPU_DmaChain(UINT32 addr);
void   GPU_DmaOut(UINT32 addr,int size);
void   GPU_DmaIn(UINT32 addr,int size);
void   GPU_ScreenShot(char *path);
int    GPU_Configure(UINT32 *par);
void   GPU_About(UINT32 *par);
void   GPU_LoadState(GPU_State *state);
void  *GPU_SaveState(fpse_save_helper_t *);
void   GPU_InfoDisplay(GPU_InfoDisplayType *nfo);
void   GPU_DrawBmp(GPU_BmpType *bm, int x, int y);
int    GPU_DmaWait(void);

// SPU
int   SPU_Open(UINT32 *spu);
void  SPU_Close(void);
int   SPU_Read(UINT32 adr);
void  SPU_Write(UINT32 adr, unsigned int data);
void  SPU_DmaExec(UINT32 adr,UINT32 bcr,UINT32 chcr);
void  SPU_DmaOut(UINT32 addr,int size);
void  SPU_DmaIn(UINT32 addr,int size);
int   SPU_Configure(UINT32 *par);
void  SPU_About(UINT32 *par);
void  SPU_PlayStream(INT16 *XAsampleBuf, int freq, int chns);
void  SPU_LoadState(SPU_State *state);
void *SPU_SaveState(fpse_save_helper_t *);
FPSE_CallBack_Type SPU_GetCallBack(void);
void  SPU_SetVolume(int volume);
int   SPU_Sync(int nClocks);

// JOY
int  JOY0_Open(UINT32 *joy);
void JOY0_Close(void);
void JOY0_SetOutputBuffer(UINT8 *buf);
int  JOY0_StartPoll(void);
int  JOY0_Poll(int outbyte);
int  JOY0_SetAnalog(int mode);
void JOY0_LoadState(JOY_State *state);
void JOY0_SaveState(JOY_State *state);
int  JOY0_Configure(UINT32 *par);
void JOY0_About(UINT32 *par);

int  JOY1_Open(UINT32 *joy);
void JOY1_Close(void);
void JOY1_SetOutputBuffer(UINT8 *buf);
int  JOY1_StartPoll(void);
int  JOY1_Poll(int outbyte);
int  JOY1_SetAnalog(int mode);
void JOY1_LoadState(JOY_State *state);
void JOY1_SaveState(JOY_State *state);
int  JOY1_Configure(UINT32 *par);
void JOY1_About(UINT32 *par);

// PAR
int  PAR_Open(UINT32 *par);
void PAR_Close(void);
int  PAR_Read(UINT32 adr);
void PAR_Write(UINT32 adr, unsigned int data);
void PAR_GetFlash(PAR_FlashDescriptor *des);
void PAR_DmaExec(UINT32 adr,UINT32 bcr,UINT32 chcr);
int  PAR_Configure(UINT32 *par);
void PAR_About(UINT32 *par);
void PAR_LoadState(PAR_State *state);
void PAR_SaveState(PAR_State *state);
FPSE_CallBack_Type PAR_GetCallBack(void);

#endif

#endif
