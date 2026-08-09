#ifndef RENDER_H
#define RENDER_H

#include <libxml/HTMLtree.h>
#include <libxml/xpath.h>

#include "context.h"
#include "post.h"

extern xmlXPathObjectPtr getXPathObjectFromXPath(htmlDocPtr doc, char *xPathStr);
extern xmlNodePtr getNodePtrFromXPath(htmlDocPtr doc, char *xPathStr);
extern xmlNodePtr addElement(xmlNodePtr parent, const char *tag, const char *text, const char *id, const char *class);
extern void addStyle(xmlNodePtr head, const char *stylePath);
extern void addNavbarButton(xmlNodePtr parent, char *linkPath, char *iconId);
extern xmlNodePtr createPostElement(xmlNodePtr parent, const PostData *post, const char *class);
extern xmlNodePtr generateThread(xmlNodePtr parent, xmlNodePtr openingPost, xmlNodeSetPtr replies);
extern void writeListItem(Context *ctx, xmlNodePtr list, char *postFilename);
extern int writeList(Context *ctx);
extern void writeConnectPage(Context *ctx);
extern int writeBulletin(Context *ctx);

#endif
