#include <stdio.h>
#include <cjson/cJSON.h>
#include "config.h"

int loadIntValue(int *configValue, const cJSON *configJson, const char *key) {
	const cJSON *value = cJSON_GetObjectItemCaseSensitive(configJson, key);
	if(cJSON_IsString(value)) {
		printf("Config value '%s' is not an integer.", key);
		return 1;
	}
	
	*configValue = value->valueint;

	return 0;
}

int loadConfigValues(const cJSON *configJson, ConfigValues *configValues) {
	if(loadIntValue(&configValues->maxTitleLength, configJson, "maxTitleLength")
		|| loadIntValue(&configValues->maxTitleLength, configJson, "maxTitleLength")
		|| loadIntValue(&configValues->maxTitleLength, configJson, "maxTitleLength")
	) return 1;

	return 0;
}
