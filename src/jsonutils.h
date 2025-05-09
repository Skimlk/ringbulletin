#ifndef JSONUTILS_H
#define JSONUTILS_H

#include <cjson/cJSON.h>

extern cJSON *loadJson(char *path);
extern int writeJson(cJSON *json, char *path);

#endif
