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
#include "bulletin.h"

#define PROGRAM_TITLE "ringbulletin"
#define CONFIG_PATH "config.json"

typedef struct {
	int reload;
} Options;

typedef struct {
	ConfigValues *config;
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
	xmlXPathObjectPtr itemNodes = xmlXPathEvalExpression((const xmlChar *)"//item", context);
	xmlNodePtr itemNode;		

	for (int i=0; i < itemNodes->nodesetval->nodeNr; i++) { 
		xmlNodePtr itemNode = itemNodes->nodesetval->nodeTab[i];
		for (xmlNodePtr child = itemNode->children; child != NULL; child = child->next) {
			xmlChar *element = xmlNodeListGetString(doc, child->children, 0);
			
			if (child->type == XML_ELEMENT_NODE && xmlStrcmp(child->name, (const xmlChar *)"title") == 0) {
				printf("Title: %s\n", element);
				post.title = strdup(element);
			}

			else if (child->type == XML_ELEMENT_NODE && xmlStrcmp(child->name, (const xmlChar *)"link") == 0) {
				printf("Link: %s\n", element);
				post.link = strdup(element);
			}

			else if (child->type == XML_ELEMENT_NODE && xmlStrcmp(child->name, (const xmlChar *)"pubDate") == 0) {
				printf("pubDate: %s\n", element);
				post.pubDate = strdup(element);
			}

			xmlFree(element);
		}

		writePost(&post, ctx->config->boardGenerationDirectory);
		//xmlFree(keyword);
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

	if(peers && ctx->config->searchDepth >= currentDepth) {
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

	/*
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
			default:
				abort();
		}
	}
	*/

	// Load Config
	ConfigValues config;
	if(loadConfig(CONFIG_PATH, &config))
		return 1;

	// Load search history file
	searchHistory = loadJson(config.searchHistoryPath);

	if(!searchHistory) {
		searchHistory = cJSON_CreateObject();	
	}

	// Load board file
	boardJson = loadJson(config.boardJsonPath);

	if(!boardJson) {
		printError("Unable to load board values.");
		ret = 1;
		goto cleanup;
	}

	// Search Boards and Feeds
	Context ctx = { &config, &options, searchHistory };
	searchBoard(boardJson, &ctx, 0);

	// Write History
	if(writeJson(searchHistory, config.searchHistoryPath)) {
		printError("Unable to write history.");
		ret = 1;
		goto cleanup;
	}

	writeBulletin();

cleanup:
	cJSON_Delete(searchHistory);
	return ret;
}
