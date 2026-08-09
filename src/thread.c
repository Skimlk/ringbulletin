#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>

#include <libxml/HTMLtree.h>
#include <libxml/xpath.h>
#include "xxhash.h"

#include "filesystem.h"
#include "render.h"
#include "thread.h"
#include "timeutils.h"

int writePost(char *directory, htmlDocPtr postDoc, time_t postTimestamp, XXH64_hash_t postHash) {
    int ret = 0;
    xmlChar *postCharBuffer = NULL;
    
    char filename[PATH_MAX];
    snprintf(filename, sizeof(filename), "%lld_%016" PRIx64 ".html",
        (long long) postTimestamp, postHash);

    int size = 0;
    htmlDocDumpMemoryFormat(postDoc, &postCharBuffer, &size, 0);

    if(!postCharBuffer) {
        printf("Post was not serialized\n");
        ret = 1;
        goto cleanup;
    }

    writeFile((const char *)postCharBuffer, &size, directory, filename);

cleanup:
    xmlFree(postCharBuffer);
    return ret;
}

void processPost(PostData *post, Context *ctx) {
    FilenameList *existingPostsWithHash 
        = getFilenameListMatchingPattern(ctx->postsDirectoryPath, contains, (void *)post->normalizedTitleHashString);
    char *existingPostWithHashContent = NULL;
    htmlDocPtr existingPostWithHashDoc = NULL;
    htmlDocPtr newPostDoc = NULL;
    time_t existingPostWithHashTime;

    bool hasExistingPost = existingPostsWithHash->numberOfFiles > 0;
    if(hasExistingPost) {
        existingPostWithHashTime = extractTimeFromFilename(existingPostsWithHash->filenames[0]);
        existingPostWithHashContent = readFileStr(ctx->postsDirectoryPath, existingPostsWithHash->filenames[0]);
        existingPostWithHashDoc = htmlReadMemory(existingPostWithHashContent,
            strlen(existingPostWithHashContent), NULL, "UTF-8", HTML_PARSE_NOBLANKS);
    }

    if (!hasExistingPost || post->pubDateUnix < existingPostWithHashTime) {
        newPostDoc = htmlNewDoc(NULL, NULL);
        xmlNodePtr html = xmlNewNode(NULL, BAD_CAST "html");
        xmlDocSetRootElement(newPostDoc, html);
        
        xmlNodePtr head = xmlNewChild(html, NULL, BAD_CAST "head", NULL);
            addStyle(head, "../css/board.css");  
            addStyle(head, "../css/theme.css");

        //If the existing post has a later date, add the existing post content
        //to this post and overwrite the existing post with this

        xmlNodeSetPtr replies = NULL;
        xmlXPathObjectPtr repliesObject = NULL;
        if (hasExistingPost) {
            repliesObject = getXPathObjectFromXPath(
                existingPostWithHashDoc, "//*[@class='post'] | //*[@class='replies']/*");
            replies = repliesObject->nodesetval;

            removeFile(ctx->postsDirectoryPath, existingPostsWithHash->filenames[0]);
        }

        xmlNodePtr body = xmlNewChild(html, NULL, BAD_CAST "body", NULL);
            generateThread(body, createPostElement(NULL, post, "post"), replies);

        xmlXPathFreeObject(repliesObject);
        writePost(ctx->postsDirectoryPath, newPostDoc, post->pubDateUnix, post->normalizedTitleHash);
    }

    //If the existing post has an earlier date, add this post's content
    //to the existing post
    else {
        createPostElement(getNodePtrFromXPath(existingPostWithHashDoc, "//*[@class='replies']"), post, "reply");
        writePost(ctx->postsDirectoryPath, existingPostWithHashDoc, existingPostWithHashTime, post->normalizedTitleHash);
    } 

    xmlFreeDoc(newPostDoc);
    xmlFreeDoc(existingPostWithHashDoc);
    free(existingPostWithHashContent);
    freeFilenameList(existingPostsWithHash);
}
