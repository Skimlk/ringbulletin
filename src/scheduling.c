
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "scheduling.h"
#include "filesystem.h"

int alreadyRunning(char *processName) {
    int alreadyRunning = 0;

    char *pidofCheck = NULL;
	asprintf(&pidofCheck, "pidof -x %s > /dev/null 2>&1", processName);
	if(system(pidofCheck) == 0) {
		printf("%s is already running\n", processName);
        alreadyRunning = 1;
	}

    free(pidofCheck);
    return alreadyRunning;
}

int removeCronTab() {
    if(hasWriteAccess(CRON_FILE_PATH))
        return removeFile(NULL, CRON_FILE_PATH);

    return 1;
}

int createCronTab(char *binaryPath, int jobsPerHour) {
    int ret = 0;
    char *cronMinuteField = NULL;
    char *crontab = NULL;
    char *loginUser = NULL;
    char *pwd = NULL;

    if(jobsPerHour < 1 || jobsPerHour > 60 || !hasWriteAccess(CRON_FILE_PATH))
        return 1;

    int jobHourSliceSize = ceil(60 / jobsPerHour);
    if(jobHourSliceSize == 1)
        cronMinuteField = strdup("0");
    else if(jobHourSliceSize == 60)
        cronMinuteField = strdup("*");
    else
        asprintf(&cronMinuteField, "*/%d", jobHourSliceSize);

    loginUser = getlogin();
    free(getcwd(pwd, 0));
    asprintf(&crontab, "%s * * * * %s cd %s && %s", cronMinuteField, loginUser, pwd, binaryPath);
    
    if(writeFile(crontab, NULL, NULL, CRON_FILE_PATH) == 1) {
        ret = 1;
        goto cleanup;
    }

cleanup:
    free(pwd);
    free(crontab);
    free(cronMinuteField);
    return ret;
}