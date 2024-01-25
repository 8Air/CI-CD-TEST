#include <stdio.h>
#include <fcntl.h>
#include <dji_logger.h>
#include <dji_core.h>
#include <dji_platform.h>
#include <time.h>
#include "errno.h"
#include "utils/util_misc.h"

/* Private constants ---------------------------------------------------------*/
#define Dji_LOG_PATH                    "Logs/Dji_local"
#define Dji_LOG_INDEX_FILE_NAME         "Logs/latest"
#define Dji_LOG_FOLDER_NAME             "Logs"
#define Dji_LOG_PATH_MAX_SIZE           (128)
#define Dji_LOG_FOLDER_NAME_MAX_SIZE    (32)

/* Exported functions definition ---------------------------------------------*/
static FILE *s_DjiLogFile;
static FILE *s_DjiLogFileCnt;
//static pthread_t s_monitorThread = 0;


static T_DjiReturnCode DjiUser_Console(const uint8_t *data, uint16_t dataLen)
{
    USER_UTIL_UNUSED(dataLen);
    printf("%s", data);
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


static T_DjiReturnCode DjiUser_FillInUserInfo(T_DjiUserInfo *userInfo, const char *licFilePath)
{
	if (!licFilePath) {
		return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
	}
	
	char appname[33] = {0};     // Increase buffer size by 1
	char appid[17] = {0};       // Increase buffer size by 1
	char appkey[33] = {0};      // Increase buffer size by 1
	char appLicense[513] = {0}; // Increase buffer size by 1
	char appda[513] = {0};      // Increase buffer size due to observed longer input
	char baudRate[8] = {0};     // Increase buffer size by 1
	
	FILE *inf = NULL;
	inf = fopen(licFilePath, "r");
	if (inf == NULL) {
		USER_LOG_ERROR("Open %s failed: %s", licFilePath, strerror(errno));
		return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
	}
	
	
	fscanf(inf, "%32s", appname);
	fscanf(inf, "%16s", appid);
	fscanf(inf, "%32s", appkey);
	fscanf(inf, "%512s", appLicense);
	fscanf(inf, "%512s", appda);
	fscanf(inf, "%7s", baudRate);

	fclose(inf);
	
	// Truncate newline characters if present
	appname[strcspn(appname, "\n")] = 0;
	appid[strcspn(appid, "\n")] = 0;
	appkey[strcspn(appkey, "\n")] = 0;
	appLicense[strcspn(appLicense, "\n")] = 0;
	appda[strcspn(appda, "\n")] = 0;
	baudRate[strcspn(baudRate, "\n")] = 0;
	
	printf("Read %s: Name=`%s`, Id=`%s`, Key=`%s`, License=`%s`, Account=`%s`, Baud=`%s`\r\n",
	       licFilePath, appname, appid, appkey, appLicense, appda, baudRate);
	
	memset(userInfo->appName, 0, sizeof(userInfo->appName));
	memset(userInfo->appId, 0, sizeof(userInfo->appId));
	memset(userInfo->appKey, 0, sizeof(userInfo->appKey));
	memset(userInfo->appLicense, 0, sizeof(userInfo->appLicense));
	memset(userInfo->developerAccount, 0, sizeof(userInfo->developerAccount));
	memset(userInfo->baudRate, 0, sizeof(userInfo->baudRate));
	
	strncpy(userInfo->appName, appname, sizeof(userInfo->appName) - 1);
	memcpy(userInfo->appId, appid, USER_UTIL_MIN(sizeof(userInfo->appId), strlen(appid)));
	memcpy(userInfo->appKey, appkey, USER_UTIL_MIN(sizeof(userInfo->appKey), strlen(appkey)));
	memcpy(userInfo->appLicense, appLicense, USER_UTIL_MIN(sizeof(userInfo->appLicense), strlen(appLicense)));
	memcpy(userInfo->developerAccount, appda, USER_UTIL_MIN(sizeof(userInfo->developerAccount), strlen(appda)));
	memcpy(userInfo->baudRate, baudRate, USER_UTIL_MIN(sizeof(userInfo->baudRate), strlen(baudRate)));
	
	return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}



static T_DjiReturnCode DjiUser_LocalWrite(const uint8_t *data, uint16_t dataLen)
{
    int32_t realLen;

    if (s_DjiLogFile == NULL) {
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    }

    realLen = fwrite(data, 1, dataLen, s_DjiLogFile);
    fflush(s_DjiLogFile);
    if (realLen == dataLen) {
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    } else {
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    }
}

static T_DjiReturnCode DjiUser_FileSystemInit(const char *path)
{
    T_DjiReturnCode DjiStat = DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    char filePath[Dji_LOG_PATH_MAX_SIZE];
    char folderName[Dji_LOG_FOLDER_NAME_MAX_SIZE];
    time_t currentTime = time(NULL);
    struct tm *localTime = localtime(&currentTime);
    uint16_t logFileIndex = 0;
    uint16_t currentLogFileIndex = 0;
    uint8_t ret = 0;

    if (localTime == NULL) {
        printf("Get local time error.\r\n");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    }

    if (access(Dji_LOG_FOLDER_NAME, F_OK) != 0) {
        printf("Log file is not existed, need create new.\r\n");
        sprintf(folderName, "mkdir %s", Dji_LOG_FOLDER_NAME);
        ret = system(folderName);
        if (ret != 0) {
            printf("Create new log folder error, ret:%d.\r\n", ret);
            return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
        }
    }

    s_DjiLogFileCnt = fopen(Dji_LOG_INDEX_FILE_NAME, "rb+");
    if (s_DjiLogFileCnt == NULL) {
        s_DjiLogFileCnt = fopen(Dji_LOG_INDEX_FILE_NAME, "wb+");
        if (s_DjiLogFileCnt == NULL) {
            return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
        }
    } else {
        ret = fseek(s_DjiLogFileCnt, 0, SEEK_SET);
        if (ret != 0) {
            printf("Seek log count file error, ret: %d, errno: %d.\r\n", ret, errno);
            return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
        }

        ret = fread((uint16_t *) &logFileIndex, 1, sizeof(uint16_t), s_DjiLogFileCnt);
        if (ret != sizeof(uint16_t)) {
            printf("Read log file index error.\r\n");
        }
    }

    printf("Get current log index: %d\r\n", logFileIndex);
    currentLogFileIndex = logFileIndex;
    logFileIndex++;

    ret = fseek(s_DjiLogFileCnt, 0, SEEK_SET);
    if (ret != 0) {
        printf("Seek log file error, ret: %d, errno: %d.\r\n", ret, errno);
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    }

    ret = fwrite((uint16_t *) &logFileIndex, 1, sizeof(uint16_t), s_DjiLogFileCnt);
    if (ret != sizeof(uint16_t)) {
        printf("Write log file index error.\r\n");
        fclose(s_DjiLogFileCnt);
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    }

    fclose(s_DjiLogFileCnt);

    sprintf(filePath, "%s_%04d_%04d%02d%02d_%02d-%02d-%02d.log", path, currentLogFileIndex,
            localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday,
            localTime->tm_hour, localTime->tm_min, localTime->tm_sec);

    s_DjiLogFile = fopen(filePath, "wb+");
    if (s_DjiLogFile == NULL) {
	    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_WARN, "Open filepath time error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    }

    return DjiStat;
}

static void DjiUser_NormalExitHandler(int signalNum)
{
    USER_UTIL_UNUSED(signalNum);
    exit(0);
}
