#define __USE_XOPEN
#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>

char *strlwr(char *string) {
	char *character = string;
	while(*character != '\0') {
		*character = tolower(*character);
		character++;
	}

	return string;
}

char *removeReplyPrefix(char *string) {
	char *replyPrefix = "re:";
	int replyPrefixLength = strlen(replyPrefix);

	char *replyPrefixLocation = strstr(string, replyPrefix);
	if (!replyPrefixLocation) { 
		return string;
	}

	int indexOfOriginalTitle = (replyPrefixLocation - string) + replyPrefixLength;

	size_t stringLength = strlen(string);
	size_t i;
	for(i = indexOfOriginalTitle; i < stringLength; i++) {
		string[i - indexOfOriginalTitle] = string[i];
	}

	string[i - indexOfOriginalTitle] = '\0';

	return string;	
}

char *strip(char *string) {
	int write = 0;
	for(int read = 0; string[read] != '\0'; read++) {
		char c = string[read];
		if((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
			string[write++] = c;
	}

	string[write] = '\0';

	return string;
}

char *normalize(char *string) {
	return strip(
		removeReplyPrefix(
			strlwr(string)
		)
	);
}

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

    for(long unsigned int i = 0; i < sizeof(timeFormats) / sizeof(char *); i++) {
        if(strptime(timeFormatString, timeFormats[i], &timeStructHelper) != NULL)
            return mktime(&timeStructHelper); 
    }

    fprintf(stderr, "Failed to parse date-time.\n");
    return 1;
}