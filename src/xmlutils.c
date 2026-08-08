#include <string.h>
#include <libxml/xpath.h>

#include "fileutils.h"
#include "bulletin.h"

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

void addNavbarButton(xmlNodePtr parent, char *linkPath, char *iconId) {
    xmlNodePtr navbarButton = addElement(parent, "a", NULL, NULL, "navbar-button");
        xmlNewProp(navbarButton, BAD_CAST "href", BAD_CAST linkPath);
        xmlNewProp(navbarButton, BAD_CAST "target", BAD_CAST "content-iframe");

        addElement(navbarButton, "img", NULL, iconId, "navbar-button-icon");
}
