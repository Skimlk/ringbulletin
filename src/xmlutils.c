#include <string.h>
#include <libxml/xpath.h>

#include "fileutils.h"

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

xmlNodePtr addDropdownButton(xmlNodePtr parent, char *iconPath) {
    xmlNodePtr details = addElement(parent, "details", NULL, NULL, "dropdown-button");
        xmlNodePtr summary = addElement(details, "summary", NULL, NULL, "icon-button");
            char *icon = readFile(NULL, iconPath);
            xmlDocPtr iconDoc = xmlReadMemory(
                icon,
                (int)strlen(icon),
                "icon.svg",
                NULL,
                XML_PARSE_NOERROR | XML_PARSE_NOWARNING
            );
            xmlNodePtr iconRoot = xmlDocGetRootElement(iconDoc);
            xmlNodePtr importedIcon = xmlDocCopyNode(iconRoot, parent->doc, 1);
            xmlAddChild(summary, importedIcon);
   
        xmlNodePtr dropdownMenu = addElement(details, "div", NULL, NULL, "dropdown-menu");

    return dropdownMenu;
}