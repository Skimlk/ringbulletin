#ifndef BULLETIN_H
#define BULLETIN_H

#include <time.h>
#include "xxhash.h"

#include "context.h"

typedef struct {
	char *title;
	char *link;
	char *description;
	XXH64_hash_t normalizedTitleHash;
	char normalizedTitleHashString[17];
	time_t pubDateUnix;
	char *pubDateFormattedString;
} PostData;

extern int writeBulletin();
extern int writePost(const PostData *post, Context *ctx);

#endif
