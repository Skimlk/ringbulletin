#ifndef CONTEXT_H
#define CONTEXT_H

#include <cjson/cJSON.h>
#include <time.h>

#include "config.h"

typedef struct Context {
	ConfigValues *config;
	time_t searchStartTime;
	char *postsDirectoryPath;
	char *viewsDirectoryPath;
	char *iconsDirectoryPath;
	char *cssDirectoryPath;
	char *boardJsonUrl;
	char *boardHtmlUrl;
} Context;

#endif