#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <stdint.h>
#include <dirent.h>
#include <inttypes.h>
#include <stdbool.h>

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

void copyXPathResults(htmlDocPtr htmlToFilter, char *xPathString, xmlNodePtr dest) {
    xmlXPathContextPtr xmlXPathHtmlToFilter = xmlXPathNewContext(htmlToFilter); 
    xmlXPathObjectPtr filteredHtml = xmlXPathEvalExpression(BAD_CAST xPathString, xmlXPathHtmlToFilter);
    for(int i = 0; i < filteredHtml->nodesetval->nodeNr; i++) {
        xmlNodePtr filterResult = filteredHtml->nodesetval->nodeTab[i];

        xmlUnsetProp(filterResult, BAD_CAST "class");
        xmlNewProp(filterResult, BAD_CAST "class", BAD_CAST "reply");

        xmlNodePtr filterResultCopied = xmlDocCopyNode(filterResult, dest->doc, 1);
        xmlAddChild(dest, filterResultCopied);
    }

    xmlXPathFreeObject(filteredHtml);
    xmlXPathFreeContext(xmlXPathHtmlToFilter);
}

void addPostToReplies(htmlDocPtr destDoc, PostData *post) {
    xmlXPathContextPtr destDocContext = xmlXPathNewContext(destDoc);
    xmlXPathObjectPtr repliesFilter = xmlXPathEvalExpression(BAD_CAST "//*[@class='replies']", destDocContext);
    createPostElement(repliesFilter->nodesetval->nodeTab[0], post, "reply");

    xmlXPathFreeObject(repliesFilter);
    xmlXPathFreeContext(destDocContext);
}

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
        = getFilenameListMatchingPattern(ctx->postsDirectoryFullPath, contains, (void *)post->normalizedTitleHashString);
    char *existingPostWithHashContent = NULL;
    htmlDocPtr existingPostWithHashDoc = NULL;
    htmlDocPtr newPostDoc = NULL;
    time_t existingPostWithHashTime;

    bool hasExistingPost = existingPostsWithHash->numberOfFiles > 0;
    if(hasExistingPost) {
        existingPostWithHashTime = extractTimeFromFilename(existingPostsWithHash->filenames[0]);
        existingPostWithHashContent = readFile(ctx->postsDirectoryFullPath, existingPostsWithHash->filenames[0]);
        existingPostWithHashDoc = htmlReadMemory(existingPostWithHashContent, 
            strlen(existingPostWithHashContent), NULL, "UTF-8", 0);
    }

    if (!hasExistingPost || post->pubDateUnix < existingPostWithHashTime) {
        newPostDoc = htmlNewDoc(BAD_CAST "http://www.w3.org/TR/html4/strict.dtd", BAD_CAST "HTML");
        xmlNodePtr html = xmlNewNode(NULL, BAD_CAST "html");
        xmlDocSetRootElement(newPostDoc, html);
        
        xmlNodePtr head = xmlNewChild(html, NULL, BAD_CAST "head", NULL);
            addStyle(head, "../theme.css");
            addStyle(head, "../board.css");

        xmlNodePtr body = xmlNewChild(html, NULL, BAD_CAST "body", NULL);
            createPostElement(body, post, "post");
            xmlNodePtr replies = addElement(body, "div", NULL, NULL, "replies");

        //If the existing post has a later date, add the existing post content
        //to this post and overwrite the existing post with this
        if (hasExistingPost) {
            copyXPathResults(existingPostWithHashDoc, "//*[@class='post']", replies);
            copyXPathResults(existingPostWithHashDoc, "//*[@class='replies']/*", replies);

            removeFile(ctx->postsDirectoryFullPath, existingPostsWithHash->filenames[0]);
        }

        writePost(ctx->postsDirectoryFullPath, newPostDoc, post->pubDateUnix, post->normalizedTitleHash);
    }

    //If the existing post has an earlier date, add this post's content
    //to the existing post
    else {
        addPostToReplies(existingPostWithHashDoc, post);
        writePost(ctx->postsDirectoryFullPath, existingPostWithHashDoc, existingPostWithHashTime, post->normalizedTitleHash);
    } 

    xmlFreeDoc(newPostDoc);
    xmlFreeDoc(existingPostWithHashDoc);
    free(existingPostWithHashContent);
    freeFilenameList(existingPostsWithHash);
}

