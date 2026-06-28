#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <stdint.h>
#include <dirent.h>
#include <inttypes.h>

#include <libxml/HTMLtree.h>
#include <libxml/xpath.h>
#include "xxhash.h"

#include "bulletin.h"
#include "fileutils.h"
#include "stringutils.h"
#include "context.h"
#include "xmlutils.h"

PostData *initalizePost() {
    PostData *post = malloc(sizeof(PostData));
   
    post->title = NULL;
    post->link = NULL;
    post->description = NULL;
    post->normalizedTitleHashString = NULL;
    post->pubDateFormattedString = NULL;
    
    return post;
}

void copyPostData(PostData *newPost, PostData *originalPost) {
    newPost->title = strdup(originalPost->title);
    newPost->link = strdup(originalPost->link);
	newPost->description = strdup(originalPost->description);
	newPost->normalizedTitleHash = originalPost->normalizedTitleHash;
	newPost->normalizedTitleHashString = strdup(originalPost->normalizedTitleHashString);
	newPost->pubDateUnix = originalPost->pubDateUnix;
	newPost->pubDateFormattedString = strdup(originalPost->pubDateFormattedString);
}

void freePostData(PostData *post) {
    free(post->title);
    free(post->link);
    free(post->description);
    free(post->normalizedTitleHashString);
    free(post->pubDateFormattedString);
    free(post);
}

int writePost(const PostData *post, Context *ctx) {
    int ret = 0;
    htmlDocPtr doc = htmlNewDoc(BAD_CAST "http://www.w3.org/TR/html4/strict.dtd", BAD_CAST "HTML");
    xmlNodePtr html = xmlNewNode(NULL, BAD_CAST "html");
    char *replyFile;
    xmlDocSetRootElement(doc, html);

	xmlNodePtr head = xmlNewChild(html, NULL, BAD_CAST "head", NULL);
		addStyle(head, "../theme.css");
		addStyle(head, "../board.css");

    xmlNodePtr body = xmlNewChild(html, NULL, BAD_CAST "body", NULL);
    xmlNodePtr postElement = addElement(body, "div", NULL, NULL, "post");
		xmlNodePtr postHeader = addElement(postElement, "div", NULL, NULL, "post-header");
            xmlNodePtr postTitle = addElement(postHeader, "div", NULL, NULL, "post-title"); 
                xmlNodePtr postLink = xmlNewChild(postTitle, NULL, BAD_CAST "a", BAD_CAST post->title);
                    xmlNewProp(postLink, BAD_CAST "href", BAD_CAST post->link);
                    xmlNewProp(postLink, BAD_CAST "target", BAD_CAST "_blank");
                
        xmlNodePtr postMeta = addElement(postElement, "div", NULL, NULL, "post-meta");
			addElement(postMeta, "span", post->pubDateFormattedString, NULL, "post-date");
			xmlNewChild(postMeta, NULL, BAD_CAST "span", BAD_CAST " • ");
			addElement(postMeta, "span", post->link, NULL, "post-url");

        addElement(postElement, "div", post->description, NULL, "post-description");

    xmlNodePtr replies = addElement(body, "div", NULL, NULL, "replies");
   
    char filename[PATH_MAX];
    FilenameList *existingPostsWithHash = getFilenameListMatchingPattern("./static/posts", contains, (void *)post->normalizedTitleHashString);
    if(existingPostsWithHash->numberOfFiles > 0) {
        char *existingPostWithHash = existingPostsWithHash->filenames[0];
        time_t existingPostWithHashTime = extractTimeFromFilename(existingPostWithHash);

        //Add feed to history
        replyFile = readFile("./static/posts/", existingPostWithHash);
        htmlDocPtr replyDoc = htmlReadMemory(replyFile, strlen(replyFile), NULL, "UTF-8", 0);
        xmlXPathContextPtr replyDocXPathCtx = xmlXPathNewContext(replyDoc); 
        
        //If the existing post has a later date, add the existing post content
            //to this post and overwrite the existing post with this one
        if(post->pubDateUnix < existingPostWithHashTime) {
            xmlXPathObjectPtr innerPost = xmlXPathEvalExpression(
                BAD_CAST "//*[@class='post']",
                replyDocXPathCtx 
            );

            xmlNodePtr sameHashPost = innerPost->nodesetval->nodeTab[0];
            xmlUnsetProp(sameHashPost, BAD_CAST "class");
            xmlNewProp(sameHashPost, BAD_CAST "class", BAD_CAST "reply");

            xmlNodePtr imported = xmlDocCopyNode(sameHashPost, replies->doc, 1);

            xmlAddChild(replies, imported);

            removeFile("./static/posts/", existingPostWithHash);
        }
        //If the existing post has an earlier date, add this post's content
            //to the existing post
        else {
            xmlXPathObjectPtr matchingHashReplies = xmlXPathEvalExpression(BAD_CAST "//*[@class='replies']", replyDocXPathCtx);

            xmlNodePtr repliesNode = matchingHashReplies->nodesetval->nodeTab[0];

            xmlUnsetProp(postElement, BAD_CAST "class");
            xmlNewProp(postElement, BAD_CAST "class", BAD_CAST "reply");

            xmlNodePtr imported = xmlDocCopyNode(postElement, replyDoc, 1);
            xmlAddChild(repliesNode, imported);

            xmlChar *buffer = NULL;
            int size = 0;
            
            htmlDocDumpMemoryFormat(replyDoc, &buffer, &size, 0);

            snprintf(filename, sizeof(filename), "%lld_%016" PRIx64 ".html",
                (long long)existingPostWithHashTime, post->normalizedTitleHash);
            
            writeFile((const char *)buffer, &size, "./static/posts/", filename);

            ret = 0;
            goto cleanup;
        }
    }

    snprintf(filename, sizeof(filename), "%lld_%016" PRIx64 ".html",
        (long long) post->pubDateUnix, post->normalizedTitleHash);

    xmlChar *postSerialized;
    int size = 0;
    
    htmlDocDumpMemoryFormat(doc, &postSerialized, &size, 0);

    if(!postSerialized) {
        printf("Post was not serialized\n");
        ret = 1;
        goto cleanup;
    }

    writeFile((const char *)postSerialized, &size, ctx->postsDirectory, filename);

cleanup:
    xmlFree(postSerialized);
    free(replyFile);
    freeFilenameList(existingPostsWithHash);
    xmlFreeDoc(doc);
    return ret;
}

