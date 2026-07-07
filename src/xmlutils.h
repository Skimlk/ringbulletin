#ifndef XMLUTILS_H
#define XMLUTILS_H

#include <libxml/xpath.h>
#include "bulletin.h"

extern xmlNodePtr addElement(xmlNodePtr parent, const char *tag, const char *text, const char *id, const char *class);
extern void addStyle(xmlNodePtr head, const char *stylePath);
extern xmlNodePtr createPostElement(xmlNodePtr parent, const PostData *post, const char *class);
extern void addNavbarButton(xmlNodePtr parent, char *linkPath, char *iconPath);

#endif