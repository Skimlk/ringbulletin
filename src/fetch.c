#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>

struct memory {
	char *memory;
	size_t size;
};

static size_t writeMemory(char *contents, size_t size, size_t nmemb, void *userp) {
	struct memory *mem = userp;
	char *ptr = realloc(mem->memory, mem->size + nmemb + 1);	
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
	} else {
		document = chunk.memory;
	}

	curl_easy_cleanup(handle);
	curl_global_cleanup();

	free(chunk.memory);
	
	return document;
}
