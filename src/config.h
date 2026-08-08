#ifndef CONFIG_H
#define CONFIG_H

#include "fileutils.h"

typedef struct {
	char boardGenerationUrl[URL_MAX];
	char boardGenerationDirectory[PATH_MAX];
	int searchDepth;
	char theme[PATH_MAX];
} ConfigValues;

extern int loadConfig(char *configPath, ConfigValues *configValues);

#endif
