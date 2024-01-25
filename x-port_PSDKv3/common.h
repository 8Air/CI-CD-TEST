#ifndef COMMON_DEFINITION_FILE_H
#define COMMON_DEFINITION_FILE_H

#include <dji_widget.h>
#include "psdk_lib_v3/include/dji_xport.h"
#include "psdk_lib_v3/include/dji_payload_camera.h"
#include "psdk_lib_v3/include/dji_fc_subscription.h"

char *licFilePath;              //"/xport/camdata/lic.txt"

char *setRangeFile;             //"/xport/camdata/range.set"
char *setIntervalFile;          //"/xport/camdata/interval.set"
char *setScaleFile;             //"/xport/camdata/scale.set"
char *setGainFile;              //"/xport/camdata/gain.set"
char *setBaudFile;              //"/xport/camdata/baud.set"
char *setISOFile;               //"/xport/camdata/iso.set"
char *setApertureFile;          //"/xport/camdata/aperture.set"
char *setShutterFile;           //"/xport/camdata/shutter.set"
char *setPMFile;                //"/xport/camdata/pm.set"
char *setFMFile;                //"/xport/camdata/fm.set"
//a6100
char *setExposureFile;          // "/xport/camdata/exposuremeteringmode.set"
char *setFocusFile;             // "/xport/camdata/focus.set"
char *setFnumberFile;           // "/xport/camdata/fnumber.set"
char *setWhitebalanceFile;      // "/xport/camdata/wb.set"
char *setBiasFile;              // "/xport/camdata/bias.set"
//~a6100

char *takeFile;                 //"/xport/camdata/take"
char *focusFile;                //"/xport/camdata/focus"

char *infoTmpFile;              //"/xport/camdata/tmp.nfo"
char *infoIsoFile;              //"/xport/camdata/iso.nfo"
char *infoApertureFile;         //"/xport/camdata/aperture.nfo"
char *infoShutterFile;          //"/xport/camdata/shutter.nfo"
char *infoPMFile;               //"/xport/camdata/pm.nfo"
char *infoFMFile;               //"/xport/camdata/fm.nfo"
//a6100
char *infoExposureFile;         // "/xport/camdata/exposuremeteringmode.nfo"
char *infoFocusFile;            // "/xport/camdata/focus.nfo"
char *infoFnumberFile;          // "/xport/camdata/fnumber.nfo"
char *infoWhitebalanceFile;     // "/xport/camdata/wb.nfo"
char *infoBiasFile;             // "/xport/camdata/bias.nfo"
//~a6100

char *listISOFile;              //"/xport/camdata/iso.list"
char *listApertureFile;         //"/xport/camdata/aperture.list"
char *listShutterFile;          //"/xport/camdata/shutter.list"
char *listPMFile;               //"/xport/camdata/pm.list"

char *photoFile;               //"/xport/camdata/cap.jpg"

char *apptestCameraPhotoFile;  //"/xport/apptest-camera/cam.jpg"
char *apptestCameraWidgetFile;  //"/xport/apptest-camera/widget"

//a6100
char *takePhoto;                //"/xport/camdata/take_photo"
char *doFocus;                  //"/xport/camdata/doFocus"
char *startVideoFile;           // "/xport/camdata/start_video"
//~a6100

char *infoStatusFile;           //"/xport/camdata/status.txt"

