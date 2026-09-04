#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <curl/urlapi.h>

#include "crawler.h"
#include "feed.h"
#include "json.h"

static size_t writeString(char *contents, size_t size, size_t nmemb, void *userp) {
	Memory *mem = userp;
	size_t total = size * nmemb;

	char *ptr = realloc(mem->data, mem->size + total + 1);	
	if(!ptr) {
		printf("Not enough memory.\n");
		return 0;
	}

	mem->data = ptr;
	memcpy(&(mem->data[mem->size]), contents, total);
	mem->size += total;
	mem->data[mem->size] = 0;

	return total;	
}

static size_t writeBinary(char *contents, size_t size, size_t nmemb, void *userp) {
	Memory *mem = userp;
	size_t total = size * nmemb;

	char *ptr = realloc(mem->data, mem->size + total);	
	if(!ptr) {
		printf("Not enough memory.\n");
		return 0;
	}

	mem->data = ptr;
	memcpy(&(mem->data[mem->size]), contents, total);
	mem->size += total;
	
	return total;
}

Memory *curl(char *URL, size_t (*writeCallback)(char *, size_t, size_t, void *)) {
	Memory *chunk = malloc(sizeof(Memory));
	chunk->data = malloc(1);
	chunk->size = 0;

	CURLcode res;
	curl_global_init(CURL_GLOBAL_ALL);
	CURL *handle = curl_easy_init();

	curl_easy_setopt(handle, CURLOPT_URL, URL);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeCallback);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, chunk);
	curl_easy_setopt(handle, CURLOPT_PROTOCOLS_STR, "http,https");
	curl_easy_setopt(handle, CURLOPT_REDIR_PROTOCOLS_STR, "http,https");

	res = curl_easy_perform(handle);
	if(res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
		free(chunk->data);
		free(chunk);
		chunk = NULL;
	}

	curl_easy_cleanup(handle);
	curl_global_cleanup();

	return chunk;
}

char *fetch(char *URL) { 
	Memory *result = curl(URL, writeString);
	if(result != NULL) {
		char *content = strdup(result->data);
		free(result);
		return content;
	}
	return NULL;
}

//https://github.com/curl/curl/blob/f190aa8ecea728e6ce7fb7a6250e03df4d4eaca5/src/tool_cb_wrt.c#L313
bool isBinary(Memory *memory) {
	return memchr(memory->data, 0, memory->size) != NULL;
}

bool isIcon(Memory *memory) {
	if( /* File signature for an ICO file */
		memory->size >= 4 &&
		memory->data[0] == 0x00 && 
		memory->data[1] == 0x00 && 
        memory->data[2] == 0x01 && 
		memory->data[3] == 0x00
	) {
        return true;
    }

	return false;
}

Memory *fetchIcon(char *URL) {
	Memory *binary = curl(URL, writeBinary);
	if(binary == NULL)
		return NULL;

	if(!isBinary(binary) || !isIcon(binary)) {
		free(binary->data);
		free(binary);
		return NULL;
	}

	return binary;
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

char *getBaseUrlFromLink(const char *link) {
    if (!link || *link == '\0')
        return NULL;

    CURLU *urlHandle = curl_url();
    if (!urlHandle)
        return NULL;

    char *scheme = NULL;
    char *host = NULL;
    char *port = NULL;
    char *baseUrl = NULL;

    if (
        curl_url_set(urlHandle, CURLUPART_URL, link, 0) == CURLUE_OK &&
        curl_url_get(urlHandle, CURLUPART_SCHEME, &scheme, 0) == CURLUE_OK &&
        curl_url_get(urlHandle, CURLUPART_HOST, &host, 0) == CURLUE_OK &&
        scheme[0] != '\0' &&
        host[0] != '\0'
    ) {
        curl_url_get(urlHandle, CURLUPART_PORT, &port, 0);
        size_t len = strlen(scheme) + strlen(host) + 4; 
        
        if (port) {
            len += strlen(port) + 1;
        }

        baseUrl = malloc(len);

        if (baseUrl) {
            if (port) {
                snprintf(baseUrl, len, "%s://%s:%s", scheme, host, port);
            } else {
                snprintf(baseUrl, len, "%s://%s", scheme, host);
            }
        }
    }

    curl_free(scheme);
    curl_free(host);
    curl_free(port);
    curl_url_cleanup(urlHandle);

    return baseUrl;
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
		updateJsonHistoryItemProperty(categoryString, itemString, "lastSearched", &now, addDoubleToJsonItem);
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
