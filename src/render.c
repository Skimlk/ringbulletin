#include <stdlib.h>
#include <stdio.h>

#include <libxml/HTMLtree.h>
#include <libxml/xpath.h>

#include "filesystem.h"
#include "render.h"
#include "post.h"
#include "timeutils.h"

xmlXPathObjectPtr getXPathObjectFromXPath(htmlDocPtr doc, char *xPathStr) {
    xmlXPathContextPtr ctx = xmlXPathNewContext(doc);
    xmlXPathObjectPtr xPathFilter = xmlXPathEvalExpression(BAD_CAST xPathStr, ctx);
    
    xmlXPathFreeContext(ctx);
    return xPathFilter;
}

xmlNodePtr getNodePtrFromXPath(htmlDocPtr doc, char *xPathStr) {
    xmlNodePtr result = NULL;
    xmlXPathObjectPtr xPathFilter = getXPathObjectFromXPath(doc, xPathStr);
    
    if(xPathFilter != NULL && xPathFilter->nodesetval->nodeNr > 0) 
        result = xPathFilter->nodesetval->nodeTab[0];
    
    xmlXPathFreeObject(xPathFilter);
    return result;
} 

xmlNodePtr addElement(xmlNodePtr parent, const char *tag, const char *text, const char *id, const char *class) {
    xmlNodePtr node;
    
    if (parent == NULL)
        node = xmlNewNode(NULL, BAD_CAST tag);
    else
        node = xmlNewChild(parent, NULL, BAD_CAST tag, NULL);

    if (text) {
        xmlNodeSetContent(node, BAD_CAST text);
    }
    
    if (id)
        xmlNewProp(node, BAD_CAST "id", BAD_CAST id);
    
    if (class)
        xmlNewProp(node, BAD_CAST "class", BAD_CAST class);
    
    return node;
}

void addStyle(xmlNodePtr head, const char *stylePath) {
    xmlNodePtr link = xmlNewChild(head, NULL, BAD_CAST "link", NULL);
    xmlNewProp(link, BAD_CAST "rel", BAD_CAST "stylesheet");
    xmlNewProp(link, BAD_CAST "type", BAD_CAST "text/css");
    xmlNewProp(link, BAD_CAST "href", BAD_CAST stylePath);
}

void addNavbarButton(xmlNodePtr parent, char *linkPath, char *iconId) {
    xmlNodePtr navbarButton = addElement(parent, "a", NULL, NULL, "navbar-button");
        xmlNewProp(navbarButton, BAD_CAST "href", BAD_CAST linkPath);
        xmlNewProp(navbarButton, BAD_CAST "target", BAD_CAST "content-iframe");

        addElement(navbarButton, "img", NULL, iconId, "navbar-button-icon");
}

xmlNodePtr createPostElement(xmlNodePtr parent, const PostData *post, const char *class) {
    xmlNodePtr postElement = addElement(parent, "div", NULL, NULL, class);
        xmlNodePtr icon = addElement(postElement, "img", NULL, NULL, "post-icon");
                xmlNewProp(icon, BAD_CAST "src", BAD_CAST post->iconPath);
        xmlNodePtr postHeader = addElement(postElement, "div", NULL, NULL, "post-header");
            xmlNodePtr postTitle = addElement(postHeader, "span", NULL, NULL, "post-title"); 
                xmlNodePtr postLink = xmlNewChild(postTitle, NULL, BAD_CAST "a", BAD_CAST post->title);
                    xmlNewProp(postLink, BAD_CAST "href", BAD_CAST post->link);
                    xmlNewProp(postLink, BAD_CAST "target", BAD_CAST "_blank");
            addElement(postHeader, "span", post->domain, NULL, "post-url");
            addElement(postHeader, "span", post->pubDateFormattedString, NULL, "post-date");
        
        addElement(postElement, "blockquote", post->description, NULL, "post-description");

    return postElement;
}

xmlNodePtr generateThread(xmlNodePtr parent, xmlNodePtr openingPost, xmlNodeSetPtr replies) {
    xmlNodePtr thread = addElement(parent, "div", NULL, NULL, "thread");
        xmlAddChild(thread, openingPost);
        xmlNodePtr repliesElement = addElement(thread, "div", NULL, NULL, "replies");
            if(replies != NULL) {
                for(int reply = 0; reply < replies->nodeNr; reply++) {
                    xmlUnsetProp(replies->nodeTab[reply], BAD_CAST "class");
                    xmlNewProp(replies->nodeTab[reply], BAD_CAST "class", BAD_CAST "reply");
                    xmlAddChild(repliesElement, xmlDocCopyNode(replies->nodeTab[reply], parent->doc, 1));
                }
            }

    return thread;
}

