#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <stdint.h>
#include <dirent.h> 

#include <libxml/HTMLtree.h>
#include <libxml/xpath.h>
#include "xxhash.h"

#include "bulletin.h"
#include "fileutils.h"
#include "stringutils.h"
#include "context.h"
#include "xmlutils.h"

int writePost(const PostData *post, Context *ctx) {
    // Normalize and Hash Title
    char normalizedTitle[strlen(post->title)+1];
    strcpy(normalizedTitle, post->title);
    normalize(normalizedTitle);
 
    XXH64_hash_t titleHash = XXH64(normalizedTitle, strlen(normalizedTitle), 0);

    htmlDocPtr doc = htmlNewDoc(BAD_CAST "http://www.w3.org/TR/html4/strict.dtd", BAD_CAST "HTML");
    xmlNodePtr html = xmlNewNode(NULL, BAD_CAST "html");
    xmlDocSetRootElement(doc, html);

	xmlNodePtr head = xmlNewChild(html, NULL, BAD_CAST "head", NULL);
		addStyle(head, "../theme.css");
		addStyle(head, "../board.css");

    xmlNodePtr body = xmlNewChild(html, NULL, BAD_CAST "body", NULL);
    xmlNodePtr postElement = addElement(body, "div", NULL, NULL, "post-inner");
		xmlNodePtr postHeader = addElement(postElement, "div", NULL, NULL, NULL);
            xmlNodePtr postTitle = addElement(postHeader, "div", NULL, NULL, "post-title"); 
                xmlNodePtr postLink = xmlNewChild(postTitle, NULL, BAD_CAST "a", post->title);
                    xmlNewProp(postLink, BAD_CAST "href", post->link);
                    xmlNewProp(postLink, BAD_CAST "target", BAD_CAST "_blank");
                
                xmlNodePtr titleViewSeparator = xmlNewChild(postTitle, NULL, BAD_CAST "span", " • ");

                xmlNodePtr viewThreadLink = xmlNewChild(postTitle, NULL, BAD_CAST "a", BAD_CAST "View Thread");
                    xmlNewProp(viewThreadLink, BAD_CAST "href", BAD_CAST "");
                    xmlNewProp(viewThreadLink, BAD_CAST "target", BAD_CAST "list-iframe");

        xmlNodePtr postMeta = addElement(postElement, "div", NULL, NULL, "post-meta");
			addElement(postMeta, "span", post->pubDate, NULL, "post-date");
			xmlNewChild(postMeta, NULL, BAD_CAST "span", BAD_CAST " • ");
			addElement(postMeta, "span", post->link, NULL, "post-url");

        xmlNodePtr description = addElement(postElement, "div", post->description, NULL, "post-description");

    xmlNodePtr replies = addElement(body, "div", NULL, NULL, "replies");
   
    char postsDirectory[PATH_MAX];
    snprintf(postsDirectory, sizeof(postsDirectory), "%s/posts/", ctx->config->boardGenerationDirectory);

	char titleHashString[40];
	snprintf(titleHashString, sizeof(titleHashString), "%lu", titleHash);

    time_t pubDateUnixTimestamp = getUnixTimestampFromTimeFormatString(post->pubDate);

    char filename[64];

    Files *existingPostsWithHash = getFilesMatchingPattern("./static/posts", contains, titleHashString);
    printf("Number of files: %d", existingPostsWithHash->numberOfFiles);
    for(int i = 0; i < existingPostsWithHash->numberOfFiles; i++) {
        char *existingPostWithHashTime = extractTimeFromFilename(existingPostsWithHash->filenames[i]);
        
        //Add feed to history
        char *replyFile = readFile("./static/posts/", existingPostsWithHash->filenames[i]);
        htmlDocPtr replyDoc = htmlReadMemory(replyFile, strlen(replyFile), NULL, "UTF-8", 0);
        xmlXPathContextPtr ctx = xmlXPathNewContext(replyDoc); 
        
        //If the existing post has a later date, add the existing post content
            //to this post and overwrite the existing post with this one
        if(strcmp(post->pubDate, existingPostWithHashTime) < 0) {
            xmlXPathObjectPtr innerPost = xmlXPathEvalExpression(
                BAD_CAST "//*[@class='post-inner']",
                ctx
            );

            xmlNodePtr sameHashPost = innerPost->nodesetval->nodeTab[0];
            xmlUnsetProp(sameHashPost, BAD_CAST "class");
            xmlNewProp(sameHashPost, BAD_CAST "class", BAD_CAST "reply");

            xmlNodePtr imported = xmlDocCopyNode(sameHashPost, replies->doc, 1);

            xmlAddChild(replies, imported);

            removeFile("./static/posts/", existingPostsWithHash->filenames[i]);
        }
        //If the existing post has an earlier date, add this post's content
            //to the existing post
        else {
            xmlXPathObjectPtr matchingHashReplies = xmlXPathEvalExpression(
                BAD_CAST "//*[@class='replies']",
                ctx
            );

            xmlNodePtr repliesNode = matchingHashReplies->nodesetval->nodeTab[0];

            xmlUnsetProp(postElement, BAD_CAST "class");
            xmlNewProp(postElement, BAD_CAST "class", BAD_CAST "reply");

            xmlNodePtr imported = xmlDocCopyNode(postElement, replyDoc, 1);
            xmlAddChild(repliesNode, imported);

            xmlChar *buffer = NULL;
            int size = 0;
            
            htmlDocDumpMemoryFormat(replyDoc, &buffer, &size, 1);

            snprintf(filename, sizeof(filename), "%s_%016llu.html",
                existingPostWithHashTime, (unsigned long long)titleHash);
            
            writeFile((const char *)buffer, &size, "./static/posts/", filename);
                        
            return 0;
        }
    }

    snprintf(filename, sizeof(filename), "%ld_%016llu.html",
        (long)pubDateUnixTimestamp, (unsigned long long)titleHash);

    xmlChar *postSerialized;
    int size = 0; 
    
    htmlDocDumpMemoryFormat(doc, &postSerialized, &size, 1);

    if(!postSerialized) {
        printf("Post was not serialized\n");
        return 1;
    }

    writeFile((const char *)postSerialized, &size, postsDirectory, filename);

    return 0;
}

