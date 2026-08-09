#ifndef TIMEUTILS_H
#define TIMEUTILS_H

#include <time.h>

extern time_t extractTimeFromFilename(char *filename);
extern char *createTimestampedFilename(char *filename, char *seperator);
extern time_t getUnixTimestampFromTimeFormatString(char *timeFormatString);
extern void getFormattedTimeStrForPost(time_t time, char *buffer, size_t size);

#endif