void writeListItem(Context *ctx, xmlNodePtr list, char *postFilename) {
    char *itemRelativePath = NULL;
    char *itemFullPath = NULL;
    asprintf(&itemFullPath, "%s/%s", ctx->postsDirectoryPath, postFilename);
    asprintf(&itemRelativePath, "../posts/%s", postFilename);

    File itemFile = readFile(NULL, itemFullPath);
    htmlDocPtr itemFileDocPtr = htmlReadMemory(
        itemFile.content, itemFile.size, NULL, "UTF-8", HTML_PARSE_NOBLANKS);
    xmlXPathContextPtr itemXPathContextPtr = xmlXPathNewContext(itemFileDocPtr);

    xmlXPathObjectPtr postInner = xmlXPathEvalExpression(BAD_CAST "//div[contains(@class,'post')]", itemXPathContextPtr);
    xmlXPathObjectPtr previewReplies = xmlXPathEvalExpression(BAD_CAST "//div[contains(@class,'replies')]/*", itemXPathContextPtr);
    generateThread(list, xmlDocCopyNode(postInner->nodesetval->nodeTab[0], list->doc, 1), previewReplies->nodesetval);

    char *lastPostHeader = "(//div[contains(@class,'post')]/div[contains(@class,'post-header')])[last()]";
    xmlNodePtr viewThreadSpan = xmlNewChild(getNodePtrFromXPath(list->doc, lastPostHeader), NULL, BAD_CAST "span", NULL);
        xmlAddChild(viewThreadSpan, xmlNewText(BAD_CAST "["));
        xmlNodePtr viewThreadLink = xmlNewChild(viewThreadSpan, NULL, BAD_CAST "a", BAD_CAST "View Thread");
            xmlNewProp(viewThreadLink, BAD_CAST "href", BAD_CAST itemRelativePath);
            xmlNewProp(viewThreadLink, BAD_CAST "target", BAD_CAST "content-iframe");
        xmlAddChild(viewThreadSpan, xmlNewText(BAD_CAST "]"));

    xmlNewChild(list, NULL, BAD_CAST "hr", NULL);

    xmlXPathFreeContext(itemXPathContextPtr);
    xmlXPathFreeObject(postInner);
    xmlXPathFreeObject(previewReplies);
    xmlFreeDoc(itemFileDocPtr);
    free(itemFile.content);
    free(itemRelativePath);
    free(itemFullPath);
}

int writeList(Context *ctx) {
    int ret = 0;
    FilenameList *posts = getFilenameList(ctx->postsDirectoryPath);
    qsort(posts->filenames, posts->numberOfFiles, sizeof(char *), compare);
   
    htmlDocPtr doc = htmlNewDoc(NULL, NULL);
    xmlNodePtr html = xmlNewNode(NULL, BAD_CAST "html");
    xmlDocSetRootElement(doc, html);

	xmlNodePtr head = xmlNewChild(html, NULL, BAD_CAST "head", NULL);
		addStyle(head, "../css/board.css");
		addStyle(head, "../css/theme.css");

    xmlNodePtr body = xmlNewChild(html, NULL, BAD_CAST "body", NULL);
    xmlNodePtr list = addElement(body, "div", NULL, "board", NULL);
        for(int i = 0; i < posts->numberOfFiles; i++) { writeListItem(ctx, list, posts->filenames[i]); }
    
    xmlChar *postSerialized;
    int size = 0;
    htmlDocDumpMemoryFormat(doc, &postSerialized, &size, 0); 

    if(!postSerialized) {
        printf("Post was not serialized\n");
        ret = 1;
        goto cleanup;
    }   

    writeFile((const char *)postSerialized, &size, ctx->viewsDirectoryPath, "list.html");

cleanup:
    xmlFree(postSerialized);
    freeFilenameList(posts);
    xmlFreeDoc(doc);
    return ret;
}

