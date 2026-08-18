#ifndef PROMPT_H
#define PROMPT_H

#include <stdbool.h>

extern char *stringInputPrompt(char *message, char *defaultInput);
extern bool boolInputPrompt(char *message, bool defaultInput);
extern int intInputPrompt(char *message, int *defaultInput);

#endif
