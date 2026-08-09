#ifndef FEED_H
#define FEED_H

#include <libxml/tree.h>

#include "context.h"
#include "post.h"

extern int feedIsValid(xmlDocPtr doc);
extern int postAlreadyWritten(PostData *post, char *url);
extern int hydratePostContent(xmlDocPtr doc, xmlNodePtr postNode, PostData *post);
extern int processFeed(char *feed, Context *ctx, char *url);

#endif
