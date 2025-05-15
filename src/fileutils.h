#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <limits.h>
#include <cjson/cJSON.h>

#define TEMP_NAME_EXT ".tmp"
#define BASE_NAME_MAX (NAME_MAX - sizeof(TEMP_NAME_EXT))
#define BASE_PATH_MAX (PATH_MAX - sizeof(TEMP_NAME_EXT))
#define URL_MAX 2048

typedef struct {
	char *title;
} PostData;

extern cJSON *loadJson(const char *path);
extern int writeJson(const cJSON *json, const char *path);
extern int writePost(const PostData *post, const char *directory);

#endif
