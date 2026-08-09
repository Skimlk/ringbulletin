#define __USE_XOPEN
#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "timeutils.h"

time_t extractTimeFromFilename(char *filename) {
	char timeString[13];

	int i = 0;
	for(;filename[i] != '_'; i++) {
		timeString[i] = filename[i];
	}

	timeString[i] = '\0';

	    return (time_t)strtoll(timeString, NULL, 10);
}

char *createTimestampedFilename(char *filename, char *seperator) {
	time_t currentTime = time(NULL);
	int timestampLength = 10;
	int sizeOfTimestampedFilename 
		= sizeof(char) * (strlen(filename) + strlen(seperator) + timestampLength + 1);	

	char *timestampedFilename = malloc(sizeOfTimestampedFilename);
	
	snprintf(timestampedFilename, sizeOfTimestampedFilename, 
		"%s%s%ld", filename, seperator, currentTime);

	return timestampedFilename;
}

time_t getUnixTimestampFromTimeFormatString(char *timeFormatString) {
	    struct tm timeStructHelper = {0};

	    char *timeFormats[] = {
	        "%a, %d %b %Y %H:%M:%S %z",
	        "%a, %d %b %Y %H:%M:%S GMT",
	    };

	    for(size_t i = 0; i < sizeof(timeFormats) / sizeof(char *); i++) {
	        if(strptime(timeFormatString, timeFormats[i], &timeStructHelper) != NULL)
				return mktime(&timeStructHelper);
	    }

	    fprintf(stderr, "Failed to parse date-time.\n");
	    return 1;
}

void getFormattedTimeStrForPost(time_t time, char *buffer, size_t size) {
	struct tm *localTime = localtime(&time);
	const char *weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	snprintf(buffer, size, "%02d/%02d/%02d(%s)",
		localTime->tm_mon + 1,          // month 1-12
		localTime->tm_mday,             // day
		localTime->tm_year % 100,       // year (last 2 digits)
		weekdays[localTime->tm_wday]	// weekday
	);
}