int writeList() {
    Files *posts = getFiles("./static/posts");
    qsort(posts->filenames, posts->numberOfFiles, sizeof(char *), compare);
   
    htmlDocPtr doc = htmlNewDoc(BAD_CAST "http://www.w3.org/TR/html4/strict.dtd", BAD_CAST "HTML");
    xmlNodePtr html = xmlNewNode(NULL, BAD_CAST "html");
    xmlDocSetRootElement(doc, html);

	xmlNodePtr head = xmlNewChild(html, NULL, BAD_CAST "head", NULL);
		addStyle(head, "./theme.css");
		addStyle(head, "./board.css");

    xmlNodePtr body = xmlNewChild(html, NULL, BAD_CAST "body", NULL);
    xmlNodePtr list = addElement(body, "div", NULL, "board", NULL);
        for(int i = 0; i < posts->numberOfFiles; i++) {
            char *fileContents = readFile("./static/posts/", posts->filenames[i]);
            htmlDocPtr fileDocPtr = htmlReadMemory(fileContents, strlen(fileContents), NULL, "UTF-8", 0);
            xmlXPathContextPtr ctx = xmlXPathNewContext(fileDocPtr);
            xmlXPathObjectPtr replies = xmlXPathEvalExpression(
                (xmlChar*)"//*[@class='reply']",
                ctx
            );

            char replyVariableString[256];
            sprintf(replyVariableString, "--replies: %d;", replies->nodesetval->nodeNr);

            xmlNodePtr iframe = addElement(list, "iframe", NULL, NULL, "post");
                xmlNewProp(iframe, BAD_CAST "style", replyVariableString);
            char iframePath[BASE_PATH_MAX];
            printf("%s\n", posts->filenames[i]);
            snprintf(iframePath, sizeof(iframePath), "./posts/%s", posts->filenames[i]);
            char *iframePathWithTimestamp = createTimestampedFilename(iframePath, "?");
            xmlNewProp(iframe, BAD_CAST "src", BAD_CAST iframePathWithTimestamp);
            free(iframePathWithTimestamp);

            free(posts->filenames[i]);
        }
    
    xmlChar *postSerialized;
    int size = 0;
    htmlDocDumpMemoryFormat(doc, &postSerialized, &size, 1); 

    if(!postSerialized) {
        printf("Post was not serialized\n");
    }   

    writeFile((const char *)postSerialized, &size, "./static/", BAD_CAST "./list.html");
}

