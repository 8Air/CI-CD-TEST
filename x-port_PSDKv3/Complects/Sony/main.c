
/* Includes ------------------------------------------------------------------*/
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>

#include <dji_logger.h>
#include <dji_core.h>
#include <dji_platform.h>
#include <dji_typedef.h>
#include <dji_time_sync.h>

#include <osal.h>
#include <hal_uart.h>
#include <hal_network.h>
#include <dji_power_management.h>
#include <dji_payload_camera.h>
#include <dji_xport.h>
#include <dji_mop_channel.h>
#include <dji_widget.h>


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


#include <pu.c>
#include <power.c>
#include "camera.c"
#include "cammedia.c"
#include "test_widget.c"
#include <getdata.c>


#include <errno.h>
#include <unistd.h>

/*
 * To correct work all buffer-files must be created
 * */

#define LIC_READNIG_ERROR 0

int main(void)
{
    if (initPath()) {
        return 1;
    }
	
	
	T_DjiUserInfo userInfo;
//    const T_DjiDataChannelBandwidthProportionOfHighspeedChannel bandwidthProportionOfHighspeedChannel =
//        {10, 60, 30};
    T_DjiLoggerConsole printConsole = {
        .consoleLevel = DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,
        .func = DjiUser_Console,
    };

    T_DjiLoggerConsole localRecordConsole = {
            .func = DjiUser_LocalWrite,
		    .consoleLevel = DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,
			.isSupportColor = true,
    };
	
	T_DjiHalUartHandler halUartHandler = {
			.UartInit = HalUart_Init,
			.UartDeInit = HalUart_DeInit,
			.UartWriteData = HalUart_WriteData,
			.UartReadData = HalUart_ReadData,
			.UartGetStatus = HalUart_GetStatus,
	};
	
	T_DjiHalNetworkHandler halNetWorkHandler = {
        .NetworkInit = HalNetWork_Init,
        .NetworkDeInit = HalNetWork_DeInit,
        .NetworkGetDeviceInfo = HalNetWork_GetDeviceInfo,
    };
	 
    T_DjiOsalHandler osalHandler = {
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
		    .SemaphoreTimedWait = Osal_SemaphoreTimedWait,
		    .SemaphorePost = Osal_SemaphorePost,
		    .GetTimeMs = Osal_GetTimeMs,
		    .GetTimeUs = Osal_GetTimeUs,
			.GetRandomNum = Osal_GetRandomNum,
	        .Malloc = Osal_Malloc,
	        .Free = Osal_Free,
    };
	
	T_DjiAircraftInfoBaseInfo aircraftBaseInfo = {0};
	
	T_DjiReturnCode ret = DjiPlatform_RegHalUartHandler(&halUartHandler);
	if (ret != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
		printf("psdk register hal uart handler error: %d\n", ret);
	}

	ret = DjiPlatform_RegHalNetworkHandler(&halNetWorkHandler);
    if (ret != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("psdk register hal network handler error: %d\n", ret);
    }
	
	ret = DjiPlatform_RegOsalHandler(&osalHandler);
    if (ret != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("psdk register osal handler error: %d\n", ret);
    }
	
	ret = DjiLogger_AddConsole(&printConsole);
    if (ret != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("psdk add console print error: %d\n", ret);
    }

	ret = DjiUser_FileSystemInit(Dji_LOG_PATH);
    if (ret != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("psdk file system init error: %d\n", ret);
    }
	
	ret = DjiLogger_AddConsole(&localRecordConsole);
    if (ret != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("psdk add console record error: %d\n", ret);
    }
	
	ret = DjiUser_FillInUserInfo(&userInfo, licFilePath);
    if (ret != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("psdk fill in user info error: %d\n", ret);
    }
    if (userInfo.appName[0]== '\0') {
        printf("%s file is not exist or key is wrong\n", licFilePath);
        return LIC_READNIG_ERROR;
    }
    printf("Name=`%s`, Id=`%s`, Key=`%s`, License=`%s`, Account=`%s`, Baud=`%s`\r\n",
           userInfo.appName, userInfo.appId, userInfo.appKey,
           userInfo.appLicense, userInfo.developerAccount, userInfo.baudRate);
	
	ret = DjiCore_Init(&userInfo);
    if (ret != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,"psdk instance init error: %d\n", ret);
    }
	printf("%d\r\n", __LINE__);
	
	ret = DjiCore_SetAlias("Topodrone");
	if (ret != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
		DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,"set product alias error: %d\n", ret);
	}
	printf("%d\r\n", __LINE__);
	
	ret = DjiAircraftInfo_GetBaseInfo(&aircraftBaseInfo);
	printf("%d\r\n", __LINE__);
	if (ret != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
		DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,"get aircraft information error: %d\n", ret);
	}
	
	printf("%d\r\n", __LINE__);
	ret = DjiCore_ApplicationStart();
	printf("%d\r\n", __LINE__);
	if (ret != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
		DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,"psdk application start error: %d\n", ret);
	}
	
	printf("%d\r\n", __LINE__);
	DjiTest_PowerManagementInit();
	
	printf("%d\r\n", __LINE__);
	//DjiTest_CameraInit();
	
	//DjiTest_GPSInit();
	
	//DjiTest_CameraMediaInit();
	
	printf("%d\r\n", __LINE__);
	DjiTest_WidgetInit();
	
	printf("%d\r\n", __LINE__);
	DjiTest_DataSubscriptionInit();
	
	signal(SIGTERM, DjiUser_NormalExitHandler);

    while (1) {
        sleep(1);
    }
}

