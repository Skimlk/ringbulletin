#define __USE_XOPEN
#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <stdint.h>
#include <dirent.h>

#include <libxml/HTMLtree.h>
#include <cjson/cJSON.h>
#include "xxhash.h"

#include "fileutils.h"
#include "stringutils.h"
#include "context.h"

#define UNUSED(x) (void)(x)

int invalidFilename(const char *filename) {
	if(strlen(filename) >= BASE_NAME_MAX) {
		fprintf(stderr, "Filename '%s' is too long.\n", filename);
		return 1;
	}

	return 0;
}

int invalidPath(const char *directory, const char *filename) {
	if(strlen(filename) + strlen(directory) >= BASE_PATH_MAX) {
		fprintf(stderr, "Path '%s%s' is too long.\n", directory, filename);
		return 1;
	}

	return 0;
}

char *readFile(const char *directory, const char *filename) {
	if(invalidFilename(filename))
		return NULL;

	if(directory) {
		if(invalidPath(directory, filename))
			return NULL;
	} else {
		directory = "";
	}

	char path[PATH_MAX];
	snprintf(path, PATH_MAX, "%s%s", directory, filename);
	
	FILE *file = fopen(path, "rb");
	if(!file) {
		printf("Could not open file '%s'.\n", path);
		return NULL;
	}	
	
	struct stat fileStat;
	if(stat(path, &fileStat) == -1) {
		printf("Could not get file '%s' status.\n", path);
		return NULL;
	}

	char *fileContents = (char*)malloc(fileStat.st_size + 1);
	size_t bytesRead = fread(fileContents, 1, fileStat.st_size, file);
	fclose(file);
	
	fileContents[bytesRead] = '\0';

	return fileContents;
}

int writeFile(const char *content, const int *size, const char *directory, const char *filename) {
	if(!content) {
		fprintf(stderr, "Content is null or empty.\n");
		return 1;
	}

	if(invalidFilename(filename))
		return 1;

	if(directory) {
		if(invalidPath(directory, filename))
			return 1;
	} else {
		directory = "";
	}

	int pathSize = strlen(directory) + strlen(filename) + 1;
	char path[pathSize];
	snprintf(path, pathSize, "%s%s", directory, filename);

	int tempPathSize = pathSize + strlen(TEMP_NAME_EXT);
	char tempPath[tempPathSize];
	snprintf(tempPath, tempPathSize, "%s%s", path, TEMP_NAME_EXT);

	FILE *fptr = fopen(tempPath, "w");

	if(!fptr) {
		fprintf(stderr, "Could not open file '%s'.\n", tempPath);
		return 1;
	}

	if(size) {
		fwrite(content, 1, *size, fptr);
	} else { 
		fputs(content, fptr);
	}

	fclose(fptr);

	if(rename(tempPath, path)) {
		fprintf(stderr, "Could not write file '%s' to '%s'.\n", tempPath, path);
		return 1;
	}

	return 0;
}

int copyFile(
	const char *sourceDirectory, const char *sourceFilename,
	const char *destinationDirectory, const char *destinationFilename
) {
	char *sourceContent = readFile(sourceDirectory, sourceFilename);
	int success = writeFile(sourceContent, NULL, destinationDirectory, destinationFilename);
	free(sourceContent);

	if(success) {
		fprintf(stderr, "Could not copy file '%s' in '%s' to '%s' in '%s'.\n", 
			sourceFilename, sourceDirectory, destinationFilename, destinationDirectory);
	}

	return success;
}

int removeFile(const char *directory, const char *filename) {
	if(invalidFilename(filename))
		return 1;

	if(directory) {
		if(invalidPath(directory, filename))
			return 1;
	} else {
		directory = "";
	}

	char path[PATH_MAX];
	snprintf(path, PATH_MAX, "%s%s", directory, filename);

	if(remove(path))
		return 1;

	return 0;
}

int directoryExists(const char *directoryPath) {
	DIR *directory = opendir(directoryPath);
	if(directory != NULL) {
		closedir(directory);
		return 1;
	}

	return 0;
}

