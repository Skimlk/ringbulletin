/*
	Ring Bulletin:
		- Monitors selected RSS feeds for intent to participate in bulletin board
		- Creates/edits HTML page based on participation
*/

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
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
#include "prompt.h"
#include "render.h"
#include "stringutils.h"

#define CONFIG_JSON_PATH "config.json"
#define BOARD_JSON_PATH "board.json"

void printInitMessage(char *filename) {
	printf("'%s' does not exist. Run 'ringbulletin init' to generate it.\n", filename);
}

void printUsage(int cur, char **argv, char *argName) {
	printf("Usage: ");
	for(int i = 0; i < cur; i++) {
		printf("%s ", argv[i]);
	}
	printf("<%s>\n", argName);
}

void printUsageCommands(int cur, char **argv, void *commands, size_t commandCount, size_t commandStructSize) {
	printUsage(cur, argv, "command");
	
	printf("\nCommands:\n");
	for(size_t i = 0; i < commandCount; i++)
		printf("%s\n", *(char **)((uintptr_t)commands + (i * commandStructSize)));
}

int generateBoard(int regenerateFlag) {
	cJSON *boardJson;
	int ret = 0;

	ConfigValues config;
	if(loadConfig(CONFIG_JSON_PATH, &config)) {
		printInitMessage(CONFIG_JSON_PATH);
		return 1;
	}

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

	boardJson = loadJson(NULL, BOARD_JSON_PATH);

	if(!boardJson) {
		printInitMessage(BOARD_JSON_PATH);
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
			if(regenerateFlag)
				processFiles(generationDirectories[i], (void *)removeCallback, generationDirectories[i]);
		}
	}

	if(regenerateFlag)
		removeFile(NULL, "./history.json");

	int numOfExcludedDirs = 1;
	FilenameList excludedSrcDir = {numOfExcludedDirs, (char *[]){"./assets/css/themes/"}};
	recursiveDirectoryCopy("./assets/", config.boardGenerationDirectory, &excludedSrcDir);

	char *themeFile = NULL;
	asprintf(&themeFile, "%s.css", config.theme);
	copyFile("./assets/css/themes/", themeFile, ctx.cssDirectoryPath, "theme.css");
	free(themeFile);
	
	copyFile(NULL, "./board.json", config.boardGenerationDirectory, "board.json");

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

int regenerate() {
	bool shouldRegenerate = true;
	return generateBoard(shouldRegenerate);
}

int listArrayOperation(cJSON *propertyJson) {
	printJsonArray(propertyJson);
	return 0;
}

int addArrayOperation(cJSON *propertyJson, char **argv, int argc, int cur) {
	if(cur == argc) {
		printUsage(cur, argv, propertyJson->string);
		return 1;
	}

	for(int i = cur; i < argc; i++)
		addStringToJsonItem(propertyJson, NULL, argv[i]);

	return 0;
}

int removeArrayOperation(cJSON *propertyJson, char **argv, int argc, int cur) {
	if(cur == argc) {
		printUsage(cur, argv, propertyJson->string);
		return 1;
	}

	for(int i = cur; i < argc; i++)
		removeStringFromJsonArray(propertyJson, argv[i]);

	return 0;
}

int arrayOperation(char *file, char *property, char **argv, int argc, int cur) {
	int ret = 1;

	typedef struct { char *name; int (*function)(); } Command;
	Command commands[] = {
		{"list", listArrayOperation},
		{"add", addArrayOperation},
		{"remove", removeArrayOperation}
	};

	size_t commandStructSize = sizeof(Command);
	size_t commandCount = sizeof(commands)/commandStructSize;

	if(cur > argc-1)
		goto cleanup;

	for(size_t i = 0; i < commandCount; i++) {
		if(strcmp(strlwr(argv[cur]), commands[i].name) == 0) {
			cJSON *fileJson = loadJson(NULL, file);
			if(!fileJson) {
				printInitMessage(file);
				ret = 1;
				goto cleanup;
			}

			cJSON *propertyJson = cJSON_GetObjectItemCaseSensitive(fileJson, property);
			ret = commands[i].function(propertyJson, argv, argc, ++cur);
			writeJson(fileJson, NULL, file);

			cJSON_Delete(fileJson);
			return ret;
		}
	}

cleanup:
	printUsageCommands(cur, argv, (void *)commands, commandCount, commandStructSize);
	return ret;
}