void writeListItem(Context *ctx, xmlNodePtr list, char *postFilename) {
    size_t strlenPath = strlen(ctx->postsDirectory) + strlen(postFilename) + 1;
    char *itemFullPath = malloc(sizeof(char) * strlenPath);
    snprintf(itemFullPath, strlenPath,
        "%s%s", ctx->postsDirectory, postFilename);

    char *itemFileContents = readFile(NULL, itemFullPath);
    htmlDocPtr itemFileDocPtr = htmlReadMemory(itemFileContents, strlen(itemFileContents), NULL, "UTF-8", 0);
    xmlXPathContextPtr itemXPathContextPtr = xmlXPathNewContext(itemFileDocPtr);

    xmlXPathObjectPtr postInner = xmlXPathEvalExpression(BAD_CAST "//div[contains(@class,'post')]", itemXPathContextPtr);

    xmlNodePtr copyPostInner = xmlDocCopyNode(postInner->nodesetval->nodeTab[0], list->doc, 1);
        xmlNewChild(copyPostInner->children->children, NULL, BAD_CAST "span", BAD_CAST " • ");
        xmlNodePtr viewThreadLink = xmlNewChild(copyPostInner->children->children, NULL, BAD_CAST "a", BAD_CAST "View Thread");
                xmlNewProp(viewThreadLink, BAD_CAST "href", BAD_CAST itemFullPath);
                xmlNewProp(viewThreadLink, BAD_CAST "target", BAD_CAST "content-iframe");
    
    xmlAddChild(list, copyPostInner);
    
    xmlXPathObjectPtr previewReplies = xmlXPathEvalExpression(BAD_CAST "//div[contains(@class,'replies')]/*", itemXPathContextPtr);

    if (previewReplies && previewReplies->nodesetval) {
        int previewPostCount = 3;
        int previewStartIndex = (previewReplies->nodesetval->nodeNr - previewPostCount < 0) ? 0
            : previewReplies->nodesetval->nodeNr - previewPostCount;

        for (int i = previewStartIndex; i < previewReplies->nodesetval->nodeNr; i++) {
            xmlNodePtr copy = xmlDocCopyNode(previewReplies->nodesetval->nodeTab[i], list->doc, 1);
            xmlAddChild(list, copy);
        }
    }

    xmlXPathFreeObject(postInner);
    xmlXPathFreeObject(previewReplies);
    xmlXPathFreeContext(itemXPathContextPtr);
    xmlFreeDoc(itemFileDocPtr);
    free(itemFileContents);
    free(itemFullPath);
}

