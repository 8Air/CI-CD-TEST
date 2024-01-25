#include "common.h"
T_DjiReturnCode DjiTest_WidgetInit(void);
T_DjiReturnCode DjiTest_WidgetSetConfigFilePath(const char *path);

#define WIDGET_DIR_PATH_LEN_MAX         (256)
#define WIDGET_TASK_STACK_SIZE          (2048)


static void *DjiTest_WidgetTask(void *arg);
static T_DjiReturnCode DjiTestWidget_SetWidgetValue(E_DjiWidgetType widgetType, uint32_t index, int32_t value,
                                                      void *userData);
static T_DjiReturnCode DjiTestWidget_GetWidgetValue(E_DjiWidgetType widgetType, uint32_t index, int32_t *value,
                                                      void *userData);

static T_DjiTaskHandle s_widgetTestThread;
static char s_widgetFileDirPath[DJI_FILE_PATH_SIZE_MAX] = {0};


static const T_DjiWidgetHandlerListItem s_widgetHandlerList[] = {
    {0, DJI_WIDGET_TYPE_LIST,          DjiTestWidget_SetWidgetValue, DjiTestWidget_GetWidgetValue, NULL},
    {1, DJI_WIDGET_TYPE_LIST,          DjiTestWidget_SetWidgetValue, DjiTestWidget_GetWidgetValue, NULL},
    {2, DJI_WIDGET_TYPE_LIST,          DjiTestWidget_SetWidgetValue, DjiTestWidget_GetWidgetValue, NULL},
    {3, DJI_WIDGET_TYPE_LIST,          DjiTestWidget_SetWidgetValue, DjiTestWidget_GetWidgetValue, NULL},
    {4, DJI_WIDGET_TYPE_LIST,          DjiTestWidget_SetWidgetValue, DjiTestWidget_GetWidgetValue, NULL},
};

static char *s_widgetTypeNameArray[] = {
    "Unknown",
    "Button",
    "Switch",
    "Scale",
    "List",
    "Int input box"
};

static const uint32_t s_widgetHandlerListCount = sizeof(s_widgetHandlerList) / sizeof(T_DjiWidgetHandlerListItem);
static int32_t s_widgetValueList[4] = {0};

