#ifndef CONFIG_H
#define CONFIG_H

#include <cjson/cJSON.h>

typedef struct {
	char boardJsonPath[256];
	char searchHistoryPath[256];
	int maxTitleLength;
	int maxDescriptionLength;
	int searchDepth;
} ConfigValues;

extern int loadConfigValues(const cJSON *configJson, ConfigValues *configValues);

#endif
