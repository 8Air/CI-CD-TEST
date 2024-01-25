#define SEND_VIDEO_TASK_FREQ            300
#define DATA_SEND_FROM_VIDEO_STREAM_MAX_LEN 65000

#include "cap2.c"
#include <time.h>

static T_PsdkTaskHandle s_userSendVideoThread;

#define TS_IDLE 0
#define TS_START 1
#define TS_READY 2

static unsigned char ts_Read = TS_IDLE;
static unsigned char ts_Convert = TS_IDLE;
static unsigned char ts_Compress = TS_IDLE;

static T_PsdkMutexHandle s_CompMutex = {0};

static T_PsdkTaskHandle ts_ReadThread;
static T_PsdkTaskHandle ts_ConvertThread;
static T_PsdkTaskHandle ts_CompressThread;

static unsigned int timeRead=0;
static unsigned int timeConvert=0;
static unsigned int timeCompress=0;

long mtime()
{
  struct timeval t;

  gettimeofday(&t, NULL);
  long mt = (long)t.tv_sec * 1000 + t.tv_usec / 1000;
  return mt;
}

static void *UserCameraMedia_ReadTask(void *arg)
{
unsigned char stat;
long stime;

    while (1) {
        PsdkOsal_TaskSleepMs(1);
        Osal_MutexLock(s_CompMutex);
        stat=ts_Read;
        Osal_MutexUnlock(s_CompMutex);
        if (stat==TS_START) {
            stime = mtime();
            do_jpeg();
            timeRead=(mtime()-stime);
            Osal_MutexLock(s_CompMutex);
            ts_Read=TS_READY;
            Osal_MutexUnlock(s_CompMutex);
        }
    }
}

static void *UserCameraMedia_ConvertTask(void *arg)
{
    unsigned char stat;
    long stime;

    while (1) {
        PsdkOsal_TaskSleepMs(1);
        Osal_MutexLock(s_CompMutex);
        stat=ts_Convert;
        Osal_MutexUnlock(s_CompMutex);
        if (stat==TS_START) {
            stime = mtime();
            do_convert_my();
            timeConvert=(mtime()-stime);
            Osal_MutexLock(s_CompMutex);
            ts_Convert=TS_READY;
            Osal_MutexUnlock(s_CompMutex);
        }
    }
}

static void *UserCameraMedia_CompressTask(void *arg)
{
    unsigned char stat;
    long stime;

    while (1) {
        PsdkOsal_TaskSleepMs(1);
        Osal_MutexLock(s_CompMutex);
        stat=ts_Compress;
        Osal_MutexUnlock(s_CompMutex);
        if (stat==TS_START) {
            stime = mtime();
            do_compress();
            timeCompress=(mtime()-stime);
            Osal_MutexLock(s_CompMutex);
            ts_Compress=TS_READY;
            Osal_MutexUnlock(s_CompMutex);
        }
    }
}



