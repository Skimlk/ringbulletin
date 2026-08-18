#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "stringutils.h"

bool isNumber(char *string) {
	if(string == NULL || string[0] == '\0')
		return false;

	for(char *ch = string; *ch != '\0'; ch++) {
		if(!isdigit((unsigned char)*ch))
			return false;
	}

	return true;
}

int stringToInt(char *string, int *number) {
	if(!isNumber(string))
		return 1;

	*number = atoi(string);

	return 0;
}

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
