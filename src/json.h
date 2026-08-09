#ifndef JSON_H
#define JSON_H

#include <cjson/cJSON.h>

extern cJSON *loadJson(const char *directory, const char *path);
extern int writeJson(const cJSON *json, const char *directory, const char *path);
extern int getJsonHistoryItemProperty(const char *categoryString, const char *itemString,
    const char *propertyName, void *property);
CJSON_PUBLIC(cJSON*) addStringToJsonHistoryItem(cJSON *itemJson, const char *stringName, void *string);
CJSON_PUBLIC(cJSON*) addDoubleToJsonHistoryItem(cJSON *itemJson, const char *numberName, void *number);
extern void updateJsonHistoryItemProperty(const char *categoryString, const char *itemString,
    const char *propertyName, void *property, CJSON_PUBLIC(cJSON*) (*addPropertyToItem)(cJSON *, const char *, void *));

#endif
