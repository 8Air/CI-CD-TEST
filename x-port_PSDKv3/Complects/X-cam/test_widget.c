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
    {2, PSDK_WIDGET_TYPE_LIST,          PsdkTestWidget_SetWidgetValue, PsdkTestWidget_GetWidgetValue, NULL},
    {3, PSDK_WIDGET_TYPE_LIST,          PsdkTestWidget_SetWidgetValue, PsdkTestWidget_GetWidgetValue, NULL},
    {4, PSDK_WIDGET_TYPE_LIST,          PsdkTestWidget_SetWidgetValue, PsdkTestWidget_GetWidgetValue, NULL},
    {5, PSDK_WIDGET_TYPE_LIST,          PsdkTestWidget_SetWidgetValue, PsdkTestWidget_GetWidgetValue, NULL},
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
static int32_t s_widgetValueList[5] = {0};

int pmdec[]={0,1,2,3,12};

#define INFOTMPFILE "/xport/camdata/tmp.nfo"
#define INFOISOFILE "/xport/camdata/iso.nfo"
#define INFOAPERTUREFILE "/xport/camdata/aperture.nfo"
#define INFOSHUTTERFILE "/xport/camdata/shutter.nfo"
#define INFOPMFILE "/xport/camdata/pm.nfo"
#define SETISOFILE "/xport/camdata/iso.set"
#define SETAPERTUREFILE "/xport/camdata/aperture.set"
#define SETSHUTTERFILE "/xport/camdata/shutter.set"
#define SETPMFILE "/xport/camdata/pm.set"
#define LISTISOFILE "/xport/camdata/iso.list"
#define LISTAPERTUREFILE "/xport/camdata/aperture.list"
#define LISTSHUTTERFILE "/xport/camdata/shutter.list"
#define LISTPMFILE "/xport/camdata/pm.list"
#define INFOSTATUS "/xport/camdata/status.txt"


int readfl (char* fn)
{
   int i=1000;
   FILE *inf = NULL;
   inf = fopen(fn, "r");
   if (!inf) return 1000;
   fscanf(inf,"%i",&i);
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
    unlink(fn);
    *lenght = len;

    return true;
}

void writefl (char* fn, int val)
{
   PsdkLogger_UserLogInfo("WFILE: %s, %i", fn, val);
   FILE *inf = NULL;
   inf = fopen(fn, "w");
   if (!inf) return;
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
        if (!readwholefile(INFOSTATUS, str, &len)) {
            PsdkOsal_GetTimeMs(&sysTimeMs);
            if ((sysTimeMs - startTimeMs) > maxTimeMsToFindInfoFile)
                PsdkLogger_UserLogInfo("%s", "Info file not found");
            else
                PsdkLogger_UserLogInfo("%s", "Camera loading...");
            len = sprintf(str, "Camera loading... \n");
        } else {
            PsdkOsal_GetTimeMs(&startTimeMs);
        }

        if(s_widgetValueList[4] == 0)
            sprintf(str + len, "Single shoot mode");
        else 
            sprintf(str + len, "Interval mode: %.1fs", interval_values[s_widgetValueList[4] - 1]);
    
        PsdkWidgetFloatingWindow_ShowMessage(str);
 //       PsdkWidgetFloatingWindow_GetChannelState(&state);

 //       PsdkLogger_UserLogInfo("INFO: limit %u, before %u, after %u. busy %i", state.realtimeBandwidthLimit, state.realtimeBandwidthBeforeFlowController, state.realtimeBandwidthAfterFlowController, state.busyState);

        s_widgetValueList[0]=readfl(INFOISOFILE);
        if (s_widgetValueList[0]>35) s_widgetValueList[0]=0;

        s_widgetValueList[1]=readfl(INFOAPERTUREFILE);
        if (s_widgetValueList[1]>19) s_widgetValueList[1]=19;
        
        s_widgetValueList[2]=readfl(INFOSHUTTERFILE);
	    if (s_widgetValueList[2]>55) s_widgetValueList[2]=55;
        
        s_widgetValueList[3]=readfl(INFOPMFILE);
        if (s_widgetValueList[3]>4) s_widgetValueList[3]=4;

         //   PsdkLogger_UserLogInfo("read vals %i %i %i %i",s_widgetValueList[0],s_widgetValueList[1],s_widgetValueList[2],s_widgetValueList[3]);
        writefl(SETINTERVALFILE,s_widgetValueList[4]);
        PsdkOsal_TaskSleepMs(900);
    }
}


static T_PsdkReturnCode PsdkTestWidget_SetWidgetValue(E_PsdkWidgetType widgetType, uint32_t index, int32_t value,
                                                      void *userData)
{
    USER_UTIL_UNUSED(userData);

    PsdkLogger_UserLogInfo("Set widget value, widgetType = %s, widgetIndex = %d ,widgetValue = %d", s_widgetTypeNameArray[widgetType], index, value);

    s_widgetValueList[index] = value;

   if (index==0) writefl(SETISOFILE,s_widgetValueList[index]);
   if (index==1) writefl(SETAPERTUREFILE,s_widgetValueList[index]);
   if (index==2) writefl(SETSHUTTERFILE,s_widgetValueList[index]);
   if (index==3) writefl(SETPMFILE,pmdec[s_widgetValueList[index]]);
   if (index==4) writefl(SETINTERVALFILE,s_widgetValueList[index]);
 

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
