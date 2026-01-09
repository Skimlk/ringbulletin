#ifndef BULLETIN_H
#define BULLETIN_H

#include "context.h"

typedef struct {
	char *title;
	char *link;
	char *pubDate;
	char *description;
} PostData;

typedef struct {
	char *postFileDate;
	char *postFileHash;
} PostFile;

extern int writeBulletin();
extern int writePost(const PostData *post, Context *ctx);

#endif