/* Exported functions definition ---------------------------------------------*/
T_DjiReturnCode DjiTest_WidgetInit(void)
{
    T_DjiReturnCode DjiStat;
    
    printf("%d\r\n", __LINE__);
    //Step 1 : Init Dji Widget
    DjiStat = DjiWidget_Init();
    if (DjiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_ERROR,"Init widget error, stat = 0x%08llX", DjiStat);
        return DjiStat;
    }
    printf("%d\r\n", __LINE__);

    //Step 2 : Set UI Config (Linux environment)

    char curFileDirPath[WIDGET_DIR_PATH_LEN_MAX];
    char tempPath[WIDGET_DIR_PATH_LEN_MAX];
    
    printf("%d\r\n", __LINE__);
    DjiStat = DjiUserUtil_GetCurrentFileDirPath(__FILE__,
												WIDGET_DIR_PATH_LEN_MAX,
												curFileDirPath);
    
    printf("%d\r\n", __LINE__);
    snprintf(tempPath, WIDGET_DIR_PATH_LEN_MAX, "/xport/apptest-camera/widget/");
    
    printf("%d\r\n", __LINE__);
    //set default ui config path
    DjiStat = DjiWidget_RegDefaultUiConfigByDirPath(tempPath);
    if (DjiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_ERROR,"Add default widget ui config error, stat = 0x%08llX", DjiStat);
        return DjiStat;
    }
    printf("%d\r\n", __LINE__);

    //Step 3 : Set widget handler list
    DjiStat = DjiWidget_RegHandlerList(s_widgetHandlerList, s_widgetHandlerListCount);
    if (DjiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_ERROR,"Set widget handler list error, stat = 0x%08llX", DjiStat);
        return DjiStat;
    }

    //Step 4 : Run widget api sample task
    if (Osal_TaskCreate("user_widget_task", &s_widgetTestThread,WIDGET_TASK_STACK_SIZE, DjiTest_WidgetTask,
                            NULL) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_ERROR,"Dji widget test task create error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }
	
	DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO, ">>>Widget init end");

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode DjiTest_WidgetSetConfigFilePath(const char *path)
{
    memset(s_widgetFileDirPath, 0, sizeof(s_widgetFileDirPath));
    memcpy(s_widgetFileDirPath, path, USER_UTIL_MIN(strlen(path), sizeof(s_widgetFileDirPath) - 1));

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
void setWidgetIndexByValue(int widgetIndex, const char* infoFile, size_t arraySize, int *array, bool isHex) {
    int value = readfl(infoFile, isHex);

    if (value == -1) {
        return;
    }

    if (value == INT_FAST32_MAX) {
        //PsdkLogger_UserLogInfo("Set s_widgetValueList[%d], bad value in file %s", widgetIndex, infoFile);
        return;
    }

    int32_t index = findIndex(array, arraySize, value);
    if (index == SIZE_MAX) {
        //PsdkLogger_UserLogInfo("Set s_widgetValueList[%d], bad value in file %s", widgetIndex, infoFile);
        return;
    }

    if (index == s_widgetValueList[widgetIndex]) {
        return;
    }

    if ((index >= 0) || (index <= arraySize)) {
        //PsdkLogger_UserLogInfo("Set s_widgetValueList[%d], old index %d, new index %d", widgetIndex, s_widgetValueList[widgetIndex], index);
        s_widgetValueList[widgetIndex]=index;
    } else {
        s_widgetValueList[widgetIndex] = 0;
        USER_LOG_INFO("Set s_widgetValueList[%d] value error. There is not index with value %d.", widgetIndex, value);
    }
}

void setWidgetValue(int widgetIndex, const char* infoFile, bool isHex) {
    int value = readfl(infoFile, isHex);

    if (value == INT_FAST32_MAX) {
        return;
    }

    if ((value <= 100) &&
        (value >= 0)) {
        //PsdkLogger_UserLogInfo("Set s_widgetValueList[%d] to %d, old value %d", widgetIndex, value, s_widgetValueList[widgetIndex]);
        s_widgetValueList[widgetIndex]=value;
    } else {
        s_widgetValueList[widgetIndex]=0;
        //PsdkLogger_UserLogInfo("Set s_widgetValueList[%d] value error. There is not value %d.", widgetIndex, value);
    }
}


/* Private functions definition-----------------------------------------------*/
static void *DjiTest_WidgetTask(void *arg)
{
    char message[DJI_WIDGET_FLOATING_WINDOW_MSG_MAX_LEN];
    uint32_t sysTimeMs = 0, startTimeMs = 0;
    const uint32_t maxTimeMsToFindInfoFile = 20000;
    T_DjiReturnCode DjiStat;
    int len = 0;
    USER_UTIL_UNUSED(arg);
    char str[DJI_WIDGET_FLOATING_WINDOW_MSG_MAX_LEN]={};
    T_DjiDataChannelState state;
    Osal_GetTimeMs(&startTimeMs);

    while (1) {
        setWidgetIndexByValue(0, setShutterFile, sizeof(shutter_dec) / sizeof(int), shutter_dec, true);
        setWidgetIndexByValue(1, setISOFile, sizeof(iso_dec) / sizeof(int), iso_dec, true);

        setWidgetIndexByValue(2, setFnumberFile, sizeof(number_dec) / sizeof(int), number_dec, true);
        setWidgetIndexByValue(3, setWhitebalanceFile, sizeof(wb_dec) / sizeof(int), wb_dec, true);
        setWidgetIndexByValue(4, setBiasFile, sizeof(bias_dec) / sizeof(int), bias_dec, true);

         //   DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO"read vals %i %i %i %i",s_widgetValueList[0],s_widgetValueList[1],s_widgetValueList[2],s_widgetValueList[3]);
        Osal_TaskSleepMs(900);
    }
}


static T_DjiReturnCode DjiTestWidget_SetWidgetValue(E_DjiWidgetType widgetType, uint32_t index, int32_t value,
                                                      void *userData)
{
    USER_UTIL_UNUSED(userData);

    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO, "Set widget value, widgetType = %s, widgetIndex = %d ,widgetValue = %d", s_widgetTypeNameArray[widgetType], index, value);

    int delta = s_widgetValueList[index];

    if ((index==0) && (s_widgetValueList[index] != value)) {
        writefl(infoShutterFile, -(s_widgetValueList[index] - value), false, true, shutter_dec[s_widgetValueList[index]]);
    }
    if ((index==1) && (s_widgetValueList[index] != value)) {
        writefl(infoIsoFile, -(s_widgetValueList[index] - value), false, true, iso_dec[s_widgetValueList[index]]);
    }

    if ((index==2) && (s_widgetValueList[index] != value)) {
        writefl(infoFnumberFile, -(s_widgetValueList[index] - value), false, true, number_dec[s_widgetValueList[index]]);
        //writefl(INFOFNUMBER, delta, true);            // check mode
    }
    if ((index==3) && (s_widgetValueList[index] != value)) {
        writefl(infoWhitebalanceFile, wb_dec[value], true, false, 0);
        //writefl(INFOWHITEBALANCE,wb_dec[value], true);
    }
    if ((index==4) && (s_widgetValueList[index] != value)) {
        writefl(infoBiasFile, -(s_widgetValueList[index] - value), false, true, bias_dec[s_widgetValueList[index]]);
        //writefl(INFOBIAS,delta, true);                // check mode
    }

    s_widgetValueList[index] = value;

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode DjiTestWidget_GetWidgetValue(E_DjiWidgetType widgetType, uint32_t index, int32_t *value,
                                                      void *userData)
{
    USER_UTIL_UNUSED(userData);
    USER_UTIL_UNUSED(widgetType);

    *value = s_widgetValueList[index];
 
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
