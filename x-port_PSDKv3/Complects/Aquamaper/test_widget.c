#include "common.h"
T_PsdkReturnCode PsdkTest_WidgetInit(void);
T_PsdkReturnCode PsdkTest_WidgetSetConfigFilePath(const char *path);

#define WIDGET_DIR_PATH_LEN_MAX         (256)
#define WIDGET_TASK_STACK_SIZE          (2048)


static void *PsdkTest_WidgetTask(void *arg);
static T_PsdkReturnCode PsdkTestWidget_SetWidgetValue(E_PsdkWidgetType widgetType, uint32_t index, int32_t value,
                                                      void *userData);
static T_PsdkReturnCode PsdkTestWidget_GetWidgetValue(E_PsdkWidgetType widgetType, uint32_t index, int32_t *value,
                                                      void *userData);

static T_PsdkTaskHandle s_widgetTestThread;
static char s_widgetFileDirPath[PSDK_FILE_PATH_SIZE_MAX] = {0};


static const T_PsdkWidgetHandlerListItem s_widgetHandlerList[] = {
    {0, PSDK_WIDGET_TYPE_LIST,          PsdkTestWidget_SetWidgetValue, PsdkTestWidget_GetWidgetValue, NULL},
    {1, PSDK_WIDGET_TYPE_LIST,          PsdkTestWidget_SetWidgetValue, PsdkTestWidget_GetWidgetValue, NULL},
    {2, PSDK_WIDGET_TYPE_SCALE,          PsdkTestWidget_SetWidgetValue, PsdkTestWidget_GetWidgetValue, NULL},
    {3, PSDK_WIDGET_TYPE_LIST,          PsdkTestWidget_SetWidgetValue, PsdkTestWidget_GetWidgetValue, NULL},
};

static char *s_widgetTypeNameArray[] = {
    "Unknown",
    "Button",
    "Switch",
    "Scale",
    "List",
    "Int input box"
};

static const uint32_t s_widgetHandlerListCount = sizeof(s_widgetHandlerList) / sizeof(T_PsdkWidgetHandlerListItem);
static int32_t s_widgetValueList[sizeof(s_widgetHandlerList) / sizeof(T_PsdkWidgetHandlerListItem)] = {2, 2, 2, 2};

int pmdec[]={0,1,2,3,12};

int range_dec[]={10,20,30,40,50, 60, 70, 80, 90, 100};
int interval_dec[]={10,5, 4, 2};
int gain_dec[]={-30, -25, -20, -15, -10, -5,
                0, 5, 10, 15, 20, 25, 30};

#define INFOSTATUS "/xport/camdata/status.txt"

#define INFORANGE "/xport/camdata/range.txt"
#define INFOINTERVAL "/xport/camdata/interval.txt"
#define INFOTHRESHOLD "/xport/camdata/threshold.txt"
#define INFOGAIN "/xport/camdata/gain.txt"

#define SETRANGE "/xport/camdata/range.set"
#define SETINTERVAL "/xport/camdata/interval.set"
#define SETTHRESHOLD "/xport/camdata/threshold.set"
#define SETGAIN "/xport/camdata/gain.set"

int32_t findIndex(int *array, size_t size, int value) {

    PsdkLogger_UserLogInfo("findIndex value %d size %d", value, size);
    for (int i = 0; i < size; i++) {
        if (!array) {
            return SIZE_MAX;
        }
        if (*array == value) {
            return i;
        }
        array++;
    }
    return SIZE_MAX;
}

int readfl (char* fn)
{
   int i=INT_FAST32_MAX;
   FILE *inf = NULL;
   inf = fopen(fn, "r");
   if (!inf) {
       if (!access(fn, F_OK)) {
           PsdkLogger_UserLogInfo("readfl error to open %s", fn);
       }
       return i;
   }
   fscanf(inf,"%i",&i);
   PsdkLogger_UserLogInfo("readfl from %s value %d", fn, i);
   fclose(inf);
   unlink(fn);
   return i;
}

bool readflstr (char* fn, char* str)
{
   FILE *inf = NULL;
   inf = fopen(fn, "r");
   if (!inf) return false;
   fgets(str,900,inf);
   fclose(inf);
   unlink(fn);
   return true;
}

