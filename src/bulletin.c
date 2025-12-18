#define __USE_XOPEN
#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <stdint.h>
#include <dirent.h> 

#include <libxml/HTMLtree.h>
#include "xxhash.h"

#include "fileutils.h"
#include "stringutils.h"

int writePost(const PostData *post, const char *directory) {
    char normalizedTitle[strlen(post->title)+1];
    strcpy(normalizedTitle, post->title);
    normalize(normalizedTitle);

    XXH64_hash_t titleHash = XXH64(normalizedTitle, strlen(normalizedTitle), 0);

    struct tm tm = {0};

    if (strptime(post->pubDate, "%a, %d %b %Y %H:%M:%S %z", &tm) == NULL
        && strptime(post->pubDate, "%a, %d %b %Y %H:%M:%S GMT", &tm) == NULL
    ) {
        fprintf(stderr, "Failed to parse date-time.\n");
        return 1;
    }

    time_t now = mktime(&tm);

    char filename[64];

    snprintf(filename, sizeof(filename), "%ld_%016llu.html",
         (long)now, (unsigned long long)titleHash);

    htmlDocPtr doc = htmlNewDoc(BAD_CAST "http://www.w3.org/TR/html4/strict.dtd", BAD_CAST "HTML");

    xmlNodePtr html = xmlNewNode(NULL, BAD_CAST "html");
    xmlDocSetRootElement(doc, html);

    xmlNodePtr head = xmlNewChild(html, NULL, BAD_CAST "head", NULL);
    
	xmlNodePtr link = xmlNewChild(head, NULL, BAD_CAST "link", NULL);
    xmlNewProp(link, BAD_CAST "rel", BAD_CAST "stylesheet");
	xmlNewProp(link, BAD_CAST "type", BAD_CAST "text/css");
    xmlNewProp(link, BAD_CAST "href", BAD_CAST "../style.css");

    xmlNodePtr body = xmlNewChild(html, NULL, BAD_CAST "body", NULL);
    xmlNewChild(body, NULL, BAD_CAST "h2", BAD_CAST post->title);
    xmlNewChild(body, NULL, BAD_CAST "p", BAD_CAST post->link);

    xmlChar *postSerialized;
    int size = 0;
    htmlDocDumpMemoryFormat(doc, &postSerialized, &size, 1);

    if(!postSerialized) {
        printf("postSerialized is empthy\n");
    }

    char postsDirectory[PATH_MAX];
    snprintf(postsDirectory, sizeof(postsDirectory), "%s/posts/", directory);

    writeFile((const char *)postSerialized, &size, postsDirectory, filename);

    return 0;
}

int writeBulletin() {
	int numberOfPosts = 0;
    processFiles("./static/posts", (void *)count, &numberOfPosts);

    char *filenames[numberOfPosts];
    processFiles("./static/posts", (void *)populateFilenamesArray, filenames);
    qsort(filenames, numberOfPosts, sizeof(char *), compare);

	htmlDocPtr doc = htmlNewDoc(BAD_CAST "http://www.w3.org/TR/html4/strict.dtd", BAD_CAST "HTML");
	xmlNodePtr html = xmlNewNode(NULL, BAD_CAST "html");
	xmlDocSetRootElement(doc, html);

	xmlNodePtr head = xmlNewChild(html, NULL, BAD_CAST "head", NULL);

	xmlNodePtr link = xmlNewChild(head, NULL, BAD_CAST "link", NULL);
	xmlNewProp(link, BAD_CAST "rel", BAD_CAST "stylesheet");
	xmlNewProp(link, BAD_CAST "type", BAD_CAST "text/css");
	xmlNewProp(link, BAD_CAST "href", BAD_CAST "./style.css");
	
	xmlNodePtr body = xmlNewChild(html, NULL, BAD_CAST "body", NULL);

	xmlNodePtr navbar = xmlNewChild(body, NULL, BAD_CAST "div", NULL);
	xmlNewProp(navbar, BAD_CAST "id", BAD_CAST "navbar");
	xmlNodePtr title = xmlNewChild(navbar, NULL, BAD_CAST "h2", "Test Board");

    for (int i = 0; i < numberOfPosts; i++) {
        xmlNodePtr iframe = xmlNewChild(body, NULL, BAD_CAST "iframe", NULL);
        char iframePath[BASE_PATH_MAX];
        printf("%s\n", filenames[i]);
        snprintf(iframePath, sizeof(iframePath), "./posts/%s", filenames[i]);
        xmlNewProp(iframe, BAD_CAST "src", BAD_CAST iframePath);
        xmlNewChild(body, NULL, BAD_CAST "br", NULL);

        free(filenames[i]);
    }   
    
    xmlChar *postSerialized;
    int size = 0;
    htmlDocDumpMemoryFormat(doc, &postSerialized, &size, 1); 

    if(!postSerialized) {
        printf("Post was not serialized\n");
    }   

    writeFile((const char *)postSerialized, &size, "./static/", "board.html");
	copyFile("./assets/css/style.css", "./static/", "style.css");	

    return 0;	
}
