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
#include <libxml/xpath.h>

#include "fetch.h"
#include "fileutils.h"
#include "config.h"

#define PROGRAM_TITLE "ringbulletin"
#define CONFIG_PATH "config.json"

typedef struct {
	int reload;
} Options;

typedef struct {
	ConfigValues *configValues;
	Options *options;
	cJSON *searchHistory;
} Context;

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

void processFeed(char *feed, Context *ctx, char *url) {
	/*	 
		- WIP
		- Get postdata and put it in struct
		- For Each Post in Posts
			-if(!options->reload || options->reload && {postdate > lastSearched})
				WritePost();
	*/
	PostData post;
	xmlDocPtr doc;
	xmlNodePtr cur;

	// Setup Feed
	doc = xmlReadMemory(feed, strlen(feed), NULL, NULL, 0);	
	if (!doc) {
		fprintf(stderr, "Document not parsed successfully.\n");
		return;
	}
	
	cur = xmlDocGetRootElement(doc);

	if (cur == NULL) {
		fprintf(stderr,"empty document\n");
		xmlFreeDoc(doc);
		return;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) "rss")) {
		fprintf(stderr,"document of the wrong type, root node != rss\n");
		xmlFreeDoc(doc);
		return;
	}

	// Iterate Through Posts (Testing)
	xmlChar *keyword;
	xmlXPathContextPtr context = xmlXPathNewContext(doc);
	xmlXPathObjectPtr result = xmlXPathEvalExpression((const xmlChar *)"//item/title", context);
	for (int i=0; i < result->nodesetval->nodeNr; i++) { 
		keyword = xmlNodeListGetString(doc, result->nodesetval->nodeTab[i]->xmlChildrenNode, 0);
		printf("keyword: %s\n", keyword);
		post.title = keyword;
		writePost(&post, ctx->configValues->boardGenerationDirectory);
		xmlFree(keyword);
	}
}

void searchBoard(cJSON *board, Context *ctx, int currentDepth) {
	// Scan Feeds
	cJSON *feedUrls = cJSON_GetObjectItemCaseSensitive(board, "feeds");
	cJSON *feedUrl = NULL;
	char *feed;

	if(feedUrls) {
		cJSON_ArrayForEach(feedUrl, feedUrls) {
			feed = fetch(feedUrl->valuestring);
			if(feed) {
				printf("Fetched feed at '%s'.\n", feedUrl->valuestring);
				processFeed(feed, ctx, feedUrl->valuestring);
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

	if(peers && ctx->configValues->searchDepth >= currentDepth) {
		cJSON_ArrayForEach(peer, peers) {
			peerBoard = fetch(peer->valuestring);
			peerBoardJson = cJSON_Parse(peerBoard);
			free(peerBoard);
			if(peerBoardJson) {
				searchBoard(peerBoardJson, ctx, currentDepth+1);
			} else {
				printf("Couldn't fetch board at '%s'.\n", peer->valuestring);
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
	ConfigValues configValues;
	if(loadConfig(CONFIG_PATH, &configValues))
		return 1;

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
	Context ctx = { &configValues, &options, searchHistory };
	searchBoard(boardJson, &ctx, 0);

	// Write History
	if(writeJson(searchHistory, configValues.searchHistoryPath)) {
		printError("Unable to write history.");
		ret = 1;
		goto cleanup;
	}

cleanup:
	cJSON_Delete(searchHistory);
	return ret;
}
