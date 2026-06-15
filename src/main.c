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
#include <inttypes.h>

#include <cjson/cJSON.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include "xxhash.h"

#include "fetch.h"
#include "fileutils.h"
#include "config.h"
#include "bulletin.h"
#include "context.h"
#include "stringutils.h"

#define PROGRAM_TITLE "ringbulletin"
#define CONFIG_PATH "config.json"

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

int searchedAlready(Context *ctx, const char *categoryString, const char *itemString) {
	double lastSearched;

	if (getJsonHistoryItemProperty(ctx, categoryString, itemString, "lastSearched", &lastSearched) != 0
		|| lastSearched < ctx->searchStartTime
	) {
		double now = (double)ctx->searchStartTime;
		updateJsonHistoryItemProperty(ctx, categoryString, itemString, "lastSearched", &now, addDoubleToJsonHistoryItem);
		return 0;
	}

	return 1;
}

void processFeed(char *feed, Context *ctx, char *url) {
	PostData post;
	PostData newestPost;
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
	xmlXPathContextPtr context = xmlXPathNewContext(doc);
	xmlXPathObjectPtr itemNodes = xmlXPathEvalExpression((const xmlChar *)"//item", context);

	for (int i = 0; i < itemNodes->nodesetval->nodeNr; i++) { 
		xmlNodePtr itemNode = itemNodes->nodesetval->nodeTab[i];
		for (xmlNodePtr child = itemNode->children; child != NULL; child = child->next) {
			xmlChar *element = xmlNodeListGetString(doc, child->children, 0);
			
			if (child->type == XML_ELEMENT_NODE && xmlStrcmp(child->name, (const xmlChar *)"title") == 0) {
				printf("Title: %s\n", element);
				post.title = strdup((char *)element);
			}

			else if (child->type == XML_ELEMENT_NODE && xmlStrcmp(child->name, (const xmlChar *)"link") == 0) {
				printf("Link: %s\n", element);
				post.link = strdup((char *)element);
			}

			else if (child->type == XML_ELEMENT_NODE && xmlStrcmp(child->name, (const xmlChar *)"pubDate") == 0) {
				printf("pubDateFormattedString: %s\n", element);
				post.pubDateFormattedString = strdup((char *)element);
				post.pubDateUnix = getUnixTimestampFromTimeFormatString(post.pubDateFormattedString);
			}

			else if (child->type == XML_ELEMENT_NODE && xmlStrcmp(child->name, (const xmlChar *)"description") == 0) {
				printf("description: %s\n", element);
				post.description = strdup((char *)element);
			}

			xmlFree(element);
		}

		char normalizedTitle[strlen(post.title)+1];
		strcpy(normalizedTitle, post.title);
		normalize(normalizedTitle);
	
		post.normalizedTitleHash = XXH64(normalizedTitle, strlen(normalizedTitle), 0);
		snprintf(post.normalizedTitleHashString, sizeof(post.normalizedTitleHashString)/sizeof(char), "%016" PRIx64, post.normalizedTitleHash);

		if (i == 0) newestPost = post;

		char *lastSearchedPostTitleHash;
		double lastSearchedPostDate;
		if (
			getJsonHistoryItemProperty(ctx, "feeds", url, "lastSearchedPostTitleHash", &lastSearchedPostTitleHash) != 0
			|| getJsonHistoryItemProperty(ctx, "feeds", url, "lastSearchedPostDate", &lastSearchedPostDate) != 0
			|| !(lastSearchedPostDate == post.pubDateUnix && strcmp((const char *)lastSearchedPostTitleHash, post.normalizedTitleHashString) == 0)
		) {
			writePost(&post, ctx);
		} else {	
			break;
		}
	}
	
	updateJsonHistoryItemProperty(ctx, "feeds", url, "lastSearchedPostTitleHash", newestPost.normalizedTitleHashString, addStringToJsonHistoryItem);
	double pubDateUnixDoubleHelper = (double)newestPost.pubDateUnix;
	updateJsonHistoryItemProperty(ctx, "feeds", url, "lastSearchedPostDate", &pubDateUnixDoubleHelper, addDoubleToJsonHistoryItem);
}

void searchBoard(cJSON *board, Context *ctx, int currentDepth) {
	// Scan Feeds
	cJSON *feedUrls = cJSON_GetObjectItemCaseSensitive(board, "feeds");
	cJSON *feedUrl = NULL;
	char *feed;

	if(feedUrls) {
		cJSON_ArrayForEach(feedUrl, feedUrls) {
			if(!searchedAlready(ctx, "feeds", feedUrl->valuestring)) {
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
	}

	// Scan Peers
	cJSON *peers = cJSON_GetObjectItemCaseSensitive(board, "peers");
	char *peerBoard;
	cJSON *peerBoardJson = NULL;
	cJSON *peer = NULL;	

	if(peers && ctx->config->searchDepth >= currentDepth) {
		cJSON_ArrayForEach(peer, peers) {
			if(!searchedAlready(ctx, "boards", peer->valuestring)) {
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
		
	}

	cJSON_Delete(board);
}

int main(int argc, char **argv) {
	cJSON *boardJson;
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

	// Load Config
	ConfigValues config;
	if(loadConfig(CONFIG_PATH, &config))
		return 1;

	Context ctx = { &config, time(NULL), NULL };
	int postsDirectoryPathLen = strlen(ctx.config->boardGenerationDirectory) + strlen("/posts/") + 1;
	ctx.postsDirectory = malloc(postsDirectoryPathLen * sizeof(char));
    snprintf(ctx.postsDirectory, postsDirectoryPathLen, "%s/posts/", ctx.config->boardGenerationDirectory);	
	
	// Load board file
	boardJson = loadJson(NULL, config.boardJsonPath);

	if(!boardJson) {
		printError("Unable to load board values.");
		ret = 1;
		goto cleanup;
	}

	//Remove generated files on reload
	if(reloadFlag == 1 && directoryExists(config.boardGenerationDirectory)) {
		removeFile(config.boardGenerationDirectory, "history.json");
		processFiles(ctx.postsDirectory, (void *)removeCallback, ctx.postsDirectory);
	}

	// Search Boards and Feeds
	searchedAlready(&ctx, "boards", config.boardJsonUrl);
	searchBoard(boardJson, &ctx, 0);

	writeBulletin(&ctx);

cleanup:
	free(ctx.postsDirectory);
	return ret;
}
