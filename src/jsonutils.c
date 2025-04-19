#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <cjson/cJSON.h>

char *readFile(const char *path) {
	FILE *file = fopen(path, "rb");
	if(!file) {
		printf("Error opening file '%s'.\n", path);
		return NULL;
	}	
	
	struct stat fileStat;
	if(stat(path, &fileStat) == -1) {
		printf("Error reading file '%s' status.\n", path);
		return NULL;
	}

	char *fileContents = (char*)malloc(fileStat.st_size + 1);
	size_t bytesRead = fread(fileContents, 1, fileStat.st_size, file);
	fclose(file);
	
	fileContents[bytesRead] = '\0';

	return fileContents;
}

cJSON *loadJson(const char *path) {
	char *fileContents = readFile(path);
	if(!fileContents) {
		return NULL;
	}
	
	printf("%s", fileContents);

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
