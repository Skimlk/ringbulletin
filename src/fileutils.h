#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <limits.h>
#include <dirent.h>
#include <cjson/cJSON.h>

typedef struct Context Context;

#define TEMP_NAME_EXT ".tmp"
#define BASE_NAME_MAX (NAME_MAX - sizeof(TEMP_NAME_EXT))
#define BASE_PATH_MAX (PATH_MAX - sizeof(TEMP_NAME_EXT))
#define URL_MAX 2048

typedef struct {
    int numberOfFiles;
	char **filenames;
} FilenameList;

typedef struct {
    int (*matched)(void *data, void *seed);
    int (*process)(void *data, struct dirent *file, int count);
    void *data;
    void *seed;
} Pattern;

extern char *readFile(const char *directory, const char *filename);
extern int writeFile(const char *content, const int *size, const char *directory, const char *filename);
extern int copyFile(const char *sourceDirectory, const char *sourceFilename,
    const char *destinationDirectory, const char *destinationFilename);
extern int removeFile(const char *directory, const char *filename);
extern int directoryExists(const char *directory);

extern cJSON *loadJson(const char *directory, const char *path);
extern int writeJson(const cJSON *json, const char *directory, const char *path);
extern int getJsonHistoryItemProperty(Context *ctx, const char *categoryString, const char *itemString,
    const char *propertyName, void *property);
CJSON_PUBLIC(cJSON*) addStringToJsonHistoryItem(cJSON *itemJson, const char *stringName, void *string);
CJSON_PUBLIC(cJSON*) addDoubleToJsonHistoryItem(cJSON *itemJson, const char *numberName, void *number);
extern void updateJsonHistoryItemProperty(Context *ctx, const char *categoryString, const char *itemString,
    const char *propertyName, void *property, CJSON_PUBLIC(cJSON*) (*addPropertyToItem)(cJSON *, const char *, void *));

extern int processFiles(char *path, int (*process)(void *, struct dirent *, int), void *data);
extern int count(int *counter, struct dirent *unused, int count);
extern int populateFilenamesArray(char **filenames, struct dirent *file, int count);
extern int removeCallback(char *directory, struct dirent *file, int count);
extern void freeFilenameList(FilenameList *filenameList);
extern FilenameList *getFilenameListMatchingPattern(char *directory, int (*pattern)(void *, void *), void *seed);
extern FilenameList *getFilenameList(char *directory);
extern int contains(void *, void *);
extern int compare(const void *a, const void *b);

#endif
