#include <ctype.h>

char *strlwr(char *string) {
	char *character = string;
	while(*character++ != '\0') {
		*character = tolower(*character);
	}

	return string;
}

char *strip(char *string) {
	return string;
}