bool readwholefile (char* fn, char* str, int* lenght)
{
    FILE *inf = NULL;
    inf = fopen(fn, "rb");
    if (!inf) return false;
    long len = 0;

    fseek (inf, 0, SEEK_END);
    len = ftell (inf);
    if(len > PSDK_WIDGET_FLOATING_WINDOW_MSG_MAX_LEN)
        len = PSDK_WIDGET_FLOATING_WINDOW_MSG_MAX_LEN;
    fseek (inf, 0, SEEK_SET);
    fread (str, 1, len, inf);
    fclose(inf);
 //   unlink(fn);
    *lenght = len;

    return true;
}

void writefl (char* fn, int val)
{
   PsdkLogger_UserLogInfo("WFILE: %s, %i", fn, val);
   FILE *inf = NULL;
   inf = fopen(fn, "w");
   if (!inf) return;
   PsdkLogger_UserLogInfo("writefl to %s value %d", fn, val);
   fprintf(inf,"%i\n",val);
   fclose(inf);
}

/* Exported functions definition ---------------------------------------------*/
T_PsdkReturnCode PsdkTest_WidgetInit(void)
{
    T_PsdkReturnCode psdkStat;

    //Step 1 : Init PSDK Widget
    psdkStat = PsdkWidget_Init();
    if (psdkStat != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        PsdkLogger_UserLogError("Init widget error, stat = 0x%08llX", psdkStat);
        return psdkStat;
    }

    //Step 2 : Set UI Config (Linux environment)

    char curFileDirPath[WIDGET_DIR_PATH_LEN_MAX];
    char tempPath[WIDGET_DIR_PATH_LEN_MAX];

    psdkStat = PsdkUserUtil_GetCurrentFileDirPath(__FILE__, WIDGET_DIR_PATH_LEN_MAX, curFileDirPath);

    snprintf(tempPath, WIDGET_DIR_PATH_LEN_MAX, "/xport/apptest-camera/widget");

    //set default ui config path
    psdkStat = PsdkWidget_RegDefaultUiConfigByDirPath(tempPath);
    if (psdkStat != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        PsdkLogger_UserLogError("Add default widget ui config error, stat = 0x%08llX", psdkStat);
        return psdkStat;
    }

    //Step 3 : Set widget handler list
    psdkStat = PsdkWidget_RegHandlerList(s_widgetHandlerList, s_widgetHandlerListCount);
    if (psdkStat != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        PsdkLogger_UserLogError("Set widget handler list error, stat = 0x%08llX", psdkStat);
        return psdkStat;
    }

    //Step 4 : Run widget api sample task
    if (PsdkOsal_TaskCreate(&s_widgetTestThread, PsdkTest_WidgetTask, "user_widget_task", WIDGET_TASK_STACK_SIZE,
                            NULL) != PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        PsdkLogger_UserLogError("Psdk widget test task create error.");
        return PSDK_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    PsdkLogger_UserLogInfo(">>>Widget init end");

    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_PsdkReturnCode PsdkTest_WidgetSetConfigFilePath(const char *path)
{
    memset(s_widgetFileDirPath, 0, sizeof(s_widgetFileDirPath));
    memcpy(s_widgetFileDirPath, path, USER_UTIL_MIN(strlen(path), sizeof(s_widgetFileDirPath) - 1));

    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

void setWidgetIndexByValue(int widgetIndex, const char* infoFile, size_t arraySize, int *array) {
    int value = readfl(infoFile);
    if (value == INT_FAST32_MAX) {
	    PsdkLogger_UserLogInfo("Set s_widgetValueList[%d], bad value in file %s", widgetIndex, infoFile);
        return;
    }

    int32_t index = findIndex(array, arraySize, value);

    if ((s_widgetValueList[widgetIndex] == index) || (index == SIZE_MAX)) {
	    PsdkLogger_UserLogInfo("Set s_widgetValueList[%d], bad index by value %d", widgetIndex, value);
        return;
    }

    if ((index >= 0) || (index <= arraySize)) {
        PsdkLogger_UserLogInfo("Set s_widgetValueList[%d], old index %d, new index %d", widgetIndex, s_widgetValueList[widgetIndex], index);
        s_widgetValueList[widgetIndex]=index;
    } else {
        s_widgetValueList[widgetIndex] = 0;
        PsdkLogger_UserLogInfo("Set s_widgetValueList[%d] value error. There is not index with value %d.", widgetIndex, value);
    }
}

void setWidgetValue(int widgetIndex, const char* infoFile) {
    int value = readfl(infoFile);

    if (value == INT_FAST32_MAX) {
        return;
    }

    if ((value <= 100) &&
        (value >= 0)) {
        PsdkLogger_UserLogInfo("Set s_widgetValueList[%d] to %d, old value %d", widgetIndex, value, s_widgetValueList[widgetIndex]);
        s_widgetValueList[widgetIndex]=value;
    } else {
        s_widgetValueList[widgetIndex]=0;
        PsdkLogger_UserLogInfo("Set s_widgetValueList[%d] value error. There is not value %d.", widgetIndex, value);
    }
}

/* Private functions definition-----------------------------------------------*/
static void *PsdkTest_WidgetTask(void *arg)
{
    char message[PSDK_WIDGET_FLOATING_WINDOW_MSG_MAX_LEN];
    uint32_t sysTimeMs = 0, startTimeMs = 0;
    const uint32_t maxTimeMsToFindInfoFile = 20000;
    T_PsdkReturnCode psdkStat;
    int len = 0;
    USER_UTIL_UNUSED(arg);
    char str[PSDK_WIDGET_FLOATING_WINDOW_MSG_MAX_LEN]={};
    T_PsdkDataChannelState state;
    PsdkOsal_GetTimeMs(&startTimeMs);

    while (1) {
        setWidgetIndexByValue(0, INFORANGE, sizeof(range_dec) / sizeof(int), range_dec);
        setWidgetIndexByValue(1, INFOINTERVAL, sizeof(interval_dec) / sizeof(int), interval_dec);
        setWidgetValue(2, INFOTHRESHOLD);
        setWidgetIndexByValue(3, INFOGAIN, sizeof(gain_dec) / sizeof(int), gain_dec);

        int timer = readfl(TIMERFILE);
        if (timer != INT32_MAX) {
            PsdkLogger_UserLogInfo("%s %d", ">>> currentVideoRecordingTimeInSeconds ",
                                   s_cameraState.currentVideoRecordingTimeInSeconds);
            s_cameraState.currentVideoRecordingTimeInSeconds = timer;
        } else {
            if (s_cameraState.isRecording) {
                PsdkLogger_UserLogInfo("%s %s %d", ">>> error while read", TIMERFILE,
                                       s_cameraState.currentVideoRecordingTimeInSeconds);
                s_cameraState.currentVideoRecordingTimeInSeconds = 0;
            }
        }

        PsdkOsal_TaskSleepMs(900);
    }
}


static T_PsdkReturnCode PsdkTestWidget_SetWidgetValue(E_PsdkWidgetType widgetType, uint32_t index, int32_t value,
                                                      void *userData)
{
    USER_UTIL_UNUSED(userData);

    PsdkLogger_UserLogInfo("Set widget value, widgetType = %s, widgetIndex = %d ,widgetValue = %d, old value = %d",
                           s_widgetTypeNameArray[widgetType], index, value, s_widgetValueList[index]);


    if ((index==0) && (s_widgetValueList[index] != value)) {
        writefl(SETRANGE,range_dec[value]);
    }
    if ((index==1) && (s_widgetValueList[index] != value)) {
        writefl(SETINTERVAL,interval_dec[value]);
    }
    if ((index==2) && (s_widgetValueList[index] != value)) {
        writefl(SETTHRESHOLD,value);
    }
    if ((index==3) && (s_widgetValueList[index] != value)) {
        writefl(SETGAIN, gain_dec[value]);
    }

    s_widgetValueList[index] = value;

    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_PsdkReturnCode PsdkTestWidget_GetWidgetValue(E_PsdkWidgetType widgetType, uint32_t index, int32_t *value,
                                                      void *userData)
{
    USER_UTIL_UNUSED(userData);
    USER_UTIL_UNUSED(widgetType);

    *value = s_widgetValueList[index];

    return PSDK_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
