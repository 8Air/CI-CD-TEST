#define SEND_VIDEO_TASK_FREQ            300
#define DATA_SEND_FROM_VIDEO_STREAM_MAX_LEN 65000

#include "cap2.c"
#include <time.h>
#include "dji_high_speed_data_channel.h"


static T_DjiTaskHandle s_userSendVideoThread;

#define TS_IDLE 0
#define TS_START 1
#define TS_READY 2

static unsigned char ts_Read = TS_IDLE;
static unsigned char ts_Convert = TS_IDLE;
static unsigned char ts_Compress = TS_IDLE;

static T_DjiMutexHandle s_CompMutex = {0};

static T_DjiTaskHandle ts_ReadThread;
static T_DjiTaskHandle ts_ConvertThread;
static T_DjiTaskHandle ts_CompressThread;

static T_DjiCameraMediaDownloadPlaybackHandler s_psdkCameraMedia = {0};


static unsigned int timeRead=0;
static unsigned int timeConvert=0;
static unsigned int timeCompress=0;

char AUD[]={0x00, 0x00, 0x00, 0x01, 0x09, 0x10};

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
        Osal_TaskSleepMs(1);
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
        Osal_TaskSleepMs(1);
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
        Osal_TaskSleepMs(1);
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
    unsigned int stime,i,status,fpsc,fps,timec, counter=0;
    uint32_t dataLength,lengthOfDataHaveBeenSent,lengthOfDataToBeSent;
    T_DjiDataChannelState videoStreamState = {0};
    char *dataBuffer = NULL;
    T_DjiReturnCode DjiStat;

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
        Osal_TaskSleepMs(1);
        counter++;

        do  
        {
            Osal_MutexLock(s_CompMutex);
            status=ts_Read;
            Osal_MutexUnlock(s_CompMutex);
            if (status==TS_START) Osal_TaskSleepMs(2);
        }
	while (status==TS_START);

        do
        {
            Osal_MutexLock(s_CompMutex);
            status=ts_Convert;
            Osal_MutexUnlock(s_CompMutex);
            if (status==TS_START) Osal_TaskSleepMs(2);
        }
	while (status==TS_START);

        do
        {
            Osal_MutexLock(s_CompMutex);
            status=ts_Compress;
            Osal_MutexUnlock(s_CompMutex);
            if (status==TS_START) Osal_TaskSleepMs(2);
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
        if ((counter%100)==0)
	        DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,"Frame info: read %d ms, convert %d ms, compress %d ms, FPS: %d, sending size: %d", timeRead, timeConvert, timeCompress, fps, dataLength);
        
        Osal_MutexLock(s_CompMutex);
        ts_Read=TS_START;
        ts_Convert=TS_START;
        ts_Compress=TS_START;
        Osal_MutexUnlock(s_CompMutex);

        lengthOfDataHaveBeenSent = 0;
        while (dataLength - lengthOfDataHaveBeenSent) {
            lengthOfDataToBeSent = USER_UTIL_MIN(DATA_SEND_FROM_VIDEO_STREAM_MAX_LEN, dataLength - lengthOfDataHaveBeenSent);
            DjiStat = DjiPayloadCamera_SendVideoStream((const uint8_t *) h264_buffer_ready + lengthOfDataHaveBeenSent,lengthOfDataToBeSent);
            if (DjiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
	            DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_ERROR, "send video stream error: 0x%08llX.", DjiStat);
            }
            lengthOfDataHaveBeenSent += lengthOfDataToBeSent;
        }

//        PsdkPayloadCamera_SendVideoStream(AUD,6);

        DjiStat = DjiPayloadCamera_GetVideoStreamState(&videoStreamState);
        if (DjiStat == DJI_RETURN_CODE_OK) {
	        if ((counter%100)==0)
	            DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,
	                "video stream state: RTBLimit: %d, RTBBeforeFlowController: %d, busyState: %d,",
	                videoStreamState.realtimeBandwidthLimit, videoStreamState.realtimeBandwidthBeforeFlowController,
	                videoStreamState.busyState);
        } else {
	        DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_ERROR, "get video stream state error.");
        }

        while (videoStreamState.busyState==1)
		{
		Osal_TaskSleepMs(100);
                DjiStat = DjiPayloadCamera_GetVideoStreamState(&videoStreamState);
				DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,
			                        "Delay 100ms due overload, Video stream state: RTBLimit: %d, RTBBeforeFlowController: %d, busyState: %d,",
                	videoStreamState.realtimeBandwidthLimit, videoStreamState.realtimeBandwidthBeforeFlowController,
                	videoStreamState.busyState);
		}
	
    }
}


T_DjiReturnCode  DjiTest_CameraMediaInit(void)
{

    char ipAddr[DJI_IP_ADDR_STR_SIZE_MAX] = {0};
    uint16_t port = 0;
    T_DjiReturnCode djiStat,returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    E_DjiChannelAddress channelAddress;
    const T_DjiDataChannelBandwidthProportionOfHighspeedChannel bandwidthProportionOfHighspeedChannel =
        {10, 80, 10};


    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> DATA CHANNEL INIT");

    returnCode = DjiHighSpeedDataChannel_SetBandwidthProportion(bandwidthProportionOfHighspeedChannel);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Set data channel bandwidth width proportion error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }


    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> CAMERA MEDIA INIT");



    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,"SetVideoStreamType");
   if (DjiPayloadCamera_SetVideoStreamType(DJI_CAMERA_VIDEO_STREAM_TYPE_H264_CUSTOM_FORMAT)
                                                            != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_ERROR,"Dji camera set video stream error.");
        return DJI_RETURN_CODE_ERR_UNKNOWN;
    }

    if (DjiPayloadCamera_GetVideoStreamRemoteAddress(ipAddr, &port) == DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,"Get video stream remote address: %s, port: %d", ipAddr, port);
    } else {
        DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_ERROR,"get video stream remote address error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }


    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,"TaskCreate user_camera_media_task");
    if (Osal_TaskCreate("user_camera_media_task",UserCameraMedia_SendVideoTask,2048, NULL, &s_userSendVideoThread) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) 
    {
        DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_ERROR,"user send video task create error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,"MutexCreate");
    Osal_MutexCreate(&s_CompMutex);
    Osal_TaskCreate("user_camera_media_read_task", UserCameraMedia_ReadTask, 2048, NULL,&ts_ReadThread);
    Osal_TaskCreate("user_camera_media_convert_task",UserCameraMedia_ConvertTask,2048,NULL, &ts_ConvertThread);
    Osal_TaskCreate("user_camera_media_compress_task",UserCameraMedia_CompressTask ,2048, NULL, &ts_CompressThread);


    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> CAMERA MEDIA INIT END");
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;

}