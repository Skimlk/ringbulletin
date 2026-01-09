#ifndef CONTEXT_H
#define CONTEXT_H

#include <cjson/cJSON.h>

#include "config.h"

typedef struct {
	int reload;
} Options;

typedef struct {
	ConfigValues *config;
	Options *options;
	cJSON *searchHistory;
} Context;

#endif