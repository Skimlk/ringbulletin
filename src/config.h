#ifndef CONFIG_H
#define CONFIG_H

#include "fileutils.h"

typedef struct {
	char boardGenerationDirectory[PATH_MAX];
	char boardJsonPath[PATH_MAX];
	char searchHistoryPath[PATH_MAX];
	int maxTitleLength;
	int maxDescriptionLength;
	int searchDepth;
} ConfigValues;

extern int loadConfig(char *configPath, ConfigValues *configValues);

#endif
