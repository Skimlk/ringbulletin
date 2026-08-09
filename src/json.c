#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cjson/cJSON.h>

#include "filesystem.h"
#include "json.h"

cJSON *loadJson(const char *directory, const char *path) {
	cJSON *ret = NULL;

	char *fileContents = readFileStr(directory, path);
	if(!fileContents) {
		goto cleanup;
	}

	cJSON *json = cJSON_Parse(fileContents);

	if(!json) {
		const char *errorMsg = cJSON_GetErrorPtr();
		if(errorMsg && *errorMsg != '\0') {
			fprintf(stderr, "JSON parse error before: %.100s\n", errorMsg);
		} else {
			fprintf(stderr, "JSON parse error: unknown location\n");
		}

		goto cleanup;
	}

	ret = json;

cleanup:
	free(fileContents);
	return ret;
}

int writeJson(const cJSON *json, const char *directory, const char *path) {
	char *jsonData = cJSON_Print(json);

	if(!jsonData) {
		fprintf(stderr, "Unable to export data for file '%s'.\n", path);
		return 1;
	}

	writeFile(jsonData, 0, directory, path);

	cJSON_free(jsonData);

	return 0;
}

int getJsonHistoryItemProperty(const char *categoryString, const char *itemString, const char *propertyName, void *property) {
	int ret = 0;
	const cJSON *history = loadJson(NULL, "./history.json");
	if (history == NULL) {
		ret = 1;
		goto cleanup;
	};

	cJSON * const categoryJson = cJSON_GetObjectItemCaseSensitive(history, categoryString);
	if (categoryJson == NULL) {
		ret = 1;
		goto cleanup;
	};
	
	cJSON * const itemJson = cJSON_GetObjectItemCaseSensitive(categoryJson, itemString);
	if (itemJson == NULL) {
		ret = 1;
		goto cleanup;
	}

	cJSON * const propertyJson = cJSON_GetObjectItemCaseSensitive(itemJson, propertyName);
	if(propertyJson == NULL) {
		ret = 1;
		goto cleanup;
	};

	if(cJSON_IsNumber(propertyJson))
		memcpy(property, &propertyJson->valuedouble, sizeof(double));
	else
		*(char **)property = strdup(propertyJson->valuestring);

cleanup:
	cJSON_Delete((cJSON *)history);
	return ret;
}

CJSON_PUBLIC(cJSON*) addStringToJsonHistoryItem(cJSON *itemJson, const char *stringName, void *string) {
	return cJSON_AddStringToObject((cJSON * const)itemJson, stringName, (const char *)string);
}

CJSON_PUBLIC(cJSON*) addDoubleToJsonHistoryItem(cJSON *itemJson, const char *numberName, void *number) {
	return cJSON_AddNumberToObject((cJSON * const)itemJson, numberName, *(const double *)number);
}

void updateJsonHistoryItemProperty(const char *categoryString, const char *itemString, const char *propertyName, void *property, CJSON_PUBLIC(cJSON*) (*addPropertyToItem)(cJSON *, const char *, void *)) {
	cJSON *history = loadJson(NULL, "./history.json");
	if (history == NULL) history = cJSON_CreateObject();

	cJSON *categoryJson = cJSON_GetObjectItemCaseSensitive(history, categoryString);
	if (categoryJson == NULL) {
		categoryJson = cJSON_CreateObject();
		cJSON_AddItemToObject(history, categoryString, categoryJson);
	}

	cJSON *itemJson = cJSON_GetObjectItemCaseSensitive(categoryJson, itemString);
	if (itemJson == NULL) {
		itemJson = cJSON_CreateObject();
		cJSON_AddItemToObject(categoryJson, itemString, itemJson);
	}

	cJSON_DeleteItemFromObjectCaseSensitive(itemJson, propertyName);
	addPropertyToItem(itemJson, propertyName, property);
	
	writeJson(history, NULL, "./history.json");
	cJSON_Delete(history);
}