cJSON *loadJson(const char *directory, const char *path) {
	char *fileContents = readFile(directory, path);
	if(!fileContents) {
		return NULL;
	}

	cJSON *json = cJSON_Parse(fileContents);
	free(fileContents);

	if(!json) {
		const char *errorMsg = cJSON_GetErrorPtr();
		if(errorMsg) {
			fprintf(stderr, "Error before: %s\n", errorMsg);
		}

		return NULL;
	}

	return json;
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

int getJsonHistoryItemProperty(Context *ctx, const char *categoryString, const char *itemString, const char *propertyName, void *property) {
	int ret = 0;
	const cJSON *history = loadJson(ctx->config->boardGenerationDirectory, "history.json");
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

void updateJsonHistoryItemProperty(Context *ctx, const char *categoryString, const char *itemString, const char *propertyName, void *property, CJSON_PUBLIC(cJSON*) (*addPropertyToItem)(cJSON *, const char *, void *)) {
	cJSON *history = loadJson(ctx->config->boardGenerationDirectory, "history.json");
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
	
	writeJson(history, ctx->config->boardGenerationDirectory, "history.json");
	cJSON_Delete(history);
}

int processFiles(char *path, int (*process)(void *, struct dirent *, int), void *data) {
	DIR *directoryStream = opendir(path);	
	struct dirent *directoryEntry;
	if(!directoryStream) 
		return 1;
	int count = 0;
	while ((directoryEntry = readdir(directoryStream)) != NULL) {
		if (strcmp(directoryEntry->d_name, ".") != 0 &&
			strcmp(directoryEntry->d_name, "..") != 0) {
			if(!process(data, directoryEntry, count))
				count++;
		}
	}
	closedir(directoryStream);
	return 0;
}

int count(int *counter, struct dirent *unusedDirent, int count) {
	UNUSED(unusedDirent);
	*counter = count + 1;
	return 0;
}

int removeCallback(char *directory, struct dirent *file, int count) {
	UNUSED(count);
	removeFile(directory, file->d_name);
	return 0;
}

int populateFilenamesArray(char **filenames, struct dirent *file, int count) {
	filenames[count] = strdup(file->d_name);
	return 0;
}

int filenameMatchesPattern(Pattern *pattern, struct dirent *file, int count) {
	if(pattern->matched(file->d_name, pattern->seed)) {
		pattern->process(pattern->data, file, count);
		return 0;
	}

	return 1;
}

void freeFilenameList(FilenameList *filenameList) {
    if(filenameList == NULL)
        return;

    for(size_t i = 0; i < (size_t)filenameList->numberOfFiles; i++)
        free(filenameList->filenames[i]);

    free(filenameList->filenames);
    free(filenameList);
}

FilenameList *getFilenameListMatchingPattern(char *directory, int (*pattern)(void *, void *), void *seed) {
	FilenameList *files = malloc(sizeof(FilenameList));
	files->numberOfFiles = 0;
	
	Pattern countIfMatching = {
		pattern,
		(void *)count,
		&files->numberOfFiles,
		seed
	};
    processFiles(directory, (void *)filenameMatchesPattern, &countIfMatching);

	files->filenames = malloc(files->numberOfFiles * sizeof(*files->filenames));

	Pattern populateFilenamesArrayIfMatching = {
		pattern,
		(void *)populateFilenamesArray,
		files->filenames,
		seed
	};
    processFiles(directory, (void *)filenameMatchesPattern, &populateFilenamesArrayIfMatching);

    return files;
}

int contains(void *string, void *substring) {
	if(strstr((char *)string, (char *)substring) != NULL) {
		return 1;
	}
	return 0;
}

int alwaysTrue(void *unusedPattern, void *unusedSeed) {
	UNUSED(unusedPattern);
	UNUSED(unusedSeed);
	return 1;
}

FilenameList *getFilenameList(char *directory) {
	return getFilenameListMatchingPattern(directory, alwaysTrue, NULL);
}

int compare(const void *a, const void *b) {
	return strcmp(*(const char **)b, *(const char **)a);
}