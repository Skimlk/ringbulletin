#ifndef CRAWLER_H
#define CRAWLER_H

#include <cjson/cJSON.h>

#include "context.h"

extern char *fetch(char *URL);
extern char *getDomainFromLink(const char *link);
extern int normalizeUrl(char **urlPtr);
extern int searchedAlready(Context *ctx, const char *categoryString, const char *itemString);
extern void searchBoard(cJSON *board, Context *ctx, int currentDepth);

#endif
