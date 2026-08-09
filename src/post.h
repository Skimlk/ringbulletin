#ifndef POST_H
#define POST_H

#include <time.h>
#include "xxhash.h"

typedef struct {
	char *title;
	char *link;
	char *domain;
	char *description;
	XXH64_hash_t normalizedTitleHash;
	char *normalizedTitleHashString;
	time_t pubDateUnix;
	char *pubDateFormattedString;
	char *iconPath;
} PostData;

extern PostData *initalizePost();
extern void copyPostData(PostData *newPost, PostData *originalPost);
extern void freePostData(PostData *post);

#endif
