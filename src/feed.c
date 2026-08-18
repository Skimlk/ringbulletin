#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include "xxhash.h"

#include "feed.h"
#include "crawler.h"
#include "json.h"
#include "post.h"
#include "stringutils.h"
#include "thread.h"
#include "timeutils.h"

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

int postAlreadyWritten(PostData *post, char *url) {
	int ret = 1;
	char *lastSearchedPostTitleHash = NULL;
	double lastSearchedPostDate;
	
	if (getJsonHistoryItemProperty("feeds", url, "lastSearchedPostTitleHash", &lastSearchedPostTitleHash) != 0) {
		ret = 0;
		goto cleanup;
	}

	if (getJsonHistoryItemProperty("feeds", url, "lastSearchedPostDate", &lastSearchedPostDate) != 0) {
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
		post->title = strdup((char *)element);
	}

	else if (xmlStrcmp(postNode->name, BAD_CAST "link") == 0) {
		post->link = strdup((char *)element);
		post->domain = getDomainFromLink(post->link);
	}

	else if (xmlStrcmp(postNode->name, BAD_CAST "pubDate") == 0) {
		post->pubDateUnix = getUnixTimestampFromTimeFormatString((char *) element);
		int pubDateFormattedStringChars = 32;
		post->pubDateFormattedString = malloc(sizeof(char) * pubDateFormattedStringChars);
		getFormattedTimeStrForPost(post->pubDateUnix, post->pubDateFormattedString, pubDateFormattedStringChars);
	}

	else if (xmlStrcmp(postNode->name, BAD_CAST "description") == 0) {
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

		post->iconPath = strdup("../icons/default-icon.ico");

		if (i == 0) copyPostData(latestPost, post);
	
		if (!postAlreadyWritten(post, url)) {
			processPost(post, ctx);
			freePostData(post);
		}
		else {
			freePostData(post);
			break;
		}
	}

	updateJsonHistoryItemProperty("feeds", url, "lastSearchedPostTitleHash", latestPost->normalizedTitleHashString, addStringToJsonItem);
	double pubDateUnixDoubleHelper = (double)latestPost->pubDateUnix;
	updateJsonHistoryItemProperty("feeds", url, "lastSearchedPostDate", &pubDateUnixDoubleHelper, addDoubleToJsonItem);

cleanup:
	xmlXPathFreeObject(itemNodes);
	xmlXPathFreeContext(docXPathContext);
	xmlFreeDoc(doc);
	freePostData(latestPost);
	return ret;
}
