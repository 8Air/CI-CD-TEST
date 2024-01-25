
/* Includes ------------------------------------------------------------------*/
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>

#include <psdk_logger.h>
#include <psdk_core.h>
#include <psdk_platform.h>
#include <psdk_typedef.h>
#include <psdk_data_subscription.h>
#include <psdk_time_sync.h>

#include <osal.h>
#include <hal_uart.h>
#include <hal_network.h>
#include <psdk_aircraft_info.h>
#include <psdk_power_management.h>
#include <psdk_payload_camera.h>
#include <psdk_xport.h>
#include <psdk_mop_channel.h>
#include <psdk_widget.h>


uint32_t ts_q_us=0;
uint32_t ts_q_ms=0;
double ts_q_0, ts_q_1, ts_q_2, ts_q_3;

uint32_t ts_t_us=0;
uint32_t ts_t_ms=0;
uint32_t ts_t_t=0;

uint32_t ts_d_us=0;
uint32_t ts_d_ms=0;
uint32_t ts_d_d=0;

uint32_t ts_c_us=0;
uint32_t ts_c_ms=0;
uint32_t ts_c_x=0;
uint32_t ts_c_y=0;
uint32_t ts_c_z=0;


#include "pu.c"
#include "power.c"
#include "camera.c"
#include "cammedia.c"
#include "test_widget.c"
#include "getdata.c"


int main(void)
{

    T_PsdkUserInfo userInfo;
//    const T_PsdkDataChannelBandwidthProportionOfHighspeedChannel bandwidthProportionOfHighspeedChannel =
//        {10, 60, 30};
    T_PsdkLoggerConsole printConsole = {
        .consoleLevel = PSDK_LOGGER_CONSOLE_LOG_LEVEL_INFO,
        .func = PsdkUser_Console,
    };

    T_PsdkLoggerConsole localRecordConsole = {
        .consoleLevel = PSDK_LOGGER_CONSOLE_LOG_LEVEL_INFO,
        .func = PsdkUser_LocalWrite,
    };

    T_PsdkHalUartHandler halUartHandler = {
        .UartInit = Hal_UartInit,
        .UartReadData = Hal_UartReadData,
        .UartWriteData = Hal_UartSendData,
    };

    T_PsdkHalNetWorkHandler halNetWorkHandler = {
        .NetWorkConfig = HalNetWork_Config,
    };

    T_PsdkOsalHandler osalHandler = {
        .Malloc = Osal_Malloc,
        .Free = Osal_Free,
        .TaskCreate = Osal_TaskCreate,
        .TaskDestroy = Osal_TaskDestroy,
        .TaskSleepMs = Osal_TaskSleepMs,
        .MutexCreate = Osal_MutexCreate,
        .MutexDestroy = Osal_MutexDestroy,
        .MutexLock = Osal_MutexLock,
        .MutexUnlock = Osal_MutexUnlock,
        .SemaphoreCreate = Osal_SemaphoreCreate,
        .SemaphoreDestroy = Osal_SemaphoreDestroy,
        .SemaphoreWait = Osal_SemaphoreWait,
        .SemaphorePost = Osal_SemaphorePost,
        .SemaphoreTimedWait = Osal_SemaphoreTimedWait,
        .GetTimeMs = Osal_GetTimeMs,
    };

    T_PsdkAircraftInfoBaseInfo aircraftBaseInfo = {0};


//    system("echo 11 > /sys/class/gpio/export");
//    system("echo 12 > /sys/class/gpio/export");
//    system("echo out > /sys/class/gpio/gpio11/direction");
//    system("echo out > /sys/class/gpio/gpio12/direction");
//    system("echo 0 > /sys/class/gpio/gpio11/value");
//    system("echo 0 > /sys/class/gpio/gpio12/value");



    if (PsdkPlatform_RegHalUartHandler(&halUartHandler) != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("psdk register hal uart handler error");
    }

    if (PsdkPlatform_RegHalNetworkHandler(&halNetWorkHandler) != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("psdk register hal network handler error");
    }

    if (PsdkPlatform_RegOsalHandler(&osalHandler) != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("psdk register osal handler error");
    }

    if (PsdkLogger_AddConsole(&printConsole) != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("psdk add console print error");
    }

    if (PsdkUser_FileSystemInit(PSDK_LOG_PATH) != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("psdk file system init error");
    }

    if (PsdkLogger_AddConsole(&localRecordConsole) != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("psdk add console record error");
    }

    if (PsdkUser_FillInUserInfo(&userInfo) != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("psdk fill in user info error");
    }

    if (PsdkCore_Init(&userInfo) != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        PsdkLogger_UserLogError("psdk instance init error");
    }

    if (PsdkProductInfo_SetAlias("Topodrone") != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        PsdkLogger_UserLogError("set product alias error.");
    }

    if (PsdkAircraftInfo_GetBaseInfo(&aircraftBaseInfo) != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        PsdkLogger_UserLogError("get aircraft information error.");
    }

    if (PsdkCore_ApplicationStart() != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        PsdkLogger_UserLogError("psdk application start error");
    }
	
    PsdkTest_PowerManagementInit();

    PsdkTest_CameraInit();

    PsdkTest_CameraMediaInit();

    PsdkTest_WidgetInit();

//    PsdkTest_DataSubscriptionInit();

    signal(SIGTERM, PsdkUser_NormalExitHandler);

    while (1) {
        sleep(1);
    }
}

