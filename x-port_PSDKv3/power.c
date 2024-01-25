static T_DjiReturnCode DjiTest_PowerOffNotificationCallback(bool *powerOffPreparationFlag)
{
    system("reboot");
    *powerOffPreparationFlag = false;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}



T_DjiReturnCode DjiTest_PowerManagementInit(void)
{
    T_DjiReturnCode DjiStat;
	DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_DEBUG,"register power off notification callback function");
    DjiStat = DjiPowerManagement_RegPowerOffNotificationCallback(DjiTest_PowerOffNotificationCallback);
    if (DjiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
	    DjiLogger_UserLogOutput(DJI_LOGGER_CONSOLE_LOG_LEVEL_ERROR,"register power off notification callback function error");
        return DjiStat;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