void writeConnectPage(Context *ctx) {
	htmlDocPtr doc = htmlNewDoc(NULL, NULL);
	xmlNodePtr html = xmlNewNode(NULL, BAD_CAST "html");
	xmlDocSetRootElement(doc, html);

	xmlNodePtr head = addElement(html, "head", NULL, NULL, NULL);
		addStyle(head, "../css/board.css");
		addStyle(head, "../css/theme.css");

	xmlNodePtr body = addElement(html, "body", NULL, NULL, NULL);
		xmlNodePtr board = addElement(body, "div", NULL, "connect-board", NULL);
			xmlNodePtr intro = addElement(board, "div", NULL, NULL, "connect-section");
				addElement(intro, "div", "Connect with this Board", NULL, "post-title");
				addElement(intro, "div", "Add this RingBulletin board to your network and start discovering federated content.", NULL, "connect-copy");
				addElement(intro, "div", "Board URL:", NULL, "connect-label");

				xmlNodePtr boardUrl = addElement(intro, "textarea", ctx->boardJsonUrl, NULL, "connect-textarea connect-url");
				xmlNewProp(boardUrl, BAD_CAST "readonly", BAD_CAST "readonly");
				xmlNewProp(boardUrl, BAD_CAST "rows", BAD_CAST "1");

			xmlNodePtr peers = addElement(board, "div", NULL, NULL, "connect-section");
				addElement(peers, "div", "How to add this board to your peers", NULL, "post-title");
                
                xmlNodePtr desc = addElement(peers, "div", NULL, NULL, "connect-steps");
                    addElement(desc, "div", "1. Copy the Board URL above.", NULL, NULL);
                    addElement(desc, "div", "2. Open (or create) your \"board.json\" file.", NULL, NULL);
                    addElement(desc, "div", "3. Add the URL to the \"peers\" array.", NULL, NULL);
                    addElement(desc, "div", "4. Run RingBulletin — it will automatically fetch posts from this board and its network.", NULL, NULL);

                addElement(peers, "div", "Example \"board.json\" snippet:", NULL, "connect-label");

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

                xmlNodePtr snippet = addElement(peers, "textarea", snippetText, NULL, "connect-textarea connect-snippet");
                    xmlNewProp(snippet, BAD_CAST "readonly", BAD_CAST "readonly");
                    xmlNewProp(snippet, BAD_CAST "rows", BAD_CAST "8");

				free(snippetText);

			xmlNodePtr embed = addElement(board, "div", NULL, NULL, "connect-section");
				addElement(embed, "div", "Embed this board on your website", NULL, "post-title");
				addElement(embed, "div", "Display the full board directly on your site using this iframe:", NULL, "connect-copy");

				char *iframeText = NULL;
				asprintf(
					&iframeText,
					"<iframe style=\"height: 750px; width: 100%%;\"\n"
					"    src=\"%s\"\n"
					"    title=\"RingBulletin Board\" allowfullscreen>\n"
					"</iframe>",
					ctx->boardHtmlUrl
				);

				xmlNodePtr iframe = addElement(embed, "textarea", iframeText, NULL, "connect-textarea connect-embed");
                    xmlNewProp(iframe, BAD_CAST "readonly", BAD_CAST "readonly");
                    xmlNewProp(iframe, BAD_CAST "rows", BAD_CAST "4");

				free(iframeText);

	xmlChar *serialized = NULL;
	int size = 0;
	htmlDocDumpMemoryFormat(doc, &serialized, &size, 1);

	writeFile((const char *)serialized, &size, ctx->viewsDirectoryPath, "connect.html");

	xmlFreeDoc(doc);
	xmlFree(serialized);
}

int writeBulletin(Context *ctx) {
    htmlDocPtr doc = htmlNewDoc(NULL, NULL);
    xmlNodePtr html = xmlNewNode(NULL, BAD_CAST "html");
    xmlDocSetRootElement(doc, html);

	xmlNodePtr head = xmlNewChild(html, NULL, BAD_CAST "head", NULL);
		addStyle(head, "./css/board.css");
		addStyle(head, "./css/theme.css");

    xmlNodePtr body = xmlNewChild(html, NULL, BAD_CAST "body", NULL);
		xmlNodePtr navbar = addElement(body, "div", NULL, "navbar", NULL);
            xmlNodePtr mainNavbarSection = addElement(navbar, "div", NULL, "nav-main", NULL);
                xmlNodePtr titleLink = addElement(mainNavbarSection, "a", "RingBulletin", "title-link", NULL);
                    xmlNewProp(titleLink, BAD_CAST "href", BAD_CAST "./board.html");
			xmlNodePtr rightNavbarSection = addElement(navbar, "div", NULL, "nav-right", NULL);
                addNavbarButton(rightNavbarSection, "./views/about.html", "navbar-button-icon-about");
                addNavbarButton(rightNavbarSection, "./views/connect.html", "navbar-button-icon-connect");
                        
        char *listTimestampedFilename = createTimestampedFilename("./views/list.html", "?");
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
    
    writeConnectPage(ctx);
    writeFile((const char *)postSerialized, &size, ctx->config->boardGenerationDirectory, "board.html");

    xmlFreeDoc(doc);
    xmlFree(postSerialized);
	return 0;   
}
