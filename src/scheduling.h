#ifndef SCHEDULING_H
#define SCHEDULING_H

#define CRON_DIR_PATH "/etc/cron.d/"
#define CRON_FILE_PATH "/etc/cron.d/ringbulletin"

extern int alreadyRunning(char *processName);
extern int removeCronTab();
extern int createCronTab(char *binaryPath, int jobsPerHour);

#endif