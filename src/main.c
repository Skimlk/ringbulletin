/*
	Ring Bulletin:
		- Monitors selected RSS feeds for intent to participate in bulletin board
		- Creates/edits HTML page based on participation
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <cjson/cJSON.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "fetch.h"
#include "jsonutils.h"
#include "config.h"

#define PROGRAM_TITLE "ringbulletin"
#define CONFIG_PATH "config.json"

typedef struct {
	int reload;
} Options;

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

void searchBoard(cJSON *board, cJSON *searchHistory, ConfigValues *configValues, Options *options, int currentDepth) {
	// Scan Feeds
	cJSON *feedUrls = cJSON_GetObjectItemCaseSensitive(board, "feeds");
	cJSON *feedUrl = NULL;
	char *feed;

	if(feedUrls) {
		cJSON_ArrayForEach(feedUrl, feedUrls) {
			feed = fetch(feedUrl->valuestring);
			if(feed) {
				printf("Fetched feed at '%s'.\n", feedUrl->valuestring);
				/* Iterate through feed */
				free(feed);
			} else {
				printf("Couldn't fetch feed at '%s'.\n", feedUrl->valuestring);
			}
		}
	}

	// Scan Peers
	cJSON *peers = cJSON_GetObjectItemCaseSensitive(board, "peers");
	char *peerBoard;
	cJSON *peerBoardJson = NULL;
	cJSON *peer = NULL;	

	if(peers && configValues->searchDepth >= currentDepth) {
		cJSON_ArrayForEach(peer, peers) {
			peerBoard = fetch(peer->valuestring);
			peerBoardJson = cJSON_Parse(peerBoard);
			free(peerBoard);
			if(peerBoardJson) {
				searchBoard(peerBoardJson, searchHistory, configValues, options, currentDepth+1);
			} else {
				printf("Couldn't fetch feed at '%s'.\n", peer->valuestring);
			}
		}
		
	}

	cJSON_Delete(board);
}

int main(int argc, char **argv) {
	cJSON *configJson;
	cJSON *boardJson;
	cJSON *searchHistory;
	int ret = 0;
	Options options = {0};
	int c = 0;
	int reloadFlag = 0;
	while((c = getopt(argc, argv, "rh")) != -1) {
		switch(c) {
			case 'r':
				reloadFlag = 1;
				break;
			case 'h':
				printHelpText();
				return 0;
			defualt:
				abort();
		}
	}

	// Load Config
	configJson = loadJson(CONFIG_PATH);
	
	if(!configJson) {
		ret = 1;
		goto cleanup;
	}

	ConfigValues configValues;
	if(loadConfigValues(configJson, &configValues)) {
		printError("Unable to load config values.");
		ret = 1;
		goto cleanup;
	}

	// Load search history file
	searchHistory = loadJson(configValues.searchHistoryPath);

	if(!searchHistory) {
		searchHistory = cJSON_CreateObject();	
	}

	// Load board file
	boardJson = loadJson(configValues.boardJsonPath);

	if(!boardJson) {
		printError("Unable to load board values.");
		ret = 1;
		goto cleanup;
	}

	// Search Boards and Feeds
	searchBoard(loadJson(configValues.boardJsonPath), searchHistory, &configValues, &options, 0);

cleanup:
	cJSON_Delete(configJson);
	cJSON_Delete(boardJson);
	cJSON_Delete(searchHistory);
	return ret;
}
