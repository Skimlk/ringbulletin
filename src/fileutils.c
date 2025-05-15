#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <stdint.h>

#include <cjson/cJSON.h>
#include "xxhash.h"

#include "fileutils.h"
#include "stringutils.h"

char *readFile(const char *path) {
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

int writeFile(const char *content, const char *directory, const char *filename) {
	if(strlen(filename) >= BASE_NAME_MAX) {
		fprintf(stderr, "Filename '%s' is too long.\n", filename);
		return 1;
	}

	if(directory) {
		if(strlen(filename) + strlen(directory) >= BASE_PATH_MAX) {
			fprintf(stderr, "Path '%s%s' is too long.\n", directory, filename);
			return 1;
		}
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

	fputs(content, fptr);
	fclose(fptr);

	if(rename(tempPath, path)) {
		fprintf(stderr, "Could not write file '%s' to '%s'.\n", tempPath, path);
		return 1;
	}

	return 0;
}

cJSON *loadJson(const char *path) {
	char *fileContents = readFile(path);
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

	writeFile(jsonData, 0, path);

	cJSON_free(jsonData);

	return 0;
}

int writePost(const PostData *post, const char *directory) {
	char *htmlExtension = ".html";
	char normalizedTitle[strlen(post->title)+1];
	strcpy(normalizedTitle, strip(strlwr(post->title)));

	XXH64_hash_t titleHash = XXH64(normalizedTitle, strlen(normalizedTitle), 0);
	time_t now = time(NULL);

	char filename[64];

	snprintf(filename, sizeof(filename), "%ld_%016llu%s",
		 (long)now, (unsigned long long)titleHash, htmlExtension);

	writeFile(post->title, directory, filename);

	return 0;
}
