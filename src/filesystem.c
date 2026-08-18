#define __USE_XOPEN
#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "filesystem.h"

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

File readFile(const char *directory, const char *filename) {
	File fileSt = { NULL, 0 }; 
	
	if(invalidFilename(filename))
		return fileSt;

	if(directory) {
		if(invalidPath(directory, filename))
			return fileSt;
	} else {
		directory = "";
	}

	char path[PATH_MAX];
	snprintf(path, PATH_MAX, "%s%s", directory, filename);
	
	FILE *fileHandle = fopen(path, "rb");
	if(!fileHandle) {
		return fileSt;
	}	
	
	struct stat fileStat;
	if(stat(path, &fileStat) == -1) {
		return fileSt;
	}

	fileSt.content = (char*)malloc(fileStat.st_size);
	fileSt.size = fread(fileSt.content, 1, fileStat.st_size, fileHandle);

	fclose(fileHandle);
	
	return fileSt;
}

char *readFileStr(const char *directory, const char *filename) {
	File fileStruct = readFile(directory, filename);
    char *fileString = malloc(fileStruct.size + 1);
    memcpy(fileString, fileStruct.content, fileStruct.size);
    fileString[fileStruct.size] = '\0';
    free(fileStruct.content);
	return fileString;
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
	File sourceContent = readFile(sourceDirectory, sourceFilename);
	int success = writeFile(sourceContent.content, (int *)&sourceContent.size, destinationDirectory, destinationFilename);
	free(sourceContent.content);

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

int fileExists(const char *filename) {
	struct stat st;
	if(stat(filename, &st) == 0) {
		return 1;
	}
	
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

int createDirectory(char *path) {
	struct stat st = {0};

	if (stat(path, &st) == -1) {
		mkdir(path, 0700);
		return 0;
	}

	return 1;
}

void recursiveDirectoryCopy(char *srcDir, char *destDir, FilenameList *excludedSrcDirs) {
	FilenameList *filenameList = getFilenameList(srcDir);
	for(int file = 0; file < filenameList->numberOfFiles; file++) {
		char *srcDirWithFile = NULL;
		char *destDirWithFile = NULL;

		asprintf(&srcDirWithFile, "%s/%s", srcDir, filenameList->filenames[file]);
		asprintf(&destDirWithFile, "%s/%s", destDir, filenameList->filenames[file]);

		int fileIsExcluded = 0;
		for(int excludeDir = 0; excludeDir < excludedSrcDirs->numberOfFiles; excludeDir++) {
			struct stat fileStat, excludeDirStat;
			if(
				stat(srcDirWithFile, &fileStat) == 0 &&
				stat(excludedSrcDirs->filenames[excludeDir], &excludeDirStat) == 0 &&
				fileStat.st_dev == excludeDirStat.st_dev &&
				fileStat.st_ino == excludeDirStat.st_ino
			) {
				fileIsExcluded = 1;
				break;
			}
		}

		if(!fileIsExcluded) {
			if(directoryExists(srcDirWithFile))
				recursiveDirectoryCopy(srcDirWithFile, destDirWithFile, excludedSrcDirs);
			else
				copyFile(NULL, srcDirWithFile, NULL, destDirWithFile);
		}

		free(srcDirWithFile);
		free(destDirWithFile);
	}
	freeFilenameList(filenameList);
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
	(void)unusedDirent;
	*counter = count + 1;
	return 0;
}

int removeCallback(char *directory, struct dirent *file, int count) {
	(void)count;
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
	(void)unusedPattern;
	(void)unusedSeed;
	return 1;
}

FilenameList *getFilenameList(char *directory) {
	return getFilenameListMatchingPattern(directory, alwaysTrue, NULL);
}

int compare(const void *a, const void *b) {
	return strcmp(*(const char **)b, *(const char **)a);
}