int32_t findIndex(int *array, size_t size, int value) {
    //PsdkLogger_UserLogInfo("findIndex value %d size %d", value, size);
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


int checkAcces(char* path) {
    int rval;
    /* Check exiting. */
    rval = access(path, F_OK);
    if (rval != 0) {
        if (errno == ENOENT)
            printf("%s does not exist ", path);
        else if (errno == EACCES)
            printf("%s is not accessible ", path);
        return 1;
    }

    /* Check acces. */
    rval = access(path, R_OK);
    if (rval != 0){
        printf("%s is not readable (access denied) ", path);
        return 2;
    }
    /* Check write-acces. */
    rval = access(path, W_OK);
    if (rval != 0) {
        switch (errno) {
            case EACCES: {
                printf("%s is not writable (access denied) ", path);
                return 3;
            }
            case EROFS: {
                printf("%s is not writable (read-only filesystem) ", path);
                return 4;
            }
            default: {
                printf("%s unexpected error ", path);
                return 5;
            }
        }
    }
    return 0;
}

int setPath(char **var, const char *envVar) {
    *var = getenv(envVar);
    if (!*var) {
        printf("%s is empty! Set %s in enviroment.\n", envVar, envVar);
        return 0;
    }
    checkAcces(*var);
    return 0;
}

int settersVars()
{
    int err = 0;
    err += setPath(&setRangeFile, "RANGE_FILE_PATH");
    err += setPath(&setIntervalFile, "INTERVAL_FILE_PATH");
    err += setPath(&setScaleFile, "SCALE_FILE_PATH");
    err += setPath(&setGainFile, "GAIN_FILE_PATH");
    err += setPath(&setBaudFile, "BAUD_FILE_PATH");
    err += setPath(&setISOFile, "SET_ISO_FILE_PATH");
    err += setPath(&setApertureFile, "SET_APERTURE_FILE_PATH");
    err += setPath(&setShutterFile, "SET_SHUTTER_FILE_PATH");
    err += setPath(&setPMFile, "SET_PM_FILE_PATH");
    err += setPath(&setFMFile, "SET_FM_FILE_PATH");
    return err;
}

int infoVars()
{
    int err = 0;
    err += setPath(&infoTmpFile, "INFO_TMP_FILE_PATH");
    err += setPath(&infoApertureFile, "INFO_APERTURE_PATH");
    err += setPath(&infoShutterFile, "INFO_SHUTTER_PATH");
    err += setPath(&infoPMFile, "INFO_PM_PATH");
    err += setPath(&infoFMFile, "INFO_FM_PATH");
    return err;
}

int listVars()
{
    int err = 0;
    err += setPath(&listISOFile, "LIST_ISO_FILE_PATH");
    err += setPath(&listApertureFile, "LIST_APERTURE_FILE_PATH");
    err += setPath(&listShutterFile, "LIST_SHUTTER_FILE_PATH");
    err += setPath(&listPMFile, "LIST_PM_FILE_PATH");
    return err;
}

int otherVars()
{
    int err = 0;
    err += setPath(&takeFile, "TAKE_FILE_PATH");
    err += setPath(&focusFile, "FOCUS_FILE_PATH");
    err += setPath(&photoFile, "PHOTO_FILE_PATH");

    err += setPath(&apptestCameraPhotoFile, "APPTEST_CAMERA_PHOTO_FILE_PATH");
    err += setPath(&apptestCameraWidgetFile, "APPTEST_CAMERA_WIDGET_FILE_PATH");

    err += setPath(&infoStatusFile, "INFO_STATUS_FILE_PATH");
    return err;
}

int initPath() {
    int err = 0;
    
    err += setPath(&licFilePath, "LIC_FILE_PATH");
    
    err += settersVars();
    err += infoVars();
    err += listVars();
    err += otherVars();

    return err;
}

int settersA6100Paths()
{
    int err = 0;
//to camera
    err += setPath(&infoIsoFile, "INFOISO");
    err += setPath(&infoExposureFile, "INFOEXPOSURE");
    err += setPath(&infoFocusFile, "INFOFOCUS");
    err += setPath(&infoFnumberFile, "INFOFNUMBER");
    err += setPath(&infoWhitebalanceFile, "INFOWHITEBALANCE");
    err += setPath(&infoBiasFile, "INFOBIAS");
    err += setPath(&startVideoFile, "STARTVIDEOFILE");

//from camera
    err += setPath(&setExposureFile, "SETEXPOSURE");
    err += setPath(&setFocusFile, "SETFOCUS");
    err += setPath(&setFnumberFile, "SETFNUMBER");
    err += setPath(&setWhitebalanceFile, "SETWHITEBALANCE");
    err += setPath(&setBiasFile, "SETBIAS");
    err += setPath(&photoFile, "PHOTO_FILE_PATH");
    err += setPath(&apptestCameraPhotoFile, "APPTEST_CAMERA_PHOTO_FILE_PATH");
    return 0;
}

int readfl (const char* fn, bool isHex)
{
    int i=UINT_FAST32_MAX;
    FILE *inf = NULL;
    inf = fopen(fn, "r");
    if (!inf) {
        if (!access(fn, F_OK)) {
            //PsdkLogger_UserLogInfo("readfl error to open %s", fn);
        }
        return i;
    }
    if (isHex) {
        char hexValue[11];  // Максимальная длина шестнадцатеричного значения в формате 0xFFFFFFFF
        fscanf(inf, "%10s", hexValue);  // Считываем значение в шестнадцатеричном формате
        i = (int)strtol(hexValue, NULL, 16); // Преобразование шестнадцатеричного значения в целое число
    } else {
        fscanf(inf,"%i",&i);
    }
    //PsdkLogger_UserLogInfo("readfl from %s value %d", fn, i);
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
    if(len > DJI_WIDGET_FLOATING_WINDOW_MSG_MAX_LEN)
        len = DJI_WIDGET_FLOATING_WINDOW_MSG_MAX_LEN;
    fseek (inf, 0, SEEK_SET);
    fread (str, 1, len, inf);
    fclose(inf);
    unlink(fn);
    *lenght = len;

    return true;
}

void writefl (char* fn, int val, bool isHex, bool writeOldValue, unsigned int oldValue)
{
    if (!fn) return;

    FILE *inf = NULL;
    inf = fopen(fn, "w");
    if (!inf) return;

    if(isHex) {
        fprintf(inf, "0x%X\n", val);
    }
    else {
        fprintf(inf, "%i\n", val);
    }

    if(writeOldValue) {
        fprintf(inf, "0x%X\n", oldValue);
    }
    fclose(inf);
}

#endif //COMMON_DEFINITION_FILE_H