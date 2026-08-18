#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <stdbool.h>

extern bool isNumber(char *string);
extern int stringToInt(char *string, int *number);
extern char *strlwr(char *string);
extern char *removeReplyPrefix(char *string);
extern char *strip(char *string);
extern char *normalize(char *string);

#endif
