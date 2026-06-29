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

int feedIsValid(xmlDocPtr doc) {
	xmlNodePtr root = xmlDocGetRootElement(doc);

	if (root == NULL) {
		fprintf(stderr, "Document is empty\n");
		return 0;
	}

	if (xmlStrcmp(root->name, BAD_CAST "rss")) {
		fprintf(stderr, "Document is of the wrong type, the root node is not rss\n");
		return 0;	
	}

	return 1;
}

int postAlreadyWritten(PostData *post, char *url, Context *ctx) {
	int ret = 1;
	char *lastSearchedPostTitleHash = NULL;
	double lastSearchedPostDate;
	
	if (getJsonHistoryItemProperty(ctx, "feeds", url, "lastSearchedPostTitleHash", &lastSearchedPostTitleHash) != 0) {
		ret = 0;
		goto cleanup;
	}

	if (getJsonHistoryItemProperty(ctx, "feeds", url, "lastSearchedPostDate", &lastSearchedPostDate) != 0) {
		ret = 0;
		goto cleanup;
	}
	
	int datesMatch = (lastSearchedPostDate == post->pubDateUnix) ? 1 : 0;
	int normalizedTitleHashesMatch
		= (strcmp((const char *)lastSearchedPostTitleHash, post->normalizedTitleHashString) == 0) ? 1 : 0;
	
	if (!(datesMatch && normalizedTitleHashesMatch))
		ret = 0;

cleanup:
	free(lastSearchedPostTitleHash);
	return ret;
}

int hydratePostContent(xmlDocPtr doc, xmlNodePtr postNode, PostData *post) {
	int ret = 0;
	xmlChar *element = NULL;

	if (postNode == NULL || postNode->type != XML_ELEMENT_NODE) {
		ret = 1;
		goto cleanup;
	}
	
	element = xmlNodeListGetString(doc, postNode->children, 0);
	if (element == NULL) {
		ret = 1;
		goto cleanup;
	}

	if (xmlStrcmp(postNode->name, BAD_CAST "title") == 0) {
		printf("Title: %s\n", element);
		post->title = strdup((char *)element);
	}

	else if (xmlStrcmp(postNode->name, BAD_CAST "link") == 0) {
		printf("Link: %s\n", element);
		post->link = strdup((char *)element);
	}

	else if (xmlStrcmp(postNode->name, BAD_CAST "pubDate") == 0) {
		printf("pubDateFormattedString: %s\n", element);
		post->pubDateFormattedString = strdup((char *)element);
		post->pubDateUnix = getUnixTimestampFromTimeFormatString(post->pubDateFormattedString);
	}

	else if (xmlStrcmp(postNode->name, BAD_CAST "description") == 0) {
		printf("description: %s\n", element);
		post->description = strdup((char *)element);
	}

cleanup:
	xmlFree(element);
	return ret;
}

int processFeed(char *feed, Context *ctx, char *url) {
	int ret = 0;
	xmlDocPtr doc = xmlReadMemory(feed, strlen(feed), NULL, NULL, 0);
	PostData *latestPost = initalizePost();
	xmlXPathContextPtr docXPathContext = NULL;
	xmlXPathObjectPtr itemNodes = NULL;

	if (!doc) {
		fprintf(stderr, "Document not parsed successfully.\n");
		ret = 1;
		goto cleanup;
	}

	if (!feedIsValid(doc)) {
		ret = 1;
		goto cleanup;
	}

	// Iterate Through Posts (Testing)
	docXPathContext = xmlXPathNewContext(doc);
	itemNodes = xmlXPathEvalExpression(BAD_CAST "//item", docXPathContext);

	if (itemNodes == NULL || itemNodes->nodesetval == NULL) {
		ret = 1;
		goto cleanup;
	}

	for (int i = 0; i < itemNodes->nodesetval->nodeNr; i++) {
		PostData *post = initalizePost();
		xmlNodePtr itemNode = itemNodes->nodesetval->nodeTab[i];
		for (xmlNodePtr child = itemNode->children; child != NULL; child = child->next)
			hydratePostContent(doc, child, post);
		char normalizedTitle[strlen(post->title)+1];
		strcpy(normalizedTitle, post->title);
		normalize(normalizedTitle);
	
		post->normalizedTitleHash = XXH64(normalizedTitle, strlen(normalizedTitle), 0);
		int hashMaxLength = 17;
		post->normalizedTitleHashString = malloc(sizeof(char) * hashMaxLength);
		snprintf(post->normalizedTitleHashString, hashMaxLength, "%016" PRIx64, post->normalizedTitleHash);

		if (i == 0) copyPostData(latestPost, post);
	
		if (!postAlreadyWritten(post, url, ctx)) {
			writePost(post, ctx);
			freePostData(post);
		}
		else {
			freePostData(post);
			break;
		}
	}

	updateJsonHistoryItemProperty(ctx, "feeds", url, "lastSearchedPostTitleHash", latestPost->normalizedTitleHashString, addStringToJsonHistoryItem);
	double pubDateUnixDoubleHelper = (double)latestPost->pubDateUnix;
	updateJsonHistoryItemProperty(ctx, "feeds", url, "lastSearchedPostDate", &pubDateUnixDoubleHelper, addDoubleToJsonHistoryItem);

cleanup:
	xmlFreeDoc(doc);
	xmlXPathFreeContext(docXPathContext);
	xmlXPathFreeObject(itemNodes);
	freePostData(latestPost);
	return ret;
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
					cJSON_Delete(peerBoardJson);
				} else {
					printf("Couldn't fetch board at '%s'.\n", peer->valuestring);
				}
			}
		}
	}
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

	Context ctx = { &config, time(NULL), NULL, NULL };
	ctx.postsDirectoryRelativePath = "posts/";
	asprintf(&ctx.postsDirectoryFullPath, "%s/%s",
		config.boardGenerationDirectory, ctx.postsDirectoryRelativePath);
	
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
		processFiles(ctx.postsDirectoryFullPath, (void *)removeCallback, ctx.postsDirectoryFullPath);
	}

	// Search Boards and Feeds
	searchedAlready(&ctx, "boards", config.boardJsonUrl);
	searchBoard(boardJson, &ctx, 0);

	writeBulletin(&ctx);

cleanup:
	free(ctx.postsDirectoryFullPath);
	cJSON_Delete(boardJson);
	return ret;
}
