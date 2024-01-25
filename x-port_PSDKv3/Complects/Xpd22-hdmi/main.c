
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
#include <osal_socket.h>
#include <hal_uart.h>
#include <hal_network.h>
#include "osal_socket.h"
#include <dji_power_management.h>
#include <dji_payload_camera.h>
#include <dji_xport.h>
#include <dji_mop_channel.h>
#include "pu.c"
#include "power.c"
#include "camera.c"
#include "cammedia.c"
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{

    T_DjiUserInfo userInfo;

    T_DjiReturnCode returnCode, ret;
    T_DjiFirmwareVersion firmwareVersion = {
        .majorVersion = 1,
        .minorVersion = 0,
        .modifyVersion = 0,
        .debugVersion = 0,
    };

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
	
    T_DjiHalNetworkHandler networkHandler = {
        .NetworkInit = HalNetWork_Init,
        .NetworkDeInit = HalNetWork_DeInit,
        .NetworkGetDeviceInfo = HalNetWork_GetDeviceInfo,
    };


    T_DjiHalUartHandler uartHandler = {
        .UartInit = HalUart_Init,
        .UartDeInit = HalUart_DeInit,
        .UartWriteData = HalUart_WriteData,
        .UartReadData = HalUart_ReadData,
        .UartGetStatus = HalUart_GetStatus,
    };
	
    T_DjiOsalHandler osalHandler = {
        .TaskCreate = Osal_TaskCreate,
        .TaskDestroy = Osal_TaskDestroy,
        .TaskSleepMs = Osal_TaskSleepMs,
        .MutexCreate= Osal_MutexCreate,
        .MutexDestroy = Osal_MutexDestroy,
        .MutexLock = Osal_MutexLock,
        .MutexUnlock = Osal_MutexUnlock,
        .SemaphoreCreate = Osal_SemaphoreCreate,
        .SemaphoreDestroy = Osal_SemaphoreDestroy,
        .SemaphoreWait = Osal_SemaphoreWait,
        .SemaphoreTimedWait = Osal_SemaphoreTimedWait,
        .SemaphorePost = Osal_SemaphorePost,
        .Malloc = Osal_Malloc,
        .Free = Osal_Free,
        .GetRandomNum = Osal_GetRandomNum,
        .GetTimeMs = Osal_GetTimeMs,
        .GetTimeUs = Osal_GetTimeUs,
    };

    T_DjiSocketHandler socketHandler = {
        .Socket = Osal_Socket,
        .Bind = Osal_Bind,
        .Close = Osal_Close,
        .UdpSendData = Osal_UdpSendData,
        .UdpRecvData = Osal_UdpRecvData,
        .TcpListen = Osal_TcpListen,
        .TcpAccept = Osal_TcpAccept,
        .TcpConnect = Osal_TcpConnect,
        .TcpSendData = Osal_TcpSendData,
        .TcpRecvData = Osal_TcpRecvData,
    };

    T_DjiAircraftInfoBaseInfo aircraftBaseInfo = {0};


    returnCode = DjiPlatform_RegOsalHandler(&osalHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("register osal handler error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    returnCode = DjiPlatform_RegHalUartHandler(&uartHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("register hal uart handler error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    returnCode = DjiLogger_AddConsole(&printConsole);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("add printf console error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    returnCode = DjiLogger_AddConsole(&localRecordConsole);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("add printf console error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    returnCode = DjiPlatform_RegHalNetworkHandler(&networkHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("register hal network handler error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    //Attention: if you want to use camera stream view function, please uncomment it.
    returnCode = DjiPlatform_RegSocketHandler(&socketHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("register osal socket handler error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

	
	ret = DjiUser_FillInUserInfo(&userInfo, licFilePath);
	if (ret != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) 
	{
		printf("psdk fill in user info error: %lld\n", ret);
	}
	
	ret = DjiCore_Init(&userInfo);
	if (ret != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) 
	{
        	DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,"psdk instance init error: %d\n", ret);
	}
	
	ret = DjiCore_SetAlias("Topodrone");
	if (ret != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) 
	{
		DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,"set product alias error: %d\n", ret);
	}
	
	ret = DjiAircraftInfo_GetBaseInfo(&aircraftBaseInfo);
	if (ret != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) 
	{
		DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,"get aircraft information error: %d\n", ret);
	}
	
	ret = DjiCore_ApplicationStart();
	if (ret != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
		DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,"psdk application start error: %d\n", ret);
	}
	
	DjiTest_PowerManagementInit();
	
	DjiTest_CameraInit();
	
	DjiTest_CameraMediaInit();
	
	signal(SIGTERM, DjiUser_NormalExitHandler);

    while (1) {
        sleep(1);
    }
}
