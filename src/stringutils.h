#ifndef STRINGUTILS_H
#define STRINGUTILS_H

extern char *strlwr(char *string);
extern char *removeReplyPrefix(char *string);
extern char *strip(char *string);
extern char *normalize(char *string);
extern char *getDomainFromLink(const char *link);
extern int normalizeUrl(char **urlPtr);
extern time_t extractTimeFromFilename(char *filename);
extern char *createTimestampedFilename(char *filename, char *seperator);
extern time_t getUnixTimestampFromTimeFormatString(char *timeFormatString);
extern void getFormattedTimeStrForPost(time_t time, char *buffer, size_t size);

#endif
