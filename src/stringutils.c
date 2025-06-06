#include <string.h>
#include <ctype.h>

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
