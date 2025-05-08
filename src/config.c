#include <stdio.h>
#include <string.h>
#include <cjson/cJSON.h>
#include "config.h"

int loadIntValue(int *configValue, const cJSON *configJson, const char *key) {
	const cJSON *value = cJSON_GetObjectItemCaseSensitive(configJson, key);
	if(cJSON_IsString(value)) {
		printf("Config value '%s' is not an integer.\n", key);
		return 1;
	}
	
	*configValue = value->valueint;

	return 0;
}

int loadStringValue(char *configValue, const cJSON *configJson, const char *key) {
	const cJSON *value = cJSON_GetObjectItemCaseSensitive(configJson, key);
	if(!cJSON_IsString(value)) {
		printf("Config value '%s' is not a string.\n", key);
		return 1;
	}

	strcpy(configValue, value->valuestring);

	return 0;
}

int loadConfigValues(const cJSON *configJson, ConfigValues *configValues) {
	if(loadStringValue(configValues->boardJsonPath, configJson, "boardJsonPath")
		|| loadStringValue(configValues->searchHistoryPath, configJson, "searchHistoryPath")
		|| loadIntValue(&configValues->maxTitleLength, configJson, "maxTitleLength")
		|| loadIntValue(&configValues->maxDescriptionLength, configJson, "maxDescriptionLength")
		|| loadIntValue(&configValues->searchDepth, configJson, "searchDepth")
	) return 1;

	return 0;
}
