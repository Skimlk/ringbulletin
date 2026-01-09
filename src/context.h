#ifndef CONTEXT_H
#define CONTEXT_H

#include <cjson/cJSON.h>

#include "config.h"

typedef struct {
	ConfigValues *config;
} Context;

#endif