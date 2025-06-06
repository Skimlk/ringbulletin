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

int writeFile(const char *content, const int *size, const char *directory, const char *filename) {
	if(!content) {
		fprintf(stderr, "Content is null or empty.\n", filename);
		return 1;
	}

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

	writeFile(jsonData, 0, 0, path);

	cJSON_free(jsonData);

	return 0;
}

int writePost(const PostData *post, const char *directory) {
	char normalizedTitle[strlen(post->title)+1];
	strcpy(normalizedTitle, normalize(post->title));

	XXH64_hash_t titleHash = XXH64(normalizedTitle, strlen(normalizedTitle), 0);

	struct tm tm = {0};

	if (strptime(post->pubDate, "%a, %d %b %Y %H:%M:%S %z", &tm) == NULL) {
		fprintf(stderr, "Failed to parse date-time.\n");
		return 1;
	}

	time_t now = mktime(&tm);

	char filename[64];

	snprintf(filename, sizeof(filename), "%ld_%016llu.html",
		 (long)now, (unsigned long long)titleHash);

	htmlDocPtr doc = htmlNewDoc(BAD_CAST "http://www.w3.org/TR/html4/strict.dtd", BAD_CAST "HTML");

	xmlNodePtr html = xmlNewNode(NULL, BAD_CAST "html");
	xmlDocSetRootElement(doc, html);

	xmlNodePtr head = xmlNewChild(html, NULL, BAD_CAST "head", NULL);
	xmlNewChild(head, NULL, BAD_CAST "title", BAD_CAST "Sample HTML Page");
	xmlNodePtr link = xmlNewChild(head, NULL, BAD_CAST "link", NULL);
	xmlNewProp(link, BAD_CAST "rel", BAD_CAST "stylesheet");
	xmlNewProp(link, BAD_CAST "href", BAD_CAST "../style.css");
	
	xmlNodePtr body = xmlNewChild(html, NULL, BAD_CAST "body", NULL);
	xmlNewChild(body, NULL, BAD_CAST "h2", BAD_CAST post->title);
	xmlNewChild(body, NULL, BAD_CAST "p", BAD_CAST post->link);

	xmlChar *postSerialized;
	int size = 0;
	htmlDocDumpMemoryFormat(doc, &postSerialized, &size, 1);

	if(!postSerialized) {
		printf("postSerialized is empthy\n");
	}

	char postsDirectory[PATH_MAX];
	snprintf(postsDirectory, sizeof(postsDirectory), "%s/posts/", directory);

	writeFile((const char *)postSerialized, &size, postsDirectory, filename);

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
			if(process(data, directoryEntry, count))
				return 1;
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

int initializeBulletin() {
	int numberOfPosts = 0;
	processFiles("./static/posts", count, &numberOfPosts);

	char *filenames[numberOfPosts];
	processFiles("./static/posts", populateFilenamesArray, filenames);
	qsort(filenames, numberOfPosts, sizeof(char *), compare);

	htmlDocPtr doc = htmlNewDoc(BAD_CAST "http://www.w3.org/TR/html4/strict.dtd", BAD_CAST "HTML");
	xmlNodePtr html = xmlNewNode(NULL, BAD_CAST "html");
	xmlDocSetRootElement(doc, html);
	xmlNodePtr head = xmlNewChild(html, NULL, BAD_CAST "head", NULL);
	xmlNodePtr link = xmlNewChild(head, NULL, BAD_CAST "link", NULL);
	xmlNewProp(link, BAD_CAST "rel", BAD_CAST "stylesheet");
	xmlNewProp(link, BAD_CAST "href", BAD_CAST "style.css");

	xmlNodePtr body = xmlNewChild(html, NULL, BAD_CAST "body", NULL);
		
	for (int i = 0; i < numberOfPosts; i++) {
		xmlNodePtr iframe = xmlNewChild(body, NULL, BAD_CAST "iframe", NULL);
		char iframePath[BASE_PATH_MAX];
		printf("%s\n", filenames[i]);
		snprintf(iframePath, sizeof(iframePath), "./posts/%s", filenames[i]);
		xmlNewProp(iframe, BAD_CAST "src", BAD_CAST iframePath);
		xmlNewChild(body, NULL, BAD_CAST "br", NULL);

		free(filenames[i]);
	}
	
	xmlChar *postSerialized;
	int size = 0;
	htmlDocDumpMemoryFormat(doc, &postSerialized, &size, 1);

	if(!postSerialized) {
		printf("postSerialized is empthy\n");
	}

	writeFile((const char *)postSerialized, &size, "./static/", "board.html");
	return 0;
}
