#ifndef CONFIG_H
#define CONFIG_H

#include <cjson/cJSON.h>

typedef struct {
	int maxTitleLength;
	int maxDescriptionLength;
	int searchDepth;
} ConfigValues;

extern int loadConfigValues(const cJSON *configJson, ConfigValues *configValues);

#endif
