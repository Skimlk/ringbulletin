#ifndef PROMPT_H
#define PROMPT_H

#include <stdbool.h>

extern char *stringInputPrompt(char *message, char *defaultInput);
extern bool boolInputPrompt(char *message, bool defaultInput);
extern bool overwriteFilePrompt(char *message);
extern int intInputPrompt(char *message, int *defaultInput);

#endif
