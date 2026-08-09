#include <stdlib.h>
#include <string.h>

#include "post.h"

PostData *initalizePost() {
    PostData *post = malloc(sizeof(PostData));
   
    post->title = NULL;
    post->link = NULL;
    post->domain = NULL;
    post->description = NULL;
    post->normalizedTitleHashString = NULL;
    post->pubDateFormattedString = NULL;
    post->iconPath = NULL;
    
    return post;
}

void copyPostData(PostData *newPost, PostData *originalPost) {
    newPost->title = strdup(originalPost->title);
    newPost->link = strdup(originalPost->link);
	newPost->domain = strdup(originalPost->domain);
	newPost->description = strdup(originalPost->description);
	newPost->normalizedTitleHash = originalPost->normalizedTitleHash;
	newPost->normalizedTitleHashString = strdup(originalPost->normalizedTitleHashString);
	newPost->pubDateUnix = originalPost->pubDateUnix;
	newPost->pubDateFormattedString = strdup(originalPost->pubDateFormattedString);
    newPost->iconPath = strdup(originalPost->iconPath);
}

void freePostData(PostData *post) {
    free(post->title);
    free(post->link);
    free(post->domain);
    free(post->description);
    free(post->normalizedTitleHashString);
    free(post->pubDateFormattedString);
    free(post->iconPath);
    free(post);
}