int writeBulletin() {
    htmlDocPtr doc = htmlNewDoc(BAD_CAST "http://www.w3.org/TR/html4/strict.dtd", BAD_CAST "HTML");
    xmlNodePtr html = xmlNewNode(NULL, BAD_CAST "html");
    xmlDocSetRootElement(doc, html);

	xmlNodePtr head = xmlNewChild(html, NULL, BAD_CAST "head", NULL);
		addStyle(head, "./theme.css");
		addStyle(head, "./board.css");

    xmlNodePtr body = xmlNewChild(html, NULL, BAD_CAST "body", NULL);
		xmlNodePtr navbar = addElement(body, "div", NULL, "navbar", NULL);
            xmlNodePtr mainNavbarSection = addElement(navbar, "div", NULL, "nav-main", NULL);
                xmlNodePtr titleLink = addElement(mainNavbarSection, "a", "RingBulletin", "title-link", NULL);
                    xmlNewProp(titleLink, BAD_CAST "href", BAD_CAST "./board.html");
			xmlNodePtr rightNavbarSection = addElement(navbar, "div", NULL, "nav-right", NULL);
                xmlNodePtr aboutDropdown = addDropdownButton(rightNavbarSection, "./assets/svg/help-info-solid.svg");
                xmlNodeAddContent(aboutDropdown, BAD_CAST "About ");
                xmlNodePtr githubLink = xmlNewChild(aboutDropdown, NULL, BAD_CAST "a", "GitHub");
                    xmlNewProp(githubLink, BAD_CAST "href", BAD_CAST "https://github.com/Skimlk/ringbulletin");
                    xmlNewProp(githubLink, BAD_CAST "target", BAD_CAST "_blank");

                xmlNodePtr copyDropdown = addDropdownButton(rightNavbarSection, "./assets/svg/copy-to-clipboard-line.svg");
                xmlNodeAddContent(copyDropdown, BAD_CAST "Board URL:");
                xmlNodePtr boardUrl = xmlNewChild(copyDropdown, NULL, BAD_CAST "input", NULL);
                    xmlNewProp(boardUrl, BAD_CAST "type", BAD_CAST "text");
                    xmlNewProp(boardUrl, BAD_CAST "value", BAD_CAST "https://example.com");
        
        char *listTimestampedFilename = createTimestampedFilename("./list.html", "?");
        writeList();
        xmlNodePtr listIFrame = addElement(body, "iframe", NULL, "list-iframe", NULL);
            xmlNewProp(listIFrame, BAD_CAST "src", listTimestampedFilename);
            xmlNewProp(listIFrame, BAD_CAST "name", BAD_CAST "list-iframe");
        free(listTimestampedFilename);

    xmlChar *postSerialized;
    int size = 0;
    htmlDocDumpMemoryFormat(doc, &postSerialized, &size, 1); 

    if(!postSerialized) {
        printf("Post was not serialized\n");
    }   

    writeFile((const char *)postSerialized, &size, "./static/", "board.html");
    copyFile("./assets/css/", "theme.css", "./static/", "theme.css"); 
    copyFile("./assets/css/", "board.css", "./static/", "board.css"); 
    
	return 0;   
}