#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <limits.h>
#include <cjson/cJSON.h>

#define TEMP_NAME_EXT ".tmp"
#define BASE_NAME_MAX (NAME_MAX - sizeof(TEMP_NAME_EXT))
#define BASE_PATH_MAX (PATH_MAX - sizeof(TEMP_NAME_EXT))
#define URL_MAX 2048

typedef struct {
    int numberOfFiles;
	char **filenames;
} Files;

typedef struct {
    int (*matched)(void *data);
    int (*process)(void *data, struct dirent *file, int count);
    void *data;
} Pattern;

extern cJSON *loadJson(const char *path);
extern char *readFile(const char *path);
extern int writeJson(const cJSON *json, const char *path);
extern int writeFile(const char *content, const int *size, const char *directory, const char *filename);
extern int copyFile(const char *sourcePath, const char *destinationDirectory, const char *destinationFilename);
extern int processFiles(char *path, int (*process)(void *, struct dirent *, int), void *data);
extern int count(int *counter, struct dirent *unused, int count);
extern int populateFilenamesArray(char **filenames, struct dirent *file, int count);
extern int compare(const void *a, const void *b);
extern Files *getFilesMatchingPattern(char *directory, int (*pattern)(void *));
extern Files *getFiles(char *directory);

#endif
