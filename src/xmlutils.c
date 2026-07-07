#include <string.h>
#include <libxml/xpath.h>

#include "fileutils.h"
#include "bulletin.h"

xmlNodePtr addElement(xmlNodePtr parent, const char *tag, const char *text, const char *id, const char *class) {
    xmlNodePtr node = xmlNewChild(parent, NULL, BAD_CAST tag, text ? BAD_CAST text : NULL);
    if(id) xmlNewProp(node, BAD_CAST "id", BAD_CAST id);
    if(class) xmlNewProp(node, BAD_CAST "class", BAD_CAST class);
    return node;
}

void addStyle(xmlNodePtr head, const char *stylePath) {
    xmlNodePtr link = xmlNewChild(head, NULL, BAD_CAST "link", NULL);
    xmlNewProp(link, BAD_CAST "rel", BAD_CAST "stylesheet");
    xmlNewProp(link, BAD_CAST "type", BAD_CAST "text/css");
    xmlNewProp(link, BAD_CAST "href", BAD_CAST stylePath);
}

xmlNodePtr createPostElement(xmlNodePtr parent, const PostData *post, const char *class) {
    xmlNodePtr postElement = addElement(parent, "div", NULL, NULL, class);
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

    return postElement;
}

void addNavbarButton(xmlNodePtr parent, char *linkPath, char *iconPath) {
    xmlNodePtr navbarButton = addElement(parent, "a", NULL, NULL, "navbar-button");
        xmlNewProp(navbarButton, BAD_CAST "href", BAD_CAST linkPath);
        xmlNewProp(navbarButton, BAD_CAST "target", BAD_CAST "content-iframe");

        xmlNodePtr navbarButtonIcon = addElement(navbarButton, "span", NULL, NULL, "navbar-button-icon");
            char *icon = readFile(NULL, iconPath);
            xmlDocPtr iconDoc = xmlReadMemory(icon, (int)strlen(icon), "icon.svg", NULL, 0);
            xmlNodePtr iconRoot = xmlDocGetRootElement(iconDoc);
            xmlNodePtr importedIcon = xmlDocCopyNode(iconRoot, parent->doc, 1);
            xmlAddChild(navbarButtonIcon, importedIcon);

    xmlFreeDoc(iconDoc);
    free(icon);
}