void writeListItem(Context *ctx, xmlNodePtr list, char *postFilename) {
    char *itemFullPath = NULL;
    char *itemRelativePath = NULL;
    asprintf(&itemFullPath, "%s/%s", ctx->postsDirectoryFullPath, postFilename);
    asprintf(&itemRelativePath, "%s/%s", ctx->postsDirectoryRelativePath, postFilename);

    char *itemFileContents = readFile(NULL, itemFullPath);
    htmlDocPtr itemFileDocPtr = htmlReadMemory(itemFileContents, strlen(itemFileContents), NULL, "UTF-8", 0);
    xmlXPathContextPtr itemXPathContextPtr = xmlXPathNewContext(itemFileDocPtr);

    xmlXPathObjectPtr postInner = xmlXPathEvalExpression(BAD_CAST "//div[contains(@class,'post')]", itemXPathContextPtr);

    xmlNodePtr copyPostInner = xmlDocCopyNode(postInner->nodesetval->nodeTab[0], list->doc, 1);
        xmlNewChild(copyPostInner->children->children, NULL, BAD_CAST "span", BAD_CAST " • ");
        xmlNodePtr viewThreadLink = xmlNewChild(copyPostInner->children->children, NULL, BAD_CAST "a", BAD_CAST "View Thread");
            xmlNewProp(viewThreadLink, BAD_CAST "href", BAD_CAST itemRelativePath);
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
    free(itemRelativePath);
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

void writeConnectPage(Context *ctx) {
	htmlDocPtr doc = htmlNewDoc(BAD_CAST "http://www.w3.org/TR/html4/strict.dtd", BAD_CAST "HTML");
	xmlNodePtr html = xmlNewNode(NULL, BAD_CAST "html");
	xmlDocSetRootElement(doc, html);

	xmlNodePtr head = addElement(html, "head", NULL, NULL, NULL);
		addStyle(head, "./theme.css");
		addStyle(head, "./board.css");

	xmlNodePtr body = addElement(html, "body", NULL, NULL, NULL);
		xmlNodePtr board = addElement(body, "div", NULL, "board", NULL);
			xmlNodePtr intro = addElement(board, "div", NULL, NULL, "post");
				addElement(intro, "div", "Connect with this Board", NULL, "post-title");
				addElement(intro, "div", "Add this RingBulletin board to your network and start discovering federated content.", NULL, NULL);
				addElement(intro, "div", "Board URL:", NULL, "post-meta");

				xmlNodePtr boardUrl = addElement(intro, "textarea", ctx->boardJsonUrl, NULL, NULL);
				xmlNewProp(boardUrl, BAD_CAST "readonly", BAD_CAST "readonly");
				xmlNewProp(boardUrl, BAD_CAST "rows", BAD_CAST "1");

			xmlNodePtr peers = addElement(board, "div", NULL, NULL, "post");
				addElement(peers, "div", "How to add this board to your peers", NULL, "post-title");
                
                xmlNodePtr desc = addElement(peers, "div", NULL, NULL, "post-description");
                    addElement(desc, "div", "1. Copy the Board URL above.", NULL, NULL);
                    addElement(desc, "div", "2. Open (or create) your \"board.json\" file.", NULL, NULL);
                    addElement(desc, "div", "3. Add the URL to the \"peers\" array.", NULL, NULL);
                    addElement(desc, "div", "4. Run RingBulletin — it will automatically fetch posts from this board and its network.", NULL, NULL);

                addElement(peers, "div", "Example \"board.json\" snippet:", NULL, "post-meta");

                char *snippetText = NULL;
                asprintf(
                    &snippetText,
                    "{\n"
                    "  \"title\": \"My Board\",\n"
                    "  \"feeds\": [\"https://yoursite.com/feed.xml\"],\n"
                    "  \"peers\": [\n"
                    "    \"%s\",\n"
                    "    \"...\"\n"
                    "  ]\n"
                    "}",
                    ctx->boardJsonUrl
                );

                xmlNodePtr snippet = addElement(peers, "textarea", snippetText, NULL, NULL);
                    xmlNewProp(snippet, BAD_CAST "readonly", BAD_CAST "readonly");
                    xmlNewProp(snippet, BAD_CAST "rows", BAD_CAST "8");

				free(snippetText);

			xmlNodePtr embed = addElement(board, "div", NULL, NULL, "post");
				addElement(embed, "div", "Embed this board on your website", NULL, "post-title");
				addElement(embed, "div", "Display the full board directly on your site using this iframe:", NULL, "post-description");

				char *iframeText = NULL;
				asprintf(
					&iframeText,
					"<iframe style=\"height: 750px; width: 100%%; border: none; border-radius: 12px;\"\n"
					"    src=\"%s\"\n"
					"    title=\"RingBulletin Board\" allowfullscreen>\n"
					"</iframe>",
					ctx->boardHtmlUrl
				);

				xmlNodePtr iframe = addElement(embed, "textarea", iframeText, NULL, NULL);
                    xmlNewProp(iframe, BAD_CAST "readonly", BAD_CAST "readonly");
                    xmlNewProp(iframe, BAD_CAST "rows", BAD_CAST "4");

				free(iframeText);

	xmlChar *serialized = NULL;
	int size = 0;
	htmlDocDumpMemoryFormat(doc, &serialized, &size, 1);

	writeFile((const char *)serialized, &size, ctx->config->boardGenerationDirectory, "connect.html");

	xmlFreeDoc(doc);
	xmlFree(serialized);
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
                addNavbarButton(rightNavbarSection, "./about.html", "./assets/svg/help-info-solid.svg");
                addNavbarButton(rightNavbarSection, "./connect.html", "./assets/svg/copy-to-clipboard-line.svg");
                        
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
    
    copyFile(NULL, "./board.json", ctx->config->boardGenerationDirectory, "board.json"); 
    copyFile(NULL, "./assets/css/theme.css", ctx->config->boardGenerationDirectory, "theme.css"); 
    copyFile(NULL, "./assets/css/board.css", ctx->config->boardGenerationDirectory, "board.css"); 
    copyFile(NULL, "./assets/html/about.html", ctx->config->boardGenerationDirectory, "about.html"); 
    writeConnectPage(ctx);
    writeFile((const char *)postSerialized, &size, ctx->config->boardGenerationDirectory, "board.html");

    xmlFreeDoc(doc);
    xmlFree(postSerialized);
	return 0;   
}