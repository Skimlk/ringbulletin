#ifndef CONFIG_H
#define CONFIG_H

#include "fileutils.h"

typedef struct {
	char boardJsonUrl[URL_MAX];
	char boardJsonPath[PATH_MAX];
	char boardGenerationDirectory[PATH_MAX];
	int searchDepth;
} ConfigValues;

extern int loadConfig(char *configPath, ConfigValues *configValues);

#endif
