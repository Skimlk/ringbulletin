#include <stdio.h>
#include <string.h>
#include <cjson/cJSON.h>
#include "config.h"

int loadIntValue(int *configValue, const cJSON *configJson, const char *key) {
	const cJSON *value = cJSON_GetObjectItemCaseSensitive(configJson, key);
	if(!value || cJSON_IsString(value)) {
		printf("Config value '%s' is missing or not an integer.\n", key);
		return 1;
	}
	
	*configValue = value->valueint;

	return 0;
}

int loadStringValue(char *configValue, const cJSON *configJson, const char *key, const size_t maxLength) {
	const cJSON *value = cJSON_GetObjectItemCaseSensitive(configJson, key);
	if(!value || !cJSON_IsString(value)) {
		printf("Config value '%s' is missing or not a string.\n", key);
		return 1;
	}

	if(strlen(value->valuestring) > maxLength) {
		printf("Config value '%s' is longer than max length of %ld.\n", key, maxLength);
		return 1;
	}

	strncpy(configValue, value->valuestring, maxLength);
	configValue[maxLength] = '\0';

	return 0;
}

int loadConfigValues(const cJSON *configJson, ConfigValues *configValues) {
	if(loadStringValue(configValues->boardJsonPath, configJson, "boardJsonPath", 252) // 252 = 256 - strlen(".tmp")
		|| loadStringValue(configValues->searchHistoryPath, configJson, "searchHistoryPath", 252)
		|| loadIntValue(&configValues->maxTitleLength, configJson, "maxTitleLength")
		|| loadIntValue(&configValues->maxDescriptionLength, configJson, "maxDescriptionLength")
		|| loadIntValue(&configValues->searchDepth, configJson, "searchDepth")
	) return 1;

	return 0;
}
