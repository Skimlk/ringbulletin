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

	if(!json)
		goto cleanup;

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

void removeStringFromJsonArray(cJSON *arrayJson, const char *string) {
	cJSON *stringJson = NULL;
	cJSON_ArrayForEach(stringJson, arrayJson) {
		if(strcmp(stringJson->valuestring, string) == 0) {
			cJSON emptyJsonWithNextItem;
			emptyJsonWithNextItem.next = stringJson->next;
			cJSON_Delete(cJSON_DetachItemViaPointer(arrayJson, stringJson));
			stringJson = &emptyJsonWithNextItem;
		}
	}
}

void addStringToJsonItem(cJSON *itemJson, const char *stringName, void *string) {
	if(stringName == NULL) {
		removeStringFromJsonArray(itemJson, string);
		cJSON_AddItemToArray(itemJson, cJSON_CreateString((const char *)string));
		return;
	}

	cJSON_AddStringToObject(itemJson, stringName, (const char *)string);
}

void addDoubleToJsonItem(cJSON *itemJson, const char *numberName, void *number) {
	if(numberName == NULL) {
		cJSON_AddItemToArray(itemJson, cJSON_CreateNumber(*(const double *)number));
		return;
	}

	cJSON_AddNumberToObject(itemJson, numberName, *(const double *)number);
}

void printJsonArray(cJSON *arrayJson) {
	cJSON *item = NULL;
	cJSON_ArrayForEach(item, arrayJson) {
		if (cJSON_IsString(item)) {
            printf("%s\n", item->valuestring);
        }
		else if (cJSON_IsNumber(item)) {
            printf("%f\n", item->valuedouble);
        }
	}
}

void replaceJsonValue(cJSON *itemJson, char *propertyName, char *propertyText, void (*addPropertyToItem)(cJSON *, const char *, void *)) {
	cJSON_DeleteItemFromObjectCaseSensitive(itemJson, propertyName);
	addPropertyToItem(itemJson, propertyName, propertyText);
}

void updateJsonHistoryItemProperty(const char *categoryString, const char *itemString, char *propertyName, void *property, void (*addPropertyToItem)(cJSON *, const char *, void *)) {
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

	replaceJsonValue(itemJson, propertyName, property, addPropertyToItem);
	
	writeJson(history, NULL, "./history.json");
	cJSON_Delete(history);
}
