#ifndef CRAWLER_H
#define CRAWLER_H

#include <cjson/cJSON.h>

#include "context.h"

typedef struct Memory {
	char *data;
	size_t size;
} Memory;

extern char *fetch(char *URL);
extern Memory *fetchBinary(char *URL);
extern Memory *fetchIcon(char *URL);
extern char *getDomainFromLink(const char *link);
extern char *getBaseUrlFromLink(const char *link);
extern int normalizeUrl(char **urlPtr);
extern int searchedAlready(Context *ctx, const char *categoryString, const char *itemString);
extern void searchBoard(cJSON *board, Context *ctx, int currentDepth);

#endif
