#ifndef JSON_H
#define JSON_H

#include <cjson/cJSON.h>

extern cJSON *loadJson(const char *directory, const char *path);
extern int writeJson(const cJSON *json, const char *directory, const char *path);
extern int getJsonHistoryItemProperty(const char *categoryString, const char *itemString,
    const char *propertyName, void *property);
extern void removeStringFromJsonArray(cJSON *listJson, const char *string);
extern void addStringToJsonItem(cJSON *itemJson, const char *stringName, void *string);
extern void addDoubleToJsonItem(cJSON *itemJson, const char *numberName, void *number);
extern void printJsonArray(cJSON *listJson);
extern void replaceJsonValue(cJSON *itemJson, char *propertyName, char *propertyText, void (*addPropertyToItem)(cJSON *, const char *, void *));
extern void updateJsonHistoryItemProperty(const char *categoryString, const char *itemString,
    char *propertyName, void *property, void (*addPropertyToItem)(cJSON *, const char *, void *));

#endif
