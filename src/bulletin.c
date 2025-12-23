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

#include "bulletin.h"
#include "fileutils.h"
#include "stringutils.h"

void addStyle(xmlNodePtr head, const char *stylePath) {
    xmlNodePtr link = xmlNewChild(head, NULL, BAD_CAST "link", NULL);
    xmlNewProp(link, BAD_CAST "rel", BAD_CAST "stylesheet");
    xmlNewProp(link, BAD_CAST "type", BAD_CAST "text/css");
    xmlNewProp(link, BAD_CAST "href", BAD_CAST stylePath);
}

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
		addStyle(head, "../theme.css");
		addStyle(head, "../board.css");
	
    xmlNodePtr postElement = xmlNewChild(html, NULL, BAD_CAST "body", NULL);
	xmlNewProp(postElement, BAD_CAST "class", BAD_CAST "post-inner");
		xmlNodePtr postTitle = xmlNewChild(postElement, NULL, BAD_CAST "div", NULL);
		xmlNewProp(postTitle, BAD_CAST "class", BAD_CAST "post-title");
		xmlNodePtr postLink = xmlNewChild(postTitle, NULL, BAD_CAST "a", post->title);
			xmlNewProp(postLink, BAD_CAST "href", post->link);
			xmlNewProp(postLink, BAD_CAST "target", BAD_CAST "_blank");

		
		xmlNodePtr postMeta = xmlNewChild(postElement, NULL, BAD_CAST "div", NULL);
		xmlNewProp(postMeta, BAD_CAST "class", BAD_CAST "post-meta");
			xmlNodePtr postDate = xmlNewChild(postMeta, NULL, BAD_CAST "span", post->pubDate);
			xmlNewProp(postDate, BAD_CAST "class", BAD_CAST "post-date");
			xmlNewChild(postMeta, NULL, BAD_CAST "span", BAD_CAST " • ");
			xmlNodePtr postUrl = xmlNewChild(postMeta, NULL, BAD_CAST "span", post->link);
			xmlNewProp(postUrl, BAD_CAST "class", BAD_CAST "post-url");

		xmlNodePtr description = xmlNewChild(postElement, NULL, BAD_CAST "div", post->description);
		xmlNewProp(description, BAD_CAST "class", BAD_CAST "post-description");

    xmlChar *postSerialized;
    int size = 0;
    htmlDocDumpMemoryFormat(doc, &postSerialized, &size, 1);

    if(!postSerialized) {
        printf("Post was not serialized\n");
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
		addStyle(head, "./theme.css");
		addStyle(head, "./board.css");
  		
    xmlNodePtr body = xmlNewChild(html, NULL, BAD_CAST "body", NULL);
	xmlNewProp(body, BAD_CAST "id", BAD_CAST "outer-body");
		xmlNodePtr navbar = xmlNewChild(body, NULL, BAD_CAST "div", NULL);
    	xmlNewProp(navbar, BAD_CAST "id", BAD_CAST "navbar");
			xmlNodePtr mainNavbarSection = xmlNewChild(navbar, NULL, BAD_CAST "div", "RingBulletin");
			xmlNewProp(mainNavbarSection, BAD_CAST "class", BAD_CAST "nav-main");

			xmlNodePtr rightNavbarSection = xmlNewChild(navbar, NULL, BAD_CAST "div", NULL);
			xmlNewProp(rightNavbarSection, BAD_CAST "class", BAD_CAST "nav-right");

		xmlNodePtr board = xmlNewChild(body, NULL, BAD_CAST "div", NULL);
		xmlNewProp(board, BAD_CAST "id", BAD_CAST "board");
			for (int i = 0; i < numberOfPosts; i++) {
				xmlNodePtr iframe = xmlNewChild(board, NULL, BAD_CAST "iframe", NULL);
				xmlNewProp(iframe, BAD_CAST "class", BAD_CAST "post");
				char iframePath[BASE_PATH_MAX];
				printf("%s\n", filenames[i]);
				snprintf(iframePath, sizeof(iframePath), "./posts/%s", filenames[i]);
				xmlNewProp(iframe, BAD_CAST "src", BAD_CAST iframePath);

				free(filenames[i]);
			}   
    
    xmlChar *postSerialized;
    int size = 0;
    htmlDocDumpMemoryFormat(doc, &postSerialized, &size, 1); 

    if(!postSerialized) {
        printf("Post was not serialized\n");
    }   

    writeFile((const char *)postSerialized, &size, "./static/", "board.html");
    copyFile("./assets/css/theme.css", "./static/", "theme.css");   
    copyFile("./assets/css/board.css", "./static/", "board.css");   
    
	return 0;   
}