int init(char **argv) {
	bool generateConfig = true;
	if(fileExists(CONFIG_JSON_PATH))
		generateConfig = boolInputPrompt("'config.json' already exists, would you like to replace it?", false);

	if(generateConfig) {
		cJSON *configJson = cJSON_CreateObject();

		char *boardGenerationUrl = stringInputPrompt("Enter a board generation URL [Ex: https://example.com/ringbulletin/]", NULL);
		cJSON_AddStringToObject(configJson, "boardGenerationUrl", boardGenerationUrl);
		free(boardGenerationUrl);

		char *boardGenerationDirectory = stringInputPrompt("Enter a board generation directory", "./static/");
		cJSON_AddStringToObject(configJson, "boardGenerationDirectory", boardGenerationDirectory);
		free(boardGenerationDirectory);
		
		int defaultSearchDepth = 4;
		int searchDepth = intInputPrompt("Enter a peer search depth", &defaultSearchDepth);
		cJSON_AddNumberToObject(configJson, "searchDepth", searchDepth);
		
		char *theme = stringInputPrompt("Enter a theme", "yotsuba");
		cJSON_AddStringToObject(configJson, "theme", theme);
		free(theme);
	
		writeJson(configJson, NULL, CONFIG_JSON_PATH);
		cJSON_Delete(configJson);
	}

	bool generateBoard = true;
	if(fileExists(BOARD_JSON_PATH))
		generateBoard = boolInputPrompt("'board.json' already exists, would you like to replace it?", false);

	if(generateBoard) {
		cJSON *boardJson = cJSON_CreateObject();
		
		char *boardTitle = stringInputPrompt("Enter a board title", NULL);
		cJSON_AddStringToObject(boardJson, "title", boardTitle);
		free(boardTitle);

		cJSON_AddItemToObject(boardJson, "peers", cJSON_CreateArray());
		cJSON_AddItemToObject(boardJson, "feeds", cJSON_CreateArray());
		writeJson(boardJson, NULL, BOARD_JSON_PATH);
		cJSON_Delete(boardJson);

		char *peerArgs = stringInputPrompt("Enter peer board.json URLs separated by spaces", "");
		char *addPeerArgCommand = NULL;
		asprintf(&addPeerArgCommand, "%s peer add %s > /dev/null 2>&1", argv[0], peerArgs);
		system(addPeerArgCommand);
		free(addPeerArgCommand);
		free(peerArgs);

		char *feedArgs = stringInputPrompt("Enter RSS feed URLs to subscribe to separated by spaces", "");
		char *addFeedArgCommand = NULL;
		asprintf(&addFeedArgCommand, "%s feed add %s > /dev/null 2>&1", argv[0], feedArgs);
		system(addFeedArgCommand);
		free(addFeedArgCommand);
		free(feedArgs);
	}

	return 0;
}

int peer(char **argv, int argc, int cur) {
	return arrayOperation(BOARD_JSON_PATH, "peers", argv, argc, cur);
}

int feed(char **argv, int argc, int cur) {
	return arrayOperation(BOARD_JSON_PATH, "feeds", argv, argc, cur);
}

int setTitle(char **argv, int argc, int cur) {
	if(cur == argc) {
		printUsage(cur, argv, "title");
		return 1;
	}

	cJSON *fileJson = loadJson(NULL, BOARD_JSON_PATH);
	if(!fileJson) {
		printInitMessage(BOARD_JSON_PATH);
		return 1;
	}

	int titleLength = 0;
	for(int i = cur; i < argc; i++)
		titleLength += strlen(argv[i]) + 1;

	char title[titleLength];
	char *titleCur = title;
	for(int i = cur; i < argc; i++)
		titleCur += sprintf(titleCur, "%s ", argv[i]);

	title[titleLength-1] = '\0';

	replaceJsonValue(fileJson, "title", title, addStringToJsonItem);
	writeJson(fileJson, NULL, BOARD_JSON_PATH);

	cJSON_Delete(fileJson);
	return 0;
}

int main(int argc, char **argv) {
	if(argc == 1) {
		bool shouldRegenerate = false;
		return generateBoard(shouldRegenerate);
	}

	typedef struct { char *name; int (*function)(); } Command;
	Command commands[] = {
		{"regenerate", regenerate},
		{"init", init},
		{"peer", peer},
		{"feed", feed},
		{"set-title", setTitle},
	};

	size_t commandStructSize = sizeof(Command);
	size_t commandCount = sizeof(commands)/commandStructSize;
	int cur = 1;

	for(size_t i = 0; i < commandCount; i++) {
		if(strcmp(strlwr(argv[cur]), commands[i].name) == 0) {
			return commands[i].function(argv, argc, ++cur);
		}
	}

	printUsageCommands(cur, argv, (void *)commands, commandCount, commandStructSize);

	return 1;
}