int writeList(Context *ctx) {
    int ret = 0;
    FilenameList *posts = getFilenameList("./static/posts");
    qsort(posts->filenames, posts->numberOfFiles, sizeof(char *), compare);
   
    htmlDocPtr doc = htmlNewDoc(BAD_CAST "http://www.w3.org/TR/html4/strict.dtd", BAD_CAST "HTML");
    xmlNodePtr html = xmlNewNode(NULL, BAD_CAST "html");
    xmlDocSetRootElement(doc, html);

	xmlNodePtr head = xmlNewChild(html, NULL, BAD_CAST "head", NULL);
		addStyle(head, "./theme.css");
		addStyle(head, "./board.css");

    xmlNodePtr body = xmlNewChild(html, NULL, BAD_CAST "body", NULL);
    xmlNodePtr list = addElement(body, "div", NULL, "board", NULL);
        for(int i = 0; i < posts->numberOfFiles; i++) { writeListItem(ctx, list, posts->filenames[i]); }
    
    xmlChar *postSerialized;
    int size = 0;
    htmlDocDumpMemoryFormat(doc, &postSerialized, &size, 1); 

    if(!postSerialized) {
        printf("Post was not serialized\n");
        ret = 1;
        goto cleanup;
    }   

    writeFile((const char *)postSerialized, &size, "./static/", "./list.html");

cleanup:
    xmlFree(postSerialized);
    freeFilenameList(posts);
    xmlFreeDoc(doc);
    return ret;
}

int writeBulletin(Context *ctx) {
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
                xmlNodePtr githubLink = xmlNewChild(aboutDropdown, NULL, BAD_CAST "a", BAD_CAST "GitHub");
                    xmlNewProp(githubLink, BAD_CAST "href", BAD_CAST "https://github.com/Skimlk/ringbulletin");
                    xmlNewProp(githubLink, BAD_CAST "target", BAD_CAST "_blank");

                xmlNodePtr copyDropdown = addDropdownButton(rightNavbarSection, "./assets/svg/copy-to-clipboard-line.svg");
                xmlNodeAddContent(copyDropdown, BAD_CAST "Board URL:");
                xmlNodePtr boardUrl = xmlNewChild(copyDropdown, NULL, BAD_CAST "input", NULL);
                    xmlNewProp(boardUrl, BAD_CAST "type", BAD_CAST "text");
                    xmlNewProp(boardUrl, BAD_CAST "value", BAD_CAST ctx->config->boardJsonUrl);
        
        char *listTimestampedFilename = createTimestampedFilename("./list.html", "?");
        writeList(ctx);
        xmlNodePtr listIFrame = addElement(body, "iframe", NULL, "content-iframe", NULL);
            xmlNewProp(listIFrame, BAD_CAST "src", BAD_CAST listTimestampedFilename);
            xmlNewProp(listIFrame, BAD_CAST "name", BAD_CAST "content-iframe");
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

    xmlFreeDoc(doc);
    xmlFree(postSerialized);
	return 0;   
}