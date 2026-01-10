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

	if (strncmp(string, replyPrefix, replyPrefixLength) != 0) { 
		return string;
	}

	int stringLength = strlen(string);
	for(int i = replyPrefixLength; i <= stringLength; i++) {
		string[i - replyPrefixLength] = string[i];
	}

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

char *extractTimeFromFilename(char *filename) {
	char *time = malloc(sizeof(char) * 13);

	int i = 0;
	for(;filename[i] != '_'; i++) {
		time[i] = filename[i];
	}

	time[i] = '\0';

	return time;
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