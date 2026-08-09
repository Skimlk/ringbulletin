#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <curl/urlapi.h>

#include "crawler.h"
#include "feed.h"
#include "json.h"

struct memory {
	char *memory;
	size_t size;
};

static size_t writeMemory(char *contents, size_t size, size_t nmemb, void *userp) {
	struct memory *mem = userp;
	char *ptr = realloc(mem->memory, mem->size + size * nmemb + 1);	
	if(!ptr) {
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}
	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, nmemb);
	mem->size += nmemb;
	mem->memory[mem->size] = 0;
	return nmemb;	
}

char *fetch(char *URL) {
	struct memory chunk = {malloc(1), 0};

	char *document = NULL;
	CURLcode res;
	curl_global_init(CURL_GLOBAL_ALL);
	CURL *handle = curl_easy_init();

	curl_easy_setopt(handle, CURLOPT_URL, URL);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeMemory);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &chunk);
	
	res = curl_easy_perform(handle);

	if(res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
		free(chunk.memory);
	} else {
		document = chunk.memory;
	}

	curl_easy_cleanup(handle);
	curl_global_cleanup();

	return document;
}

char *getDomainFromLink(const char *link) {
	if (!link || *link == '\0')
		return NULL;

	CURLU *urlHandle = curl_url();
	if (!urlHandle)
		return NULL;

	char *host = NULL;
	char *domain = NULL;

	if (
		curl_url_set(urlHandle, CURLUPART_URL, link, 0) == CURLUE_OK &&
		curl_url_get(urlHandle, CURLUPART_HOST, &host, 0) == CURLUE_OK &&
		host[0] != '\0'
	)
		domain = strdup(host);

	curl_free(host);
	curl_url_cleanup(urlHandle);
	return domain;
}

int normalizeUrl(char **urlPtr) {
	if (!urlPtr || !*urlPtr) {
	        return 1;
	    }

	    CURLU *urlHandle = curl_url();
	    if (!urlHandle) {
	        return 1;
	    }

	    CURLUcode curlResultCode;
	    char *normalizedUrl = NULL;
	    char *path = NULL;

	    char *fragment = strchr(*urlPtr, '#');
	    if (fragment) {
	        *fragment = '\0';
	    }

	    curlResultCode = curl_url_set(urlHandle, CURLUPART_URL, *urlPtr, 0);
	    if (curlResultCode != CURLUE_OK) {
	        curl_url_cleanup(urlHandle);
	        return 1;
	    }

	    curl_url_set(urlHandle, CURLUPART_FRAGMENT, NULL, 0);

	    if (curl_url_get(urlHandle, CURLUPART_PATH, &path, 0) == CURLUE_OK && path) {
	        char *src = path;
	        char *dst = path;
	        while (*src) {
	            *dst++ = *src;
	            if (*src == '/') {
	                while (*(src + 1) == '/') {
	                    src++;
	                }
	            }
	            src++;
	        }
	        *dst = '\0';
	        curl_url_set(urlHandle, CURLUPART_PATH, path, 0);
	        curl_free(path);
	    }

	    curlResultCode = curl_url_get(urlHandle, CURLUPART_URL, &normalizedUrl, 0);
	    curl_url_cleanup(urlHandle);

	    if (curlResultCode != CURLUE_OK) {
	        return 1;
	    }

	    free(*urlPtr);
	    *urlPtr = normalizedUrl;

	    return 0;
}

int searchedAlready(Context *ctx, const char *categoryString, const char *itemString) {
	double lastSearched;

	if (getJsonHistoryItemProperty(categoryString, itemString, "lastSearched", &lastSearched) != 0
		|| lastSearched < ctx->searchStartTime
	) {
		double now = (double)ctx->searchStartTime;
		updateJsonHistoryItemProperty(categoryString, itemString, "lastSearched", &now, addDoubleToJsonHistoryItem);
		return 0;
	}

	return 1;
}

void searchBoard(cJSON *board, Context *ctx, int currentDepth) {
	// Scan Feeds
	cJSON *feedUrls = cJSON_GetObjectItemCaseSensitive(board, "feeds");
	cJSON *feedUrl = NULL;
	char *feed;

	if(feedUrls) {
		cJSON_ArrayForEach(feedUrl, feedUrls) {
			normalizeUrl(&feedUrl->valuestring);
			if(feedUrl != NULL && !searchedAlready(ctx, "feeds", feedUrl->valuestring)) {
				feed = fetch(feedUrl->valuestring);
				if(feed) {
					printf("Fetched and processing feed at '%s'.\n", feedUrl->valuestring);
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
			normalizeUrl(&peer->valuestring);
			if(peer != NULL && !searchedAlready(ctx, "boards", peer->valuestring)) {
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