static void *UserCameraMedia_SendVideoTask(void *arg)
{
    unsigned int stime,i,status,fpsc,fps,timec;
    uint32_t dataLength,lengthOfDataHaveBeenSent,lengthOfDataToBeSent;
    T_PsdkDataChannelState videoStreamState = {0};
    char *dataBuffer = NULL;
    T_PsdkReturnCode psdkStat;

    do_jpeg();
    bmp_buffer_ready=(unsigned char*) malloc(bmp_size);
    memcpy(bmp_buffer_ready,bmp_buffer,bmp_size);
    free(bmp_buffer);

    do_convert_my();

    for (i=0;i<3; i++)
    {
        i420_ready[i] = (uint8_t*)malloc((size_t)i420Stride[i] * height);
        memcpy(i420_ready[i],i420[i],(size_t)i420Stride[i] * height);
        free(i420[i]);
    }

    init_compress();

    do_compress();

    h264_buffer_ready=(unsigned char*) malloc(h264_size);
    memcpy(h264_buffer_ready, h264_buffer, h264_size);

    Osal_MutexLock(s_CompMutex);
    ts_Read=TS_START;
    ts_Convert=TS_START;
    ts_Compress=TS_START;
    Osal_MutexUnlock(s_CompMutex);
    timec=time(NULL);

    while (1) {
        PsdkOsal_TaskSleepMs(1);

        do  
        {
            Osal_MutexLock(s_CompMutex);
            status=ts_Read;
            Osal_MutexUnlock(s_CompMutex);
            if (status==TS_START) PsdkOsal_TaskSleepMs(2);
        }
	while (status==TS_START);

        do
        {
            Osal_MutexLock(s_CompMutex);
            status=ts_Convert;
            Osal_MutexUnlock(s_CompMutex);
            if (status==TS_START) PsdkOsal_TaskSleepMs(2);
        }
	while (status==TS_START);

        do
        {
            Osal_MutexLock(s_CompMutex);
            status=ts_Compress;
            Osal_MutexUnlock(s_CompMutex);
            if (status==TS_START) PsdkOsal_TaskSleepMs(2);
        }
	while (status==TS_START);

        free(bmp_buffer_ready);
        bmp_buffer_ready=(unsigned char*) malloc(bmp_size);
        memcpy(bmp_buffer_ready,bmp_buffer,bmp_size);
        free(bmp_buffer);
 
        for (i=0;i<3; i++)
        {
            free(i420_ready[i]);
        }
 
        for (i=0;i<3; i++)
        {
            i420_ready[i] = (uint8_t*)malloc((size_t)i420Stride[i] * height);
            memcpy(i420_ready[i],i420[i],(size_t)i420Stride[i] * height);
            free(i420[i]);
        }

        free(h264_buffer_ready);
        h264_buffer_ready=(unsigned char*) malloc(h264_size);
        memcpy(h264_buffer_ready, h264_buffer, h264_size);
        dataLength=h264_size;

        fpsc++;
        if (time(NULL)>timec)
        {
            timec=time(NULL);
            fps=fpsc;
            fpsc=0;
        }
        PsdkLogger_UserLogInfo("Frame info: read %d ms, convert %d ms, compress %d ms, FPS: %d, sending size: %d", timeRead, timeConvert, timeCompress, fps, dataLength);
        
        Osal_MutexLock(s_CompMutex);
        ts_Read=TS_START;
        ts_Convert=TS_START;
        ts_Compress=TS_START;
        Osal_MutexUnlock(s_CompMutex);


        lengthOfDataHaveBeenSent = 0;
        while (dataLength - lengthOfDataHaveBeenSent) {
            lengthOfDataToBeSent = USER_UTIL_MIN(DATA_SEND_FROM_VIDEO_STREAM_MAX_LEN, dataLength - lengthOfDataHaveBeenSent);
            psdkStat = PsdkPayloadCamera_SendVideoStream((const uint8_t *) h264_buffer_ready + lengthOfDataHaveBeenSent,
                                                         lengthOfDataToBeSent);
            if (psdkStat != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                PsdkLogger_UserLogError("send video stream error: 0x%08llX.", psdkStat);
            }
            lengthOfDataHaveBeenSent += lengthOfDataToBeSent;
        }

	
        psdkStat = PsdkPayloadCamera_GetVideoStreamState(&videoStreamState);
        if (psdkStat == PSDK_RETURN_CODE_OK) {
//            PsdkLogger_UserLogInfo(
//                "video stream state: RTBLimit: %d, RTBBeforeFlowController: %d, busyState: %d,",
//                videoStreamState.realtimeBandwidthLimit, videoStreamState.realtimeBandwidthBeforeFlowController,
//                videoStreamState.busyState);
        } else {
            PsdkLogger_UserLogError("get video stream state error.");
        }


        while (videoStreamState.busyState==1)
		{
		PsdkOsal_TaskSleepMs(100);
                psdkStat = PsdkPayloadCamera_GetVideoStreamState(&videoStreamState);
                PsdkLogger_UserLogInfo(
                	"Delay 100ms due overload, Video stream state: RTBLimit: %d, RTBBeforeFlowController: %d, busyState: %d,",
                	videoStreamState.realtimeBandwidthLimit, videoStreamState.realtimeBandwidthBeforeFlowController,
                	videoStreamState.busyState);
		}


    }
}


T_PsdkReturnCode  PsdkTest_CameraMediaInit(void)
{

    PsdkLogger_UserLogInfo(">>> CAMERA MEDIA INIT");

   if (PsdkPayloadCamera_SetVideoStreamType(PSDK_CAMERA_VIDEO_STREAM_TYPE_H264_CUSTOM_FORMAT) != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        PsdkLogger_UserLogError("psdk camera set video stream error.");
        return PSDK_RETURN_CODE_ERR_UNKNOWN;
    }


    if (PsdkOsal_TaskCreate(&s_userSendVideoThread, UserCameraMedia_SendVideoTask, "user_camera_media_task", 2048,
                            NULL) != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        PsdkLogger_UserLogError("user send video task create error.");
        return PSDK_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    PsdkOsal_MutexCreate(&s_CompMutex);
    PsdkOsal_TaskCreate(&ts_ReadThread, UserCameraMedia_ReadTask, "user_camera_media_read_task", 2048, NULL);
    PsdkOsal_TaskCreate(&ts_ConvertThread, UserCameraMedia_ConvertTask, "user_camera_media_convert_task", 2048, NULL);
    PsdkOsal_TaskCreate(&ts_CompressThread, UserCameraMedia_CompressTask, "user_camera_media_compress_task", 2048, NULL);


    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;

}