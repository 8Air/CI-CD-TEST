/**
 ********************************************************************
 * @file    test_data_subscription.c
 * @version V2.0.0
 * @date    2019/8/15
 * @brief
 *
 * @copyright (c) 2017-2018 DJI. All rights reserved.
 *
 * All information contained herein is, and remains, the property of DJI.
 * The intellectual and technical concepts contained herein are proprietary
 * to DJI and may be covered by U.S. and foreign patents, patents in process,
 * and protected by trade secret or copyright law.  Dissemination of this
 * information, including but not limited to data and other proprietary
 * material(s) incorporated within the information, in any form, is strictly
 * prohibited without the express written consent of DJI.
 *
 * If you receive this source code without DJI’s authorization, you may not
 * further disseminate the information, and you must immediately remove the
 * source code and notify DJI of its removal. DJI reserves the right to pursue
 * legal actions against you for any loss(es) or damage(s) caused by your
 * failure to do so.
 *
 *********************************************************************
 */

#include <sys/time.h>
#include "dji_logger.h"
#include "dji_fc_subscription.h"

#define DATA_SUBSCRIPTION_TASK_FREQ         (100)
#define DATA_SUBSCRIPTION_TASK_STACK_SIZE   (2048)

static void *UserDataSubscription_Task(void *arg);

/* Private variables ---------------------------------------------------------*/
static T_DjiTaskHandle s_userDataSubscriptionThread;

/* Exported functions definition ---------------------------------------------*/
T_DjiReturnCode DjiTest_DataSubscriptionInit(void)
{
    T_DjiReturnCode djiStat;

    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>>Subscr init start");

    djiStat = DjiFcSubscription_Init();
    if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_ERROR,"init data subscription module error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    djiStat = DjiFcSubscription_SubscribeTopic(DJI_FC_SUBSCRIPTION_TOPIC_QUATERNION, 100,NULL);
    djiStat = DjiFcSubscription_SubscribeTopic(DJI_FC_SUBSCRIPTION_TOPIC_GPS_POSITION, 5, NULL);
    djiStat = DjiFcSubscription_SubscribeTopic(DJI_FC_SUBSCRIPTION_TOPIC_GPS_DATE, 1, NULL);
    djiStat = DjiFcSubscription_SubscribeTopic(DJI_FC_SUBSCRIPTION_TOPIC_GPS_TIME, 1, NULL);


    if (Osal_TaskCreate("user_subscription_task",UserDataSubscription_Task,
        DATA_SUBSCRIPTION_TASK_STACK_SIZE, NULL, &s_userDataSubscriptionThread) !=DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS)
    {
        DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_ERROR,"user data subscription task create error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>>Subscr init end");

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

/* Private functions definition-----------------------------------------------*/

static void *UserDataSubscription_Task(void *arg)
{
    T_DjiReturnCode djiStat;
    T_DjiFcSubscriptionQuaternion quaternion = {0};
	T_DjiDataTimestamp timestamp = {0};
    T_DjiFcSubscriptionGpsPosition gpsPosition = {0};
    T_DjiFcSubscriptionGpsDate gpsDate = {0};
    T_DjiFcSubscriptionGpsTime gpsTime = {0};
    uint32_t currTimeMS = {0};
    struct timeval time;
    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>>Subscr task");

    USER_UTIL_UNUSED(arg);

    while (1) {
        Osal_TaskSleepMs(1000 / DATA_SUBSCRIPTION_TASK_FREQ);

        djiStat = DjiFcSubscription_GetLatestValueOfTopic(DJI_FC_SUBSCRIPTION_TOPIC_QUATERNION, (uint8_t *) &quaternion, sizeof(T_DjiFcSubscriptionQuaternion), &timestamp);
        ts_q_us=timestamp.microsecond;
	ts_q_ms=timestamp.millisecond;
        ts_q_0=quaternion.q0;
        ts_q_1=quaternion.q1; 
        ts_q_2=quaternion.q2; 
        ts_q_3=quaternion.q3;

        djiStat = DjiFcSubscription_GetLatestValueOfTopic(DJI_FC_SUBSCRIPTION_TOPIC_GPS_POSITION, (uint8_t *) &gpsPosition, sizeof(T_DjiFcSubscriptionGpsPosition),&timestamp);
        ts_c_us=timestamp.microsecond;
        ts_c_ms=timestamp.millisecond;
        ts_c_x=gpsPosition.x;
        ts_c_y=gpsPosition.y;
        ts_c_z=gpsPosition.z;

        djiStat = DjiFcSubscription_GetLatestValueOfTopic(DJI_FC_SUBSCRIPTION_TOPIC_GPS_DATE, (uint8_t *) &gpsDate, sizeof(T_DjiFcSubscriptionGpsDate), &timestamp);
        ts_d_us=timestamp.microsecond;
        ts_d_ms=timestamp.millisecond;
        ts_d_d=gpsDate;

        djiStat = DjiFcSubscription_GetLatestValueOfTopic(DJI_FC_SUBSCRIPTION_TOPIC_GPS_TIME, (uint8_t *) &gpsTime, sizeof(T_DjiFcSubscriptionGpsTime), &timestamp);
        ts_t_us=timestamp.microsecond;
        ts_t_ms=timestamp.millisecond;
        ts_t_t=gpsTime;

        
        Osal_GetTimeMs(&currTimeMS);

        // 1st Jan 2022
        if (timestamp.millisecond > 1641048993 && abs(currTimeMS - timestamp.millisecond) > 1000) {
            DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,">>>Sync time with drone GPS timestamp");
            time.tv_sec = timestamp.millisecond / 1000;
            time.tv_usec = (timestamp.millisecond % 1000) * 1000;
            settimeofday(&time, NULL);
        }
    }
}

