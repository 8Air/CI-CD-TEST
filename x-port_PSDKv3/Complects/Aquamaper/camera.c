#include <common.h>
#include "constants.h"

#define TAKEFILE "/xport/camdata/take"
#define FOCUSFILE "/xport/camdata/focus"

#define STARTVIDEOFILE "/xport/camdata/start_video"
#define STOPVIDEOFILE  "/xport/camdata/stop_video"
#define TIMERFILE  "/xport/camdata/timer_video"

/* Private variables ---------------------------------------------------------*/
static bool s_isCamInited = false;

static int x_PT=0;
static int x_shootingSeries=0;
static float x_userIntervalTimeSeconds = 0.f;

static bool x_InitPT = false;

static T_PsdkCameraCommonHandler s_commonHandler;
static T_PsdkCameraFocusHandler s_focusHandler;

static T_PsdkTaskHandle s_userCameraThread;
static T_PsdkTaskHandle s_userIntervalShootingThread;

static T_PsdkCameraSystemState s_cameraState;
static E_PsdkCameraShootPhotoMode s_cameraShootPhotoMode = PSDK_CAMERA_SHOOT_PHOTO_MODE_SINGLE;
static E_PsdkCameraBurstCount s_cameraBurstCount = PSDK_CAMERA_BURST_COUNT_2;
static T_PsdkCameraPhotoTimeIntervalSettings s_cameraPhotoTimeIntervalSettings = {0, 1};
static T_PsdkCameraSDCardState s_cameraSDCardState = {0};
static T_PsdkMutexHandle s_commonMutex = {0};

static E_PsdkCameraMeteringMode s_cameraMeteringMode = PSDK_CAMERA_METERING_MODE_CENTER;
static T_PsdkCameraSpotMeteringTarget s_cameraSpotMeteringTarget = {0};

static E_PsdkCameraFocusMode s_cameraFocusMode = PSDK_CAMERA_FOCUS_MODE_AUTO;
static T_PsdkCameraPointInScreen s_cameraFocusTarget = {0};
static uint32_t s_cameraFocusRingValue = 1;
static T_PsdkCameraFocusAssistantSettings s_cameraFocusAssistantSettings = {0};

/* Private functions declaration ---------------------------------------------*/
static T_PsdkReturnCode GetSystemState(T_PsdkCameraSystemState *systemState);
static T_PsdkReturnCode SetMode(E_PsdkCameraMode mode);
static T_PsdkReturnCode StartRecordVideo(void);
static T_PsdkReturnCode StopRecordVideo(void);
static T_PsdkReturnCode StartShootPhoto(void);
static T_PsdkReturnCode StopShootPhoto(void);
static T_PsdkReturnCode SetShootPhotoMode(E_PsdkCameraShootPhotoMode mode);
static T_PsdkReturnCode GetShootPhotoMode(E_PsdkCameraShootPhotoMode *mode);
static T_PsdkReturnCode SetPhotoBurstCount(E_PsdkCameraBurstCount burstCount);
static T_PsdkReturnCode GetPhotoBurstCount(E_PsdkCameraBurstCount *burstCount);
static T_PsdkReturnCode SetPhotoTimeIntervalSettings(T_PsdkCameraPhotoTimeIntervalSettings settings);
static T_PsdkReturnCode GetPhotoTimeIntervalSettings(T_PsdkCameraPhotoTimeIntervalSettings *settings);
static T_PsdkReturnCode GetSDCardState(T_PsdkCameraSDCardState *sdCardState);
static T_PsdkReturnCode FormatSDCard(void);

static T_PsdkReturnCode SetFocusMode(E_PsdkCameraFocusMode mode);
static T_PsdkReturnCode GetFocusMode(E_PsdkCameraFocusMode *mode);
static T_PsdkReturnCode SetFocusTarget(T_PsdkCameraPointInScreen target);
static T_PsdkReturnCode GetFocusTarget(T_PsdkCameraPointInScreen *target);
static T_PsdkReturnCode SetFocusAssistantSettings(T_PsdkCameraFocusAssistantSettings settings);
static T_PsdkReturnCode GetFocusAssistantSettings(T_PsdkCameraFocusAssistantSettings *settings);
static T_PsdkReturnCode SetFocusRingValue(uint32_t value);
static T_PsdkReturnCode GetFocusRingValue(uint32_t *value);
static T_PsdkReturnCode GetFocusRingValueUpperBound(uint32_t *value);

static void *UserCamera_Task(void *arg);
static void *UserIntervalShoot_Task(void *arg);

