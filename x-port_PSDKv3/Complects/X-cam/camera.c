#include <common.h>
#include "constants.h"
#include "math.h"
#include "utils/util_misc.h"
#include "dji_logger.h"
#include "dji_platform.h"
#include "dji_payload_camera.h"
#include "dji_aircraft_info.h"
#include "dji_gimbal.h"
#include "dji_xport.h"
/* Private variables ---------------------------------------------------------*/
static bool s_isCamInited = false;

static int x_PT=0;
static int x_shootingSeries=0;
static float x_userIntervalTimeSeconds = 0.f;

static bool x_InitPT = false;

static T_DjiCameraCommonHandler s_commonHandler;
static T_DjiCameraFocusHandler s_focusHandler;
static T_DjiCameraExposureMeteringHandler s_exposureMeteringHandler;
static T_DjiCameraTapZoomHandler s_tapZoomHandler;
static T_DjiCameraDigitalZoomHandler s_digitalZoomHandler;
static T_DjiCameraOpticalZoomHandler s_opticalZoomHandler;


static T_DjiReturnCode SetDigitalZoomFactor(dji_f32_t factor);
static T_DjiReturnCode SetOpticalZoomFocalLength(uint32_t focalLength);
static T_DjiReturnCode GetOpticalZoomFocalLength(uint32_t *focalLength);
static T_DjiReturnCode GetOpticalZoomSpec(T_DjiCameraOpticalZoomSpec *spec);
static T_DjiReturnCode StartContinuousOpticalZoom(E_DjiCameraZoomDirection direction, E_DjiCameraZoomSpeed speed);
static T_DjiReturnCode StopContinuousOpticalZoom(void);


static T_DjiTaskHandle s_userCameraThread;
static T_DjiTaskHandle s_userIntervalShootingThread;

static T_DjiCameraSystemState s_cameraState;
static E_DjiCameraShootPhotoMode s_cameraShootPhotoMode = DJI_CAMERA_SHOOT_PHOTO_MODE_SINGLE;
static E_DjiCameraBurstCount s_cameraBurstCount = DJI_CAMERA_BURST_COUNT_2;
static T_DjiCameraPhotoTimeIntervalSettings s_cameraPhotoTimeIntervalSettings = {0, 1};
static T_DjiCameraSDCardState s_cameraSDCardState = {0};
static T_DjiMutexHandle s_commonMutex = {0};

static E_DjiCameraMeteringMode s_cameraMeteringMode = DJI_CAMERA_METERING_MODE_CENTER;
static T_DjiCameraSpotMeteringTarget s_cameraSpotMeteringTarget = {0};

static bool s_isTapZoomEnabled = false;
static T_DjiCameraTapZoomState s_cameraTapZoomState = {0};
static uint8_t s_tapZoomMultiplier = 1;
static uint32_t s_tapZoomStartTime = 0;
static bool s_isStartTapZoom = false;
static bool s_isTapZooming = false;

static E_DjiCameraFocusMode s_cameraFocusMode = DJI_CAMERA_FOCUS_MODE_AUTO;
static T_DjiCameraPointInScreen s_cameraFocusTarget = {0};
static uint32_t s_cameraFocusRingValue = 1;
static T_DjiCameraFocusAssistantSettings s_cameraFocusAssistantSettings = {0};

/* Private functions declaration ---------------------------------------------*/
static T_DjiReturnCode GetSystemState(T_DjiCameraSystemState *systemState);
static T_DjiReturnCode SetMode(E_DjiCameraMode mode);
static T_DjiReturnCode StartRecordVideo(void);
static T_DjiReturnCode StopRecordVideo(void);
static T_DjiReturnCode StartShootPhoto(void);
static T_DjiReturnCode StopShootPhoto(void);
static T_DjiReturnCode SetShootPhotoMode(E_DjiCameraShootPhotoMode mode);
static T_DjiReturnCode GetShootPhotoMode(E_DjiCameraShootPhotoMode *mode);
static T_DjiReturnCode SetPhotoBurstCount(E_DjiCameraBurstCount burstCount);
static T_DjiReturnCode GetPhotoBurstCount(E_DjiCameraBurstCount *burstCount);
static T_DjiReturnCode SetPhotoTimeIntervalSettings(T_DjiCameraPhotoTimeIntervalSettings settings);
static T_DjiReturnCode GetPhotoTimeIntervalSettings(T_DjiCameraPhotoTimeIntervalSettings *settings);
static T_DjiReturnCode GetSDCardState(T_DjiCameraSDCardState *sdCardState);
static T_DjiReturnCode FormatSDCard(void);

