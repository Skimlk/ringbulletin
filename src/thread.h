#ifndef THREAD_H
#define THREAD_H

#include <libxml/HTMLtree.h>
#include <libxml/xpath.h>
#include "xxhash.h"

#include "context.h"
#include "post.h"

extern int writePost(char *directory, htmlDocPtr postDoc, time_t postTimestamp, XXH64_hash_t postHash);
extern void processPost(PostData *post, Context *ctx);

#endif
