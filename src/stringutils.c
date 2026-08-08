#define __USE_XOPEN
#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <curl/urlapi.h>

char *strlwr(char *string) {
	char *character = string;
	while(*character != '\0') {
		*character = tolower(*character);
		character++;
	}

	return string;
}

char *removeReplyPrefix(char *string) {
	char *replyPrefix = "re:";
	int replyPrefixLength = strlen(replyPrefix);

	char *replyPrefixLocation = strstr(string, replyPrefix);
	if (!replyPrefixLocation) { 
		return string;
	}

	int indexOfOriginalTitle = (replyPrefixLocation - string) + replyPrefixLength;

	size_t stringLength = strlen(string);
	size_t i;
	for(i = indexOfOriginalTitle; i < stringLength; i++) {
		string[i - indexOfOriginalTitle] = string[i];
	}

	string[i - indexOfOriginalTitle] = '\0';

	return string;	
}

char *strip(char *string) {
	int write = 0;
	for(int read = 0; string[read] != '\0'; read++) {
		char c = string[read];
		if((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
			string[write++] = c;
	}

	string[write] = '\0';

	return string;
}

char *normalize(char *string) {
	return strip(
		removeReplyPrefix(
			strlwr(string)
		)
	);
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

time_t extractTimeFromFilename(char *filename) {
	char timeString[13];

	int i = 0;
	for(;filename[i] != '_'; i++) {
		timeString[i] = filename[i];
	}

	timeString[i] = '\0';

    return (time_t)strtoll(timeString, NULL, 10);
}

char *createTimestampedFilename(char *filename, char *seperator) {
	time_t currentTime = time(NULL);
	int timestampLength = 10;
	int sizeOfTimestampedFilename 
		= sizeof(char) * (strlen(filename) + strlen(seperator) + timestampLength + 1);	

	char *timestampedFilename = malloc(sizeOfTimestampedFilename);
	
	snprintf(timestampedFilename, sizeOfTimestampedFilename, 
		"%s%s%ld", filename, seperator, currentTime);

	return timestampedFilename;
}

time_t getUnixTimestampFromTimeFormatString(char *timeFormatString) {
    struct tm timeStructHelper = {0};

    char *timeFormats[] = {
        "%a, %d %b %Y %H:%M:%S %z",
        "%a, %d %b %Y %H:%M:%S GMT",
    };

    for(size_t i = 0; i < sizeof(timeFormats) / sizeof(char *); i++) {
        if(strptime(timeFormatString, timeFormats[i], &timeStructHelper) != NULL)
			return mktime(&timeStructHelper);
    }

    fprintf(stderr, "Failed to parse date-time.\n");
    return 1;
}

void getFormattedTimeStrForPost(time_t time, char *buffer, size_t size) {
	struct tm *localTime = localtime(&time);
	const char *weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	snprintf(buffer, size, "%02d/%02d/%02d(%s)",
		localTime->tm_mon + 1,          // month 1-12
		localTime->tm_mday,             // day
		localTime->tm_year % 100,       // year (last 2 digits)
		weekdays[localTime->tm_wday]	// weekday
	);
}
