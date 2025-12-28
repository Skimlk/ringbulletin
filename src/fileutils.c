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
		fprintf(stderr, "Content is null or empty.\n", filename);
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

	char path[PATH_MAX];
	snprintf(path, PATH_MAX, "%s%s", directory, filename);

	char tempPath[PATH_MAX];
	snprintf(tempPath, PATH_MAX, "%s%s", path, TEMP_NAME_EXT);

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

cJSON *loadJson(const char *path) {
	char *fileContents = readFile(NULL, path);
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

int writeJson(const cJSON *json, const char *path) {
	char *jsonData = cJSON_Print(json);

	if(!jsonData) {
		fprintf(stderr, "Unable to export data for file '%s'.\n", path);
		return 1;
	}

	writeFile(jsonData, 0, 0, path);

	cJSON_free(jsonData);

	return 0;
}

int processFiles(char *path, int (*process)(void *, struct dirent *, int), void *data) {
	DIR *directoryStream = opendir(path);	
	struct dirent *directoryEntry;
	if(!directoryStream) 
		return 1;
	int count = 0;
	while ((directoryEntry = readdir(directoryStream)) != NULL) {
		if(directoryEntry->d_name[0] != '.') {
			if(!process(data, directoryEntry, count))
				count++;
		}
	}
	closedir(directoryStream);
	return 0;
}

int count(int *counter, struct dirent *unused, int count) {
	*counter = count + 1;
	return 0;
}

int populateFilenamesArray(char **filenames, struct dirent *file, int count) {
	filenames[count] = strdup(file->d_name);
	return 0;
}

int compare(const void *a, const void *b) {
	return strcmp(*(const char **)b, *(const char **)a);
}

int matchesPattern(Pattern *pattern, struct dirent *file, int count) {
	if(pattern->matched(pattern->data)) {
		return pattern->process(pattern->data, file, count);
	}

	return 1;
}

Files *getFilesMatchingPattern(char *directory, int (*pattern)(void *)) {
	Files *files = malloc(sizeof(Files));

	Pattern countIfMatching = {
		pattern,
		(void *)count,
		&files->numberOfFiles
	};

    files->numberOfFiles = 0;
    processFiles(directory, (void *)matchesPattern, &countIfMatching);

	files->filenames = malloc(files->numberOfFiles * sizeof(*files->filenames));

	Pattern populateFilenamesArrayIfMatching = {
		pattern,
		(void *)populateFilenamesArray,
		files->filenames
	};

    processFiles(directory, (void *)matchesPattern, &populateFilenamesArrayIfMatching);

    return files;
}

int alwaysTrue(void *unused) {
	return 1;
}

Files *getFiles(char *directory) {
	return getFilesMatchingPattern(directory, alwaysTrue);
}