static T_DjiReturnCode SetFocusMode(E_DjiCameraFocusMode mode);
static T_DjiReturnCode GetFocusMode(E_DjiCameraFocusMode *mode);
static T_DjiReturnCode SetFocusTarget(T_DjiCameraPointInScreen target);
static T_DjiReturnCode GetFocusTarget(T_DjiCameraPointInScreen *target);
static T_DjiReturnCode SetFocusAssistantSettings(T_DjiCameraFocusAssistantSettings settings);
static T_DjiReturnCode GetFocusAssistantSettings(T_DjiCameraFocusAssistantSettings *settings);
static T_DjiReturnCode SetFocusRingValue(uint32_t value);
static T_DjiReturnCode GetFocusRingValue(uint32_t *value);
static T_DjiReturnCode GetFocusRingValueUpperBound(uint32_t *value);

static void *UserCamera_Task(void *arg);
static void *UserIntervalShoot_Task(void *arg);

//POLL
static T_DjiReturnCode GetSystemState(T_DjiCameraSystemState *systemState) //POLL!!!
{
    *systemState = s_cameraState;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode GetShootPhotoMode(E_DjiCameraShootPhotoMode *mode) //POLL!!!
{
    *mode = s_cameraShootPhotoMode;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode GetPhotoBurstCount(E_DjiCameraBurstCount *burstCount) //POLL!!!
{
    *burstCount = s_cameraBurstCount;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


static T_DjiReturnCode GetPhotoTimeIntervalSettings(T_DjiCameraPhotoTimeIntervalSettings *settings)  //POLL!!!!
{
    memcpy(settings, &s_cameraPhotoTimeIntervalSettings, sizeof(T_DjiCameraPhotoTimeIntervalSettings));
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode GetSDCardState(T_DjiCameraSDCardState *sdCardState) //POLL!!!!
{
    memcpy(sdCardState, &s_cameraSDCardState, sizeof(T_DjiCameraSDCardState));
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode GetFocusMode(E_DjiCameraFocusMode *mode) //POLL!!!
{
    *mode = s_cameraFocusMode;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode GetFocusTarget(T_DjiCameraPointInScreen *target) //POLL!!!
{
    memcpy(target, &s_cameraFocusTarget, sizeof(T_DjiCameraPointInScreen));
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode GetFocusAssistantSettings(T_DjiCameraFocusAssistantSettings *settings) //POLL!!!
{
    memcpy(settings, &s_cameraFocusAssistantSettings, sizeof(T_DjiCameraFocusAssistantSettings));
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


static T_DjiReturnCode GetFocusRingValue(uint32_t *value) //POLL!!!
{
    *value = s_cameraFocusRingValue;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode GetFocusRingValueUpperBound(uint32_t *value) //POLL!!!
{
    *value = 1;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


// SET

static T_DjiReturnCode SetMode(E_DjiCameraMode mode)
{
    s_cameraState.cameraMode = mode;
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> SetMode set camera mode:%d", mode);
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode FormatSDCard(void)
{
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> FormatSDCard Format SD Card");
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


static T_DjiReturnCode StartRecordVideo(void)
{
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> StartRecordVideo start record video");
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode StopRecordVideo(void)
{
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> StopRecordVideo stop record video");
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode StartShootPhoto(void)
{
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> StartShootPhoto start shoot photo");

//    system("echo 1 > /sys/class/gpio/gpio11/value");
    switch (s_cameraShootPhotoMode)
    {
        case DJI_CAMERA_SHOOT_PHOTO_MODE_BURST:
            s_cameraState.shootingState=DJI_CAMERA_SHOOTING_BURST_PHOTO;
            break;
        case DJI_CAMERA_SHOOT_PHOTO_MODE_INTERVAL:
            s_cameraState.shootingState=DJI_CAMERA_SHOOTING_INTERVAL_PHOTO;
            s_cameraState.isShootingIntervalStart = true;
            break;            
        case DJI_CAMERA_SHOOT_PHOTO_MODE_SINGLE:
            s_cameraState.shootingState = DJI_CAMERA_SHOOTING_SINGLE_PHOTO;
        default:
            break;
    }
    writefl(takeFile,0, false, false, 0);
    x_InitPT = true;

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode StopShootPhoto(void)
{
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> StopShootPhoto stop shoot photo");
    s_cameraState.shootingState = DJI_CAMERA_SHOOTING_PHOTO_IDLE;
    s_cameraState.isShootingIntervalStart = false;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode SetShootPhotoMode(E_DjiCameraShootPhotoMode mode)
{
    s_cameraShootPhotoMode = mode;
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> SetShootPhotoMode set shoot photo mode:%d", mode);

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


static T_DjiReturnCode SetPhotoBurstCount(E_DjiCameraBurstCount burstCount)
{
    s_cameraBurstCount = burstCount;
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> SetPhotoBurstCount set photo burst count:%d", burstCount);
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


static T_DjiReturnCode SetPhotoTimeIntervalSettings(T_DjiCameraPhotoTimeIntervalSettings settings)
{
    s_cameraPhotoTimeIntervalSettings.captureCount = settings.captureCount;
    s_cameraPhotoTimeIntervalSettings.timeIntervalSeconds = settings.timeIntervalSeconds;
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> SetPhotoTimeIntervalSettings set photo interval settings count:%d seconds:%d", settings.captureCount,
                            settings.timeIntervalSeconds);
    s_cameraState.currentPhotoShootingIntervalCount = settings.captureCount;

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}



static T_DjiReturnCode SetMeteringMode(E_DjiCameraMeteringMode mode)
{
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> SetMeteringMode set metering mode:%d", mode);
    s_cameraMeteringMode = mode;

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode GetMeteringMode(E_DjiCameraMeteringMode *mode)
{
    *mode = s_cameraMeteringMode;
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> GetMeteringMode GET metering mode:%d", mode);
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode SetSpotMeteringTarget(T_DjiCameraSpotMeteringTarget target)
{
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> SetSpotMeteringTarget set spot metering area col:%d row:%d", target.col, target.row);
    memcpy(&s_cameraSpotMeteringTarget, &target, sizeof(T_DjiCameraSpotMeteringTarget));

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode GetSpotMeteringTarget(T_DjiCameraSpotMeteringTarget *target)
{
    memcpy(target, &s_cameraSpotMeteringTarget, sizeof(T_DjiCameraSpotMeteringTarget));
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> GetSpotMeteringTarget GET spot metering area");

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode SetFocusMode(E_DjiCameraFocusMode mode)
{
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> SetFocusMode set focus mode:%d", mode);
    s_cameraFocusMode = mode;

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


static T_DjiReturnCode SetFocusTarget(T_DjiCameraPointInScreen target)
{
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> SetFocusTarget set focus target x:%.2f y:%.2f", target.focusX, target.focusY);
    memcpy(&s_cameraFocusTarget, &target, sizeof(T_DjiCameraPointInScreen));
    writefl(focusFile,0, false, false, 0);
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


static T_DjiReturnCode SetFocusAssistantSettings(T_DjiCameraFocusAssistantSettings settings)
{
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> SetFocusAssistantSettings set focus assistant setting MF:%d AF:%d", settings.isEnabledMF, settings.isEnabledAF);
    memcpy(&s_cameraFocusAssistantSettings, &settings, sizeof(T_DjiCameraFocusAssistantSettings));

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


static T_DjiReturnCode SetFocusRingValue(uint32_t value)
{
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> SetFocusRingValue set focus ring value:%d", value);
    s_cameraFocusRingValue = value;

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}



static T_DjiReturnCode GetTapZoomState(T_DjiCameraTapZoomState *state)
{
    T_DjiReturnCode psdkStat;
    memcpy(state, &s_cameraTapZoomState, sizeof(T_DjiCameraTapZoomState));
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode SetTapZoomEnabled(bool enabledFlag)
{
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,"set tap zoom enabled flag: %d.", enabledFlag);
    s_isTapZoomEnabled = enabledFlag;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode GetTapZoomEnabled(bool *enabledFlag)
{
    *enabledFlag = s_isTapZoomEnabled;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode SetTapZoomMultiplier(uint8_t multiplier)
{
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,"set tap zoom multiplier: %d.", multiplier);
    s_tapZoomMultiplier = multiplier;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode GetTapZoomMultiplier(uint8_t *multiplier)
{
    *multiplier = s_tapZoomMultiplier;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode TapZoomAtTarget(T_DjiCameraPointInScreen target)
{
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,"tap zoom at target: x %f, y %f.", target.focusX, target.focusY);
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}



static void *UserCamera_Task(void *arg)
{
    USER_UTIL_UNUSED(arg);
    int a;
    struct stat fstat;
    while (1)
    {
        Osal_TaskSleepMs(1000);
        a=readfl(infoFMFile, true);
        if ((a!=0)&&(s_cameraFocusMode==DJI_CAMERA_FOCUS_MODE_AUTO)) writefl(setFMFile,0, false, false, 0);
        if ((a!=4)&&(s_cameraFocusMode==DJI_CAMERA_FOCUS_MODE_MANUAL)) writefl(setFMFile,4, false, false, 0);

        a = readfl(setIntervalFile, true);
        if(1000 == a || 0 == a)
            x_userIntervalTimeSeconds = 0.f;
        else
            x_userIntervalTimeSeconds = interval_values[a - 1];

        //PsdkLogger_UserLogInfo("[Inverval shooting] x_userIntervalTimeSeconds#:%f", x_userIntervalTimeSeconds);
        s_cameraState.currentPhotoShootingIntervalTimeInSeconds = x_userIntervalTimeSeconds != 0.f ? x_userIntervalTimeSeconds : s_cameraPhotoTimeIntervalSettings.timeIntervalSeconds;
    }
}

#define NEED_MAKE_SHOOT(interval, current_tick) (interval && (current_tick % (uint32_t)(interval * 1000)) == 0)   
static void *UserIntervalShoot_Task(void *arg)
{
    USER_UTIL_UNUSED(arg);
    int a;
    struct stat fstat;
    const uint32_t timeToSleep = 100;
    bool needMakeShoot = false;
    while (1)
    {
        x_shootingSeries++;
        Osal_TaskSleepMs(timeToSleep);
        needMakeShoot = (x_userIntervalTimeSeconds == 0.f && NEED_MAKE_SHOOT(s_cameraPhotoTimeIntervalSettings.timeIntervalSeconds, timeToSleep * x_shootingSeries)) ||
                        (NEED_MAKE_SHOOT(x_userIntervalTimeSeconds, timeToSleep * x_shootingSeries));

        if (s_cameraState.isShootingIntervalStart && needMakeShoot)
        {
            writefl(takeFile,0, false, false, 0);
            DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,"[Inverval shooting] Shoot#:%u", x_shootingSeries);
        }
        else if (s_cameraState.isShootingIntervalStart == false)
            x_shootingSeries = 0;


        s_cameraState.shootingState = DJI_CAMERA_SHOOTING_PHOTO_IDLE;
    }
}



T_DjiReturnCode DjiTest_CameraGetDigitalZoomFactor(dji_f32_t *factor)
{
    T_DjiReturnCode psdkStat;
    *factor = 1;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode DjiTest_CameraGetOpticalZoomFactor(dji_f32_t *factor)
{
    T_DjiReturnCode psdkStat;
    //Formula:factor = currentFocalLength / minFocalLength
    *factor = 1;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode SetDigitalZoomFactor(dji_f32_t factor)
{
    T_DjiReturnCode psdkStat;
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,"set digital zoom factor:%.2f", factor);
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode SetOpticalZoomFocalLength(uint32_t focalLength)
{
    T_DjiReturnCode psdkStat;
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,"set optical zoom focal length:%d", focalLength);
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode GetOpticalZoomFocalLength(uint32_t *focalLength)
{
    T_DjiReturnCode psdkStat;
    *focalLength = 50;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode GetOpticalZoomSpec(T_DjiCameraOpticalZoomSpec *spec)
{
    spec->maxFocalLength = 50;
    spec->minFocalLength = 50;
    spec->focalLengthStep = 0;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode StartContinuousOpticalZoom(E_DjiCameraZoomDirection direction, E_DjiCameraZoomSpeed speed)
{
    T_DjiReturnCode psdkStat;
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,"start continuous optical zoom direction:%d speed:%d", direction, speed);
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode StopContinuousOpticalZoom(void)
{
    T_DjiReturnCode DjiStat;
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,"stop continuous optical zoom");
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


/* Private functions definition-----------------------------------------------*/

T_DjiReturnCode DjiTest_CameraGetMode(E_DjiCameraMode *mode)
{
    *mode = s_cameraState.cameraMode;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

bool DjiTest_CameraIsInited(void)
{
    return s_isCamInited;
}


T_DjiReturnCode DjiTest_CameraInit(void)
{
    T_DjiReturnCode returnCode;
    char ipAddr[DJI_IP_ADDR_STR_SIZE_MAX] = {0};
    uint16_t port = 0;

    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> CAMERA INIT START");


    s_cameraState.cameraMode=DJI_CAMERA_MODE_SHOOT_PHOTO;
//    s_cameraState.shootingState=DJI_CAMERA_SHOOT_PHOTO_MODE_SINGLE;
    s_cameraState.isStoring=false;
    s_cameraState.isShootingIntervalStart=false;
    s_cameraState.isRecording=false;
    s_cameraState.isOverheating=false;
    s_cameraState.hasError=false;
    s_cameraState.shootingState = DJI_CAMERA_SHOOTING_PHOTO_IDLE;

    returnCode = Osal_MutexCreate(&s_commonMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_ERROR, "create mutex used to lock tap zoom arguments error: 0x%08llX", returnCode);
        return returnCode;
    }


    returnCode = DjiPayloadCamera_Init();
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_ERROR, "payload camera init error:0x%08llX", returnCode);
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
    s_commonHandler.GetMode = DjiTest_CameraGetMode;
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

    returnCode = DjiPayloadCamera_RegCommonHandler(&s_commonHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_ERROR, "camera register common handler error:0x%08llX", returnCode);
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

    returnCode = DjiPayloadCamera_RegFocusHandler(&s_focusHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_ERROR, "camera register adjustable focal point handler error:0x%08llX", returnCode);
        return returnCode;
    }

    s_opticalZoomHandler.SetOpticalZoomFocalLength = SetOpticalZoomFocalLength;
    s_opticalZoomHandler.GetOpticalZoomFocalLength = GetOpticalZoomFocalLength;
    s_opticalZoomHandler.GetOpticalZoomFactor = DjiTest_CameraGetOpticalZoomFactor;
    s_opticalZoomHandler.GetOpticalZoomSpec = GetOpticalZoomSpec;
    s_opticalZoomHandler.StartContinuousOpticalZoom = StartContinuousOpticalZoom;
    s_opticalZoomHandler.StopContinuousOpticalZoom = StopContinuousOpticalZoom;

    returnCode = DjiPayloadCamera_RegOpticalZoomHandler(&s_opticalZoomHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_ERROR, "camera register optical zoom handler error:0x%08llX", returnCode);
        return returnCode;
    }

    /* Register the camera tap zoom handler */
    s_tapZoomHandler.GetTapZoomState = GetTapZoomState;
    s_tapZoomHandler.SetTapZoomEnabled = SetTapZoomEnabled;
    s_tapZoomHandler.GetTapZoomEnabled = GetTapZoomEnabled;
    s_tapZoomHandler.SetTapZoomMultiplier = SetTapZoomMultiplier;
    s_tapZoomHandler.GetTapZoomMultiplier = GetTapZoomMultiplier;
    s_tapZoomHandler.TapZoomAtTarget = TapZoomAtTarget;

    returnCode = DjiPayloadCamera_RegTapZoomHandler(&s_tapZoomHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_ERROR, "camera register tap zoom handler error:0x%08llX", returnCode);
        return returnCode;
    }

    /* Create the camera emu task */
    if (Osal_TaskCreate(&s_userCameraThread, UserCamera_Task, 4096,"user_camera_task", NULL)
        != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_ERROR, "user camera task create error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    if (Osal_TaskCreate(&s_userIntervalShootingThread, UserIntervalShoot_Task, 4096,"user_interval_shoot_task", NULL)
        != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_ERROR, "user interval shoot task create error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>> CAMERA INIT END");

    s_isCamInited = true;

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