//POLL
static T_PsdkReturnCode GetSystemState(T_PsdkCameraSystemState *systemState) //POLL!!!
{
    *systemState = s_cameraState;
    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_PsdkReturnCode GetShootPhotoMode(E_PsdkCameraShootPhotoMode *mode) //POLL!!!
{
    *mode = s_cameraShootPhotoMode;
    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_PsdkReturnCode GetPhotoBurstCount(E_PsdkCameraBurstCount *burstCount) //POLL!!!
{
    *burstCount = s_cameraBurstCount;
    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


static T_PsdkReturnCode GetPhotoTimeIntervalSettings(T_PsdkCameraPhotoTimeIntervalSettings *settings)  //POLL!!!!
{
    memcpy(settings, &s_cameraPhotoTimeIntervalSettings, sizeof(T_PsdkCameraPhotoTimeIntervalSettings));
    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_PsdkReturnCode GetSDCardState(T_PsdkCameraSDCardState *sdCardState) //POLL!!!!
{
    memcpy(sdCardState, &s_cameraSDCardState, sizeof(T_PsdkCameraSDCardState));
    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_PsdkReturnCode GetFocusMode(E_PsdkCameraFocusMode *mode) //POLL!!!
{
    *mode = s_cameraFocusMode;
    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_PsdkReturnCode GetFocusTarget(T_PsdkCameraPointInScreen *target) //POLL!!!
{
    memcpy(target, &s_cameraFocusTarget, sizeof(T_PsdkCameraPointInScreen));
    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_PsdkReturnCode GetFocusAssistantSettings(T_PsdkCameraFocusAssistantSettings *settings) //POLL!!!
{
    memcpy(settings, &s_cameraFocusAssistantSettings, sizeof(T_PsdkCameraFocusAssistantSettings));
    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


static T_PsdkReturnCode GetFocusRingValue(uint32_t *value) //POLL!!!
{
    *value = s_cameraFocusRingValue;
    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_PsdkReturnCode GetFocusRingValueUpperBound(uint32_t *value) //POLL!!!
{
    *value = 1;
    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


// SET

static T_PsdkReturnCode SetMode(E_PsdkCameraMode mode)
{
    s_cameraState.cameraMode = mode;
    PsdkLogger_UserLogInfo(">>> SetMode set camera mode:%d", mode);
    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_PsdkReturnCode FormatSDCard(void)
{
    PsdkLogger_UserLogInfo(">>> FormatSDCard Format SD Card");
    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

///from SDK documentation
///https://developer.dji.com/document/949be716-6424-4c9a-b5d5-aca7cb4f08d5  "Develop with the Record Video"
static T_PsdkReturnCode StartRecordVideo(void)
{
    s_cameraState.isRecording = true;
    s_cameraState.currentVideoRecordingTimeInSeconds = 0;
    PsdkLogger_UserLogInfo(">>> StartRecordVideo start record video");

    writefl(STARTVIDEOFILE,0);
    return PSDK_RETURN_CODE_OK;
}

static T_PsdkReturnCode StopRecordVideo(void)
{
    s_cameraState.isRecording = false;
    s_cameraState.currentVideoRecordingTimeInSeconds = 0;
    PsdkLogger_UserLogInfo(">>> StopRecordVideo stop record video");

    writefl(STOPVIDEOFILE,0);
    return PSDK_RETURN_CODE_OK;
}

static T_PsdkReturnCode StartShootPhoto(void)
{
    PsdkLogger_UserLogInfo(">>> StartShootPhoto start shoot photo");

    switch (s_cameraShootPhotoMode)
    {
        case PSDK_CAMERA_SHOOT_PHOTO_MODE_BURST:
            s_cameraState.shootingState=PSDK_CAMERA_SHOOTING_BURST_PHOTO;
            break;
        case PSDK_CAMERA_SHOOT_PHOTO_MODE_INTERVAL:
            s_cameraState.shootingState=PSDK_CAMERA_SHOOTING_INTERVAL_PHOTO;
            s_cameraState.isShootingIntervalStart = true;
            break;            
        case PSDK_CAMERA_SHOOT_PHOTO_MODE_SINGLE:
            s_cameraState.shootingState = PSDK_CAMERA_SHOOTING_SINGLE_PHOTO;
        default:
            break;
    }
    writefl(TAKEFILE,0);

    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_PsdkReturnCode StopShootPhoto(void)
{
    PsdkLogger_UserLogInfo(">>> StopShootPhoto stop shoot photo");
    s_cameraState.shootingState = PSDK_CAMERA_SHOOTING_PHOTO_IDLE;
    s_cameraState.isShootingIntervalStart = false;
    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_PsdkReturnCode SetShootPhotoMode(E_PsdkCameraShootPhotoMode mode)
{
    s_cameraShootPhotoMode = mode;
    PsdkLogger_UserLogInfo(">>> SetShootPhotoMode set shoot photo mode:%d", mode);

    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


static T_PsdkReturnCode SetPhotoBurstCount(E_PsdkCameraBurstCount burstCount)
{
    s_cameraBurstCount = burstCount;
    PsdkLogger_UserLogInfo(">>> SetPhotoBurstCount set photo burst count:%d", burstCount);
    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


static T_PsdkReturnCode SetPhotoTimeIntervalSettings(T_PsdkCameraPhotoTimeIntervalSettings settings)
{
    s_cameraPhotoTimeIntervalSettings.captureCount = settings.captureCount;
    s_cameraPhotoTimeIntervalSettings.timeIntervalSeconds = settings.timeIntervalSeconds;
    PsdkLogger_UserLogInfo(">>> SetPhotoTimeIntervalSettings set photo interval settings count:%d seconds:%d", settings.captureCount,
                            settings.timeIntervalSeconds);
    s_cameraState.currentPhotoShootingIntervalCount = settings.captureCount;

    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}



static T_PsdkReturnCode SetMeteringMode(E_PsdkCameraMeteringMode mode)
{
    PsdkLogger_UserLogInfo(">>> SetMeteringMode set metering mode:%d", mode);
    s_cameraMeteringMode = mode;

    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_PsdkReturnCode GetMeteringMode(E_PsdkCameraMeteringMode *mode)
{
    *mode = s_cameraMeteringMode;
    PsdkLogger_UserLogInfo(">>> GetMeteringMode GET metering mode:%d", mode);
    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_PsdkReturnCode SetSpotMeteringTarget(T_PsdkCameraSpotMeteringTarget target)
{
    PsdkLogger_UserLogInfo(">>> SetSpotMeteringTarget set spot metering area col:%d row:%d", target.col, target.row);
    memcpy(&s_cameraSpotMeteringTarget, &target, sizeof(T_PsdkCameraSpotMeteringTarget));

    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_PsdkReturnCode GetSpotMeteringTarget(T_PsdkCameraSpotMeteringTarget *target)
{
    memcpy(target, &s_cameraSpotMeteringTarget, sizeof(T_PsdkCameraSpotMeteringTarget));
    PsdkLogger_UserLogInfo(">>> GetSpotMeteringTarget GET spot metering area");

    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_PsdkReturnCode SetFocusMode(E_PsdkCameraFocusMode mode)
{
    PsdkLogger_UserLogInfo(">>> SetFocusMode set focus mode:%d", mode);
    s_cameraFocusMode = mode;

    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


static T_PsdkReturnCode SetFocusTarget(T_PsdkCameraPointInScreen target)
{
    PsdkLogger_UserLogInfo(">>> SetFocusTarget set focus target x:%.2f y:%.2f", target.focusX, target.focusY);
    memcpy(&s_cameraFocusTarget, &target, sizeof(T_PsdkCameraPointInScreen));
    writefl(FOCUSFILE,0);
    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


static T_PsdkReturnCode SetFocusAssistantSettings(T_PsdkCameraFocusAssistantSettings settings)
{
    PsdkLogger_UserLogInfo(">>> SetFocusAssistantSettings set focus assistant setting MF:%d AF:%d", settings.isEnabledMF, settings.isEnabledAF);
    memcpy(&s_cameraFocusAssistantSettings, &settings, sizeof(T_PsdkCameraFocusAssistantSettings));

    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


static T_PsdkReturnCode SetFocusRingValue(uint32_t value)
{
    PsdkLogger_UserLogInfo(">>> SetFocusRingValue set focus ring value:%d", value);
    s_cameraFocusRingValue = value;

    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}




static void *UserCamera_Task(void *arg)
{
    USER_UTIL_UNUSED(arg);
    int a;
    struct stat fstat;
    while (1) 
    {
        PsdkOsal_TaskSleepMs(1000);
            
        //PsdkLogger_UserLogInfo("[Inverval shooting] x_userIntervalTimeSeconds#:%f", x_userIntervalTimeSeconds);
        s_cameraState.currentPhotoShootingIntervalTimeInSeconds = x_userIntervalTimeSeconds != 0.f ? x_userIntervalTimeSeconds : s_cameraPhotoTimeIntervalSettings.timeIntervalSeconds;
    }
}

static void *UserIntervalShoot_Task(void *arg)
{
    USER_UTIL_UNUSED(arg);
    int a;
    struct stat fstat;
    const uint32_t timeToSleep = 100;
    while (1)
    {
        x_shootingSeries++;
        PsdkOsal_TaskSleepMs(timeToSleep);


        s_cameraState.shootingState = PSDK_CAMERA_SHOOTING_PHOTO_IDLE;
    }
}


/* Private functions definition-----------------------------------------------*/

T_PsdkReturnCode PsdkTest_CameraGetMode(E_PsdkCameraMode *mode)
{
    *mode = s_cameraState.cameraMode;
    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

bool PsdkTest_CameraIsInited(void)
{
    return s_isCamInited;
}


T_PsdkReturnCode PsdkTest_CameraInit(void)
{
    T_PsdkReturnCode returnCode;
    char ipAddr[PSDK_IP_ADDR_STR_SIZE_MAX] = {0};
    uint16_t port = 0;

    PsdkLogger_UserLogInfo(">>> CAMERA INIT START");


    s_cameraState.cameraMode=PSDK_CAMERA_MODE_SHOOT_PHOTO;
//    s_cameraState.shootingState=PSDK_CAMERA_SHOOT_PHOTO_MODE_SINGLE;
    s_cameraState.isStoring=false;
    s_cameraState.isShootingIntervalStart=false;
    s_cameraState.isRecording=false;
    s_cameraState.isOverheating=false;
    s_cameraState.hasError=false;
    s_cameraState.shootingState = PSDK_CAMERA_SHOOTING_PHOTO_IDLE;

    returnCode = PsdkOsal_MutexCreate(&s_commonMutex);
    if (returnCode != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        PsdkLogger_UserLogError("create mutex used to lock tap zoom arguments error: 0x%08llX", returnCode);
        return returnCode;
    }


    returnCode = PsdkPayloadCamera_Init();
    if (returnCode != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        PsdkLogger_UserLogError("payload camera init error:0x%08llX", returnCode);
    }

    /* Init the SDcard parameters */
    s_cameraSDCardState.isInserted = true;
    s_cameraSDCardState.totalSpaceInMB = 1000;
    s_cameraSDCardState.remainSpaceInMB = 1000;
    s_cameraSDCardState.availableCaptureCount = 1000;
    s_cameraSDCardState.availableRecordingTimeInSeconds = 1000;

    /* Register the camera common handler */
    s_commonHandler.GetSystemState = GetSystemState;
    s_commonHandler.SetMode = SetMode;
    s_commonHandler.GetMode = PsdkTest_CameraGetMode;
    s_commonHandler.StartRecordVideo = StartRecordVideo;
    s_commonHandler.StopRecordVideo = StopRecordVideo;
    s_commonHandler.StartShootPhoto = StartShootPhoto;
    s_commonHandler.StopShootPhoto = StopShootPhoto;
    s_commonHandler.SetShootPhotoMode = SetShootPhotoMode;
    s_commonHandler.GetShootPhotoMode = GetShootPhotoMode;
    s_commonHandler.SetPhotoBurstCount = SetPhotoBurstCount;
    s_commonHandler.GetPhotoBurstCount = GetPhotoBurstCount;
    s_commonHandler.SetPhotoTimeIntervalSettings = SetPhotoTimeIntervalSettings;
    s_commonHandler.GetPhotoTimeIntervalSettings = GetPhotoTimeIntervalSettings;
    s_commonHandler.GetSDCardState = GetSDCardState;
    s_commonHandler.FormatSDCard = FormatSDCard;

    returnCode = PsdkPayloadCamera_RegCommonHandler(&s_commonHandler);
    if (returnCode != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        PsdkLogger_UserLogError("camera register common handler error:0x%08llX", returnCode);
    }


    /* Register the camera focus handler */
    s_focusHandler.SetFocusMode = SetFocusMode;
    s_focusHandler.GetFocusMode = GetFocusMode;
    s_focusHandler.SetFocusTarget = SetFocusTarget;
    s_focusHandler.GetFocusTarget = GetFocusTarget;
    s_focusHandler.SetFocusAssistantSettings = SetFocusAssistantSettings;
    s_focusHandler.GetFocusAssistantSettings = GetFocusAssistantSettings;
    s_focusHandler.SetFocusRingValue = SetFocusRingValue;
    s_focusHandler.GetFocusRingValue = GetFocusRingValue;
    s_focusHandler.GetFocusRingValueUpperBound = GetFocusRingValueUpperBound;

    returnCode = PsdkPayloadCamera_RegFocusHandler(&s_focusHandler);
    if (returnCode != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        PsdkLogger_UserLogError("camera register adjustable focal point handler error:0x%08llX", returnCode);
        return returnCode;
    }

    /* Create the camera emu task */
    if (PsdkOsal_TaskCreate(&s_userCameraThread, UserCamera_Task, "user_camera_task",4096, NULL)
        != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        PsdkLogger_UserLogError("user camera task create error");
        return PSDK_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    if (PsdkOsal_TaskCreate(&s_userIntervalShootingThread, UserIntervalShoot_Task, "user_interval_shoot_task",4096, NULL)
        != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        PsdkLogger_UserLogError("user interval shoot task create error");
        return PSDK_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

	writefl(STOPVIDEOFILE,0);
    PsdkLogger_UserLogInfo(">>> CAMERA INIT END");

    s_isCamInited = true;

    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
