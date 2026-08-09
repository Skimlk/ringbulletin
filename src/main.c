/*
	Ring Bulletin:
		- Monitors selected RSS feeds for intent to participate in bulletin board
		- Creates/edits HTML page based on participation
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <cjson/cJSON.h>

#include "config.h"
#include "context.h"
#include "crawler.h"
#include "filesystem.h"
#include "json.h"
#include "render.h"

#define PROGRAM_TITLE "ringbulletin"

void printUsage() {
	printf("\nUsage: %s [OPTION]...\n\n", PROGRAM_TITLE);
}

void printHelpText() {
	printUsage();
	printf("Options:\n");
	printf("  -h\tPrint this help text and exit.\n");
	printf("  -r\tReload all bulletin board files.\n");
}

void printError(char *msg) {
	fprintf(stderr, "%s: error: %s\n", PROGRAM_TITLE, msg);
}

static int generateBoard(int reloadFlag) {
	cJSON *boardJson;
	int ret = 0;

	// Load Config
	ConfigValues config;
	if(loadConfig("config.json", &config))
		return 1;

	// Load Context
	Context ctx;
	ctx.config = &config;
	ctx.searchStartTime = time(NULL);

	asprintf(&ctx.postsDirectoryPath, "%s/posts/", config.boardGenerationDirectory);
	asprintf(&ctx.viewsDirectoryPath, "%s/views/", config.boardGenerationDirectory);
	asprintf(&ctx.iconsDirectoryPath, "%s/icons/", config.boardGenerationDirectory);
	asprintf(&ctx.cssDirectoryPath, "%s/css/", config.boardGenerationDirectory);

	asprintf(&ctx.boardJsonUrl, "%s/%s", config.boardGenerationUrl, "board.json");
	normalizeUrl(&ctx.boardJsonUrl);

	asprintf(&ctx.boardHtmlUrl, "%s/%s", config.boardGenerationUrl, "board.html");
	normalizeUrl(&ctx.boardHtmlUrl);

	// Load board file
	boardJson = loadJson(NULL, "./board.json");

	if(!boardJson) {
		printError("Unable to load board values.");
		ret = 1;
		goto cleanup;
	}

	char *generationDirectories[] = {
		config.boardGenerationDirectory,
		ctx.postsDirectoryPath,
		ctx.viewsDirectoryPath,
		ctx.iconsDirectoryPath,
		ctx.cssDirectoryPath
	};

	for(size_t i = 0; i < sizeof(generationDirectories)/sizeof(char *); i++) {
		if(!directoryExists(generationDirectories[i]))
			createDirectory(generationDirectories[i]);
		else {
			if(reloadFlag)
				processFiles(generationDirectories[i], (void *)removeCallback, generationDirectories[i]);
		}
	}

	if(reloadFlag)
		removeFile(NULL, "./history.json");

	//Copy assets to board generation directory
	FilenameList excludedSrcDir = {1, (char *[]){"./assets/css/themes/"}};
	recursiveDirectoryCopy("./assets/", config.boardGenerationDirectory, &excludedSrcDir);

	char *themeFile = NULL;
	asprintf(&themeFile, "%s.css", config.theme);
	copyFile("./assets/css/themes/", themeFile, ctx.cssDirectoryPath, "theme.css");
	free(themeFile);
	
	copyFile(NULL, "./board.json", config.boardGenerationDirectory, "board.json");

	// Search Boards and Feeds
	searchedAlready(&ctx, "boards", ctx.boardJsonUrl);
	searchBoard(boardJson, &ctx, 0);

	writeBulletin(&ctx);

cleanup:
	free(ctx.boardHtmlUrl);
	free(ctx.boardJsonUrl);
	free(ctx.postsDirectoryPath);
	free(ctx.viewsDirectoryPath);
	free(ctx.iconsDirectoryPath);
	free(ctx.cssDirectoryPath);
	cJSON_Delete(boardJson);
	return ret;
}

int main(int argc, char **argv) {
	int reloadFlag = 0;
	int ret = 0;

	int opt;
	while((opt = getopt(argc, argv, "rh")) != -1) {
		switch(opt) {
			case 'r':
				reloadFlag = 1;
				break;
			case 'h':
				printHelpText();
				goto cleanup;
			default:
				ret = 1;
				goto cleanup;
		}
	}

	ret = generateBoard(reloadFlag);

cleanup:
	return ret;
}
