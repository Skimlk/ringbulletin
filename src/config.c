#include <stdio.h>
#include <string.h>
#include <cjson/cJSON.h>

#include "config.h"
#include "fileutils.h"

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

int loadConfig(char *configPath, ConfigValues *configValues) {
	int ret = 0;
	cJSON *configJson = loadJson(configPath);
	if(!configJson) {
		ret = 1;
		goto cleanup;
	}

	if(
		loadStringValue(configValues->boardGenerationDirectory, configJson, "boardGenerationDirectory", PATH_MAX) ||
		loadStringValue(configValues->boardJsonPath, configJson, "boardJsonPath", PATH_MAX) ||
		loadStringValue(configValues->searchHistoryPath, configJson, "searchHistoryPath", PATH_MAX) ||
		loadIntValue(&configValues->maxTitleLength, configJson, "maxTitleLength") ||
		loadIntValue(&configValues->maxDescriptionLength, configJson, "maxDescriptionLength") ||
		loadIntValue(&configValues->searchDepth, configJson, "searchDepth")
	) {
		printf("Unable to load config values.");
		ret = 1;
		goto cleanup;
	} 

	printf("Loaded config '%s'.\n", configPath);

cleanup:
	cJSON_Delete(configJson);
	return ret;
